"""Validate a WP2 app/SDFS OTA without importing the SDK packager."""
import argparse
import hashlib
import json
import lzma
from pathlib import Path
import struct
import xml.etree.ElementTree as ET
import zlib

APP_LIMIT = 3452928
SDFS_LIMIT = 65536
TEMP_LIMIT = 3670016
AGPS_SHA256 = "648b05af22f7f17cb54c314a4b7a351e947e086f35878366afb021f86b8885a4"
SDFS_SHA256 = "16bec831210ff887cfb9bab5397cd16c104524dfc8cd88b16e7721a52e2e34be"


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def crc(data):
    return zlib.crc32(data) & 0xffffffff


def aota(data):
    require(len(data) >= 1024 and data[:4] == b"AOTA", "Invalid AOTA header")
    require(crc(data[8:1024]) == struct.unpack_from("<I", data, 4)[0], "Header CRC mismatch")
    start = struct.unpack_from("<H", data, 0x12)[0]
    end, checksum = struct.unpack_from("<II", data, 0x14)
    require(1024 <= start <= end <= len(data), "Invalid data bounds")
    require(crc(data[start:end]) == checksum, "Whole-data CRC mismatch")
    count = struct.unpack_from("<H", data, 12)[0]
    require(1 <= count <= 16, "Invalid file count")
    files, ranges = {}, []
    for i in range(count):
        entry = data[512+32*i:544+32*i]
        name = entry[:16].split(b"\0")[0].decode("ascii")
        offset, size, flags, checksum = struct.unpack_from("<4I", entry, 16)
        require(name not in files and start <= offset < offset+size <= end and flags == 0,
                f"Invalid entry {name}")
        require(not any(offset < high and low < offset+size for low, high in ranges), "Overlapping entries")
        ranges.append((offset, offset+size))
        files[name] = data[offset:offset+size]
        require(crc(files[name]) == checksum, f"File CRC mismatch: {name}")
    require("ota.xml" in files, "Missing OTA manifest")
    nodes = ET.fromstring(files["ota.xml"]).findall("./partitions/partition")
    declared = {node.findtext("file_name"): node for node in nodes}
    require(len(nodes) == len(declared) and set(declared) == set(files)-{"ota.xml"}, "Manifest file mismatch")
    for name, node in declared.items():
        payload = files[name]
        require(int(node.findtext("file_size"), 0) == len(payload), f"Manifest size mismatch: {name}")
        require(int(node.findtext("orig_size"), 0) == len(payload), f"Original size mismatch: {name}")
        require(int(node.findtext("checksum"), 0) == crc(payload), f"Manifest CRC mismatch: {name}")
    return files, declared, end


def decompress(packed):
    output, position = bytearray(), 0
    while position < len(packed):
        require(position+16 <= len(packed), "Truncated chunk header")
        magic, header, compressed, original = struct.unpack_from("<4I", packed, position)
        require(magic == 0x414d5a4c and header == 16, "Invalid LZMA chunk")
        require(0 < compressed+header <= 1572864 and 0 < original <= 2097152, "Receiver chunk limit exceeded")
        require(position+header+compressed <= len(packed), "Truncated chunk")
        decoder = lzma.LZMADecompressor()
        raw = decoder.decompress(packed[position+header:position+header+compressed], max_length=original+1)
        require(decoder.eof and not decoder.unused_data and len(raw) == original, "Invalid decompressed chunk")
        output.extend(raw)
        position += header+compressed
    return bytes(output)


def sum32(data):
    require(len(data) % 4 == 0, "Unaligned checksum data")
    return sum(struct.unpack(f"<{len(data)//4}I", data)) & 0xffffffff


def verify(path):
    image = Path(path).read_bytes()
    outer, outer_nodes, end = aota(image)
    require(set(outer) == {"ota.xml", "TEMP.bin"}, "Unexpected outer files")
    require(int(outer_nodes["TEMP.bin"].findtext("file_id"), 0) == 254, "Unexpected TEMP partition")
    trailer = image[end:]
    require(len(trailer) == 1242696 and trailer[:4] == b"AGPS" and sha(trailer) == AGPS_SHA256,
            "Stock CMF AGPS trailer mismatch")
    packed = outer["TEMP.bin"]
    require(len(packed) <= TEMP_LIMIT, "TEMP exceeds the measured watch partition")
    inner = decompress(packed)
    files, nodes, inner_end = aota(inner)
    require(inner_end == len(inner), "Unexpected inner trailer")
    require(set(files) == {"ota.xml", "app.bin", "sdfs.bin"}, "Unexpected inner files")
    for name, file_id, limit in [("app.bin", 4, APP_LIMIT), ("sdfs.bin", 5, SDFS_LIMIT)]:
        require(int(nodes[name].findtext("file_id"), 0) == file_id, f"Unexpected partition: {name}")
        require(int(nodes[name].findtext("storage_id"), 0) == 0, f"Unexpected storage: {name}")
        require(len(files[name]) <= limit, f"Partition limit exceeded: {name}")
    require(sha(files["sdfs.bin"]) == SDFS_SHA256, "Baseline SDFS identity mismatch")
    app = files["app.bin"]
    require(len(app) >= 512 and app[:8] == b"ACTHHTCA", "Invalid ACTH app signature")
    size = struct.unpack_from("<I", app, 24)[0]
    require(size+512 <= len(app) and size % 4 == 0, "Invalid ACTH data size")
    require(sum32(app[:512]) == 0xffffffff, "ACTH header checksum mismatch")
    require((sum32(app[512:512+size])+struct.unpack_from("<I", app, 28)[0]) & 0xffffffff == 0xffffffff,
            "ACTH data checksum mismatch")
    return {"sha256": sha(image), "image_bytes": len(image), "app_sha256": sha(app),
            "app_bytes": len(app), "sdfs_sha256": sha(files["sdfs.bin"]),
            "sdfs_bytes": len(files["sdfs.bin"]), "temp_bytes": len(packed),
            "agps_trailer_sha256": sha(trailer), "updated_ids": [4, 5],
            "checks": ["AOTA header/data/file/XML CRCs", "entry bounds and overlap",
                       "LZMA chunk bounds", "ACTH checksums", "measured partition limits", "SDFS/AGPS identities"],
            "bootloader_updated": False, "recovery_updated": False, "hardware_tested": False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    result = verify(args.image)
    output = json.dumps(result, indent=2) + "\n"
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(output, encoding="utf-8")
    print(output, end="")


if __name__ == "__main__":
    main()
