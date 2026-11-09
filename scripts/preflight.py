"""Validate the OTA package and boot its exact payloads in strict QEMU."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from verify import require, sha, verify_package


def preflight(package, emulator):
    logs = emulator / "logs"
    logs.mkdir(exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix="ota-preflight-", dir=logs))
    report = {
        "package": str(package),
        "result": "failed",
        "coverage": "package validation and application handoff smoke test",
        "ota_install_tested": False,
        "physical_watch_tested": False,
    }
    try:
        verification, payloads = verify_package(package.read_bytes())
        report["package_verification"] = verification
        for name in ("app.bin", "sdfs.bin"):
            (run / name).write_bytes(payloads[name])

        # Ignore inherited debug patches, MMIO exploration and device bridges.
        env = {key: value for key, value in os.environ.items()
               if not key.upper().startswith("QEMU_WP2_")}
        command = [
            sys.executable, str(emulator / "probe.py"),
            "--image", str(run / "app.bin"), "--sdfs", str(run / "sdfs.bin"),
            "--nand", str(emulator / "images/nand-empty-b-defaults-language.bin"),
            "--boot-app-from-nor", "--icount", "--unplugged",
            "--seconds", "12", "--name", run.name,
        ]
        with (run / "probe-console.log").open("wb") as log:
            subprocess.run(command, env=env, stdout=log, stderr=subprocess.STDOUT,
                           check=True, timeout=45)

        probe = json.loads((run / "report.json").read_text())
        report["probe_outcome"] = probe["outcome"]
        require(probe["outcome"] == "bounded_observation_complete",
                f"Strict QEMU probe failed: {probe['outcome']}")
        require(probe["boot_app_from_nor"], "QEMU bypassed the staged NOR application")
        require(not (run / "stderr.log").read_bytes().strip(),
                "QEMU reported a hardware model or runtime error")
        require(probe["sha256_before"] == probe["sha256_after"] == verification["app_sha256"],
                "QEMU did not test the exact verified app")
        require(sha(package.read_bytes()) == verification["sha256"],
                "OTA package changed during preflight")
        uart = (run / "uart0.log").read_text(errors="replace")
        require(uart.count("*** Booting Zephyr OS") == 1, "Expected one Zephyr boot")
        for marker in ("bt init ok!", "PRO2 touch chip: b7", "view 15 layout finished"):
            require(marker in uart, f"Missing firmware startup marker: {marker}")
        frame = (run / "frame.ppm").read_bytes()
        header = b"P6\n466 466\n255\n"
        require(frame.startswith(header) and len(frame) == len(header) + 466 * 466 * 3,
                "Missing or malformed panel frame")
        pixels = frame[len(header):]
        require(len(set(zip(pixels[::3], pixels[1::3], pixels[2::3]))) > 2,
                "Panel frame is blank or lacks UI detail")
        report["result"] = "passed"
    except (OSError, ValueError, KeyError, subprocess.SubprocessError) as exc:
        report["error"] = str(exc)
    path = run / "preflight.json"
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"OTA preflight {report['result']}: {path}")
    if "error" in report:
        print(report["error"], file=sys.stderr)
    return report["result"] == "passed"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("package", type=Path)
    parser.add_argument("--emulator", type=Path,
                        default=Path(__file__).resolve().parents[2] / "qemu-wp2")
    args = parser.parse_args()
    return 0 if preflight(args.package.resolve(), args.emulator.resolve()) else 1


if __name__ == "__main__":
    sys.exit(main())
