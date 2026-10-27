"""Encode private media or embed validated 192x144/15fps assets as C arrays."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import zlib

ROOT = Path(__file__).resolve().parents[1]
WIDTH, HEIGHT, FPS, FRAMES = 192, 144, 15, 3285
BASE = "badapple_192x144_15fps_sync"


def encode(args):
    from PIL import Image
    assets = args.assets
    assets.mkdir(parents=True, exist_ok=True)
    raw_path = assets / "source_192x144_15fps.gray"
    subprocess.run([args.ffmpeg, "-y", "-i", str(args.source), "-t", "219",
                    "-vf", f"fps={FPS},scale={WIDTH}:{HEIGHT}:flags=lanczos",
                    "-pix_fmt", "gray", "-f", "rawvideo", str(raw_path)], check=True)
    subprocess.run([args.ffmpeg, "-y", "-i", str(args.source), "-t", "219", "-vn",
                    "-af", "volume=0.5", "-ar", "16000", "-ac", "1", "-c:a",
                    "libmp3lame", "-b:a", "16k", str(assets / "badapple_song_16k.mp3")], check=True)
    raw = raw_path.read_bytes()
    if len(raw) != WIDTH * HEIGHT * FRAMES:
        raise ValueError("Source must provide all 3285 frames (219 seconds)")
    previous, blob, offsets = bytes(WIDTH * HEIGHT // 8), bytearray(), []
    for i in range(FRAMES):
        image = Image.frombytes("L", (WIDTH, HEIGHT), raw[i*WIDTH*HEIGHT:(i+1)*WIDTH*HEIGHT])
        frame = image.convert("1", dither=Image.Dither.NONE).tobytes()
        delta = bytes(a ^ b for a, b in zip(frame, previous))
        key, diff = zlib.compress(frame, 9), zlib.compress(delta, 9)
        is_key = i % 30 == 0 or len(key) < len(diff)
        offsets.append(len(blob) | (0x80000000 if is_key else 0))
        blob.extend(key if is_key else diff)
        previous = frame
    offsets.append(len(blob))
    (assets / f"{BASE}.zframes").write_bytes(blob)
    (assets / f"{BASE}.zindex").write_bytes(struct.pack(f"<{len(offsets)}I", *offsets))
    embed(args)


def c_array(name, data, words=False):
    width = 12 if words else 32
    values = [f"0x{x:08x}" if words else f"0x{x:02x}" for x in data]
    lines = [",".join(values[i:i+width]) + "," for i in range(0, len(values), width)]
    return f"const {'uint32_t' if words else 'uint8_t'} {name}[] = {{\n" + "\n".join(lines) + "\n};\n"


def embed(args):
    assets = args.assets
    video = (assets / f"{BASE}.zframes").read_bytes()
    index = (assets / f"{BASE}.zindex").read_bytes()
    mp3 = (assets / "badapple_song_16k.mp3").read_bytes()
    if len(index) != (FRAMES + 1) * 4:
        raise ValueError("Unexpected frame index length")
    offsets = struct.unpack(f"<{FRAMES+1}I", index)
    if offsets[0] != 0x80000000 or offsets[-1] != len(video):
        raise ValueError("Index must start with a keyframe and end at the blob boundary")
    for i in range(FRAMES):
        start, end = offsets[i] & 0x7fffffff, offsets[i+1] & 0x7fffffff
        if not 0 <= start < end <= len(video):
            raise ValueError(f"Invalid frame range {i}")
        if len(zlib.decompress(video[start:end])) != WIDTH * HEIGHT // 8:
            raise ValueError(f"Invalid decoded frame size {i}")
    if not mp3:
        raise ValueError("Empty MP3")
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "badapple_data.c").write_text(
        "#include <stdint.h>\n" + c_array("badapple_data", video) +
        c_array("badapple_offsets", offsets, words=True) +
        f"const uint32_t badapple_frame_count={FRAMES};\n", encoding="utf-8")
    (args.output / "badapple_audio_data.c").write_text(
        "#include <stdint.h>\n" + c_array("badapple_mp3", mp3) +
        f"const uint32_t badapple_mp3_bytes={len(mp3)};\n", encoding="utf-8")
    report = {"width": WIDTH, "height": HEIGHT, "fps": FPS, "frames": FRAMES,
              "assets": {name: {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}
                         for name, data in [("video", video), ("index", index), ("mp3", mp3)]}}
    (args.output / "media-manifest.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=["encode", "embed"])
    parser.add_argument("--assets", type=Path, default=ROOT / "assets/private")
    parser.add_argument("--output", type=Path, default=ROOT / "generated")
    parser.add_argument("--source", type=Path)
    parser.add_argument("--ffmpeg", default="ffmpeg")
    args = parser.parse_args()
    if args.command == "encode" and not args.source:
        parser.error("encode requires --source")
    (encode if args.command == "encode" else embed)(args)


if __name__ == "__main__":
    main()
