"""Package the source-built app and local SDFS; never package boot/recovery."""
import argparse
import json
import lzma
from pathlib import Path
import shutil
import struct
import sys

from verify import AGPS_SHA256, aota, crc, require, sha, verify

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk-root", required=True, type=Path)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build")
    parser.add_argument("--vendor-dir", type=Path, default=ROOT / "vendor")
    parser.add_argument("--output", type=Path, default=ROOT / "dist")
    args = parser.parse_args()
    sys.path.insert(0, str(args.sdk_root / "zephyr/tools"))
    import pack_app_image as pack
    import build_ota_image as builder

    output = args.output
    inner_dir, outer_dir = output / "inner", output / "outer"
    inner_dir.mkdir(parents=True, exist_ok=True)
    outer_dir.mkdir(parents=True, exist_ok=True)
    # Remove stale files from the packager input directories, never user inputs.
    for directory in [inner_dir, outer_dir]:
        for path in directory.iterdir():
            require(path.is_file(), f"Unexpected subdirectory in packaging workspace: {path}")
            path.unlink()
    raw = args.build_dir / "zephyr/zephyr.bin"
    shutil.copyfile(raw, inner_dir / "app.bin")
    (inner_dir / "fw_version.bin").write_bytes(b"badapple14_261105\0")
    pack.boot_post_build(str(inner_dir / "app.bin"))
    app = (inner_dir / "app.bin").read_bytes()
    size = struct.unpack_from("<I", app, 24)[0]
    require(app[512:512+size] == raw.read_bytes()[512:512+size], "Packager changed executable payload")
    shutil.copyfile(args.vendor_dir / "sdfs.bin", inner_dir / "sdfs.bin")
    version = {"version_code": "0x10000", "version_res": "0x10000",
               "version_name": "1.00_badapple14", "board_name": "jx402_01_3089c"}
    builder.LZMA_OTA_BIN = 0
    builder.LZMA_TEMP_BIN = 0
    fw = builder.ota_fw()
    fw.fw_version = version.copy()
    fw.partitions = [
        {"type": "SYSTEM", "name": "fw0_sys", "file_id": "4", "storage_id": "0",
         "file_name": "app.bin", "enable_ota": "true"},
        {"type": "DATA", "name": "fw0_sdfs", "file_id": "5", "storage_id": "0",
         "file_name": "sdfs.bin", "enable_ota": "true"},
    ]
    inner_path = output / "inner.bin"
    fw.generate_ota_image_internal(str(inner_path), str(inner_dir))
    inner = inner_path.read_bytes()
    aota(inner)
    compressed = bytearray()
    for offset in range(0, len(inner), 2097152):
        block = inner[offset:offset+2097152]
        xz = lzma.compress(block)
        compressed.extend(struct.pack("<4I", 0x414d5a4c, 16, len(xz), len(block)))
        compressed.extend(xz)
    (outer_dir / "TEMP.bin").write_bytes(compressed)
    fw = builder.ota_fw()
    fw.fw_version = version.copy()
    fw.partitions = [{"type": "TEMP", "name": "fw0_temp", "file_id": "254", "storage_id": "2",
                      "file_name": "TEMP.bin", "enable_ota": "true"}]
    image_path = output / "wp2-badapple-ota.bin"
    fw.generate_ota_image_internal(str(image_path), str(outer_dir))
    stock = (args.vendor_dir / "stock.bin").read_bytes()
    require(len(stock) >= 1024 and stock[:4] == b"AOTA", "Invalid stock container")
    require(crc(stock[8:1024]) == struct.unpack_from("<I", stock, 4)[0], "Stock header CRC mismatch")
    stock_end = struct.unpack_from("<I", stock, 0x14)[0]
    require(1024 <= stock_end <= len(stock), "Invalid stock container length")
    trailer = stock[stock_end:]
    require(sha(trailer) == AGPS_SHA256, "Wrong stock AGPS trailer")
    image_path.write_bytes(image_path.read_bytes() + trailer)
    report = verify(image_path)
    report["variant"] = "badapple14"
    (output / "manifest.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
