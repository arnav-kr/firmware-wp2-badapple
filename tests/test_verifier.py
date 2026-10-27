"""Exercise corruption rejection independently of the vendor SDK and media."""
import lzma
from pathlib import Path
import struct
import sys
import unittest
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from verify import aota, decompress


class ChunkTests(unittest.TestCase):
    def test_round_trip(self):
        raw = b"watch data" * 100
        packed = lzma.compress(raw)
        chunk = struct.pack("<4I", 0x414d5a4c, 16, len(packed), len(raw)) + packed
        self.assertEqual(decompress(chunk), raw)
        with self.assertRaises(ValueError):
            decompress(chunk[:-1])

    def test_receiver_limit(self):
        chunk = struct.pack("<4I", 0x414d5a4c, 16, 1572864, 100)
        with self.assertRaisesRegex(ValueError, "limit"):
            decompress(chunk)

    def test_trailing_compressed_data(self):
        packed = lzma.compress(b"watch") + b"unexpected"
        chunk = struct.pack("<4I", 0x414d5a4c, 16, len(packed), 5) + packed
        with self.assertRaisesRegex(ValueError, "decompressed"):
            decompress(chunk)

    def test_raw_size_mismatch(self):
        packed = lzma.compress(b"watch")
        chunk = struct.pack("<4I", 0x414d5a4c, 16, len(packed), 4) + packed
        with self.assertRaises(ValueError):
            decompress(chunk)


def container():
    payload = b"test payload"
    checksum = zlib.crc32(payload) & 0xffffffff
    xml = ("<ota_firmware><partitions><partition><file_name>app.bin</file_name>"
           f"<file_size>{len(payload)}</file_size><orig_size>{len(payload)}</orig_size>"
           f"<checksum>{checksum}</checksum></partition></partitions></ota_firmware>").encode()
    image = bytearray(1024)
    image[:4] = b"AOTA"
    struct.pack_into("<H", image, 12, 2)
    struct.pack_into("<H", image, 0x12, 1024)
    for i, (name, data) in enumerate([("ota.xml", xml), ("app.bin", payload)]):
        entry = name.encode().ljust(16, b"\0") + struct.pack("<4I", len(image), len(data), 0, zlib.crc32(data))
        image[512+32*i:544+32*i] = entry
        image.extend(data)
    struct.pack_into("<II", image, 0x14, len(image), zlib.crc32(image[1024:]))
    struct.pack_into("<I", image, 4, zlib.crc32(image[8:1024]))
    return image


class ContainerTests(unittest.TestCase):
    def test_minimal_container(self):
        files, _, end = aota(container())
        self.assertEqual(files["app.bin"], b"test payload")
        self.assertEqual(end, len(container()))

    def test_header_corruption(self):
        image = container()
        image[100] ^= 1
        with self.assertRaisesRegex(ValueError, "Header CRC"):
            aota(image)

    def test_whole_data_corruption(self):
        image = container()
        image[-1] ^= 1
        with self.assertRaisesRegex(ValueError, "Whole-data"):
            aota(image)

    def test_file_crc_even_if_data_crc_recomputed(self):
        image = container()
        image[-1] ^= 1
        struct.pack_into("<I", image, 0x18, zlib.crc32(image[1024:]))
        struct.pack_into("<I", image, 4, zlib.crc32(image[8:1024]))
        with self.assertRaisesRegex(ValueError, "File CRC"):
            aota(image)


if __name__ == "__main__":
    unittest.main()
