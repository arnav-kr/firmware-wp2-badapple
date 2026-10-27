"""Check tracked/publishable files for accidental local inputs and credentials."""
import argparse
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
FORBIDDEN_SUFFIXES = {".bin", ".a", ".lib", ".elf", ".exe", ".obj", ".o", ".mp3", ".mp4", ".gray", ".zframes", ".zindex", ".log"}
PATTERNS = {
    "absolute Windows path": re.compile(r"[A-Za-z]:[\\/](?:Users|upitest)[\\/]", re.I),
    "GitHub credential": re.compile(r"(?:gh[pousr]_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{30,})"),
    "private key": re.compile(r"-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----"),
    "device MAC literal": re.compile(r"(?<![A-Za-z0-9])(?:[A-Fa-f0-9]{2}:){5}[A-Fa-f0-9]{2}(?![A-Za-z0-9])"),
}


def audit():
    result = subprocess.run(["git", "ls-files", "-z", "--cached", "--others", "--exclude-standard"],
                            cwd=ROOT, check=True, capture_output=True)
    paths = sorted(set(result.stdout.decode().strip("\0").split("\0")))
    errors, total = [], 0
    for name in filter(None, paths):
        path = ROOT / name
        if not path.is_file():
            continue
        total += path.stat().st_size
        if path.suffix.lower() in FORBIDDEN_SUFFIXES or name.startswith(("generated/", "assets/private/", "local/", "build/", "dist/")):
            errors.append({"file": name, "reason": "local/binary input"})
            continue
        data = path.read_bytes()
        if b"\0" in data:
            errors.append({"file": name, "reason": "binary content"})
            continue
        text = data.decode("utf-8", errors="replace")
        for label, pattern in PATTERNS.items():
            if pattern.search(text):
                errors.append({"file": name, "reason": label})
    return {"files": len(paths), "bytes": total, "findings": errors}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.parse_args()
    report = audit()
    print(json.dumps(report, indent=2))
    raise SystemExit(1 if report["findings"] else 0)
