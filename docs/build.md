# Build from a new checkout

## Dependencies

- Matching Actions `action_technology_sdk`, board `ats3089c_dev_watch` / ATS3089C.
- ARM GNU toolchain used by the original baseline (GCC 9 family, `gnuarmemb`).
- CMake and Ninja on PATH, or explicit `-CMake` / `-Ninja` paths.
- SDK Python environment with its Zephyr dependencies (including pyelftools).
- For media encoding: ffmpeg with libmp3lame and Pillow (`pip install -r requirements.txt`).

The build was checked with [Actions SDK revision `26f51e5`](https://github.com/lvgl/lv_port_actions_technology/tree/26f51e584940d5b141cd1d09d2629d69b595f579/action_technology_sdk)
and Arm GNU 9.3.1. Point `-SdkRoot` at the `action_technology_sdk` directory in
that checkout. The script prepares the board configuration, compiler cache and
FreeType archive under `build/`.

## Prepare local inputs

Follow [vendor inputs](../vendor/README.md) to supply `libbt_test.a`, `sdfs.bin`
and `stock.bin`. Follow [media inputs](../assets/README.md) to obtain the video
and audio you are entitled to use and generate or copy the encoded assets.
These folders are ignored by Git.

```powershell
python scripts/media.py encode --source 'D:/media/bad-apple.mp4'
# Or, if the encoded files already exist:
python scripts/media.py embed
```

Encoding targets the established 219 second, 192x144, 15fps layout. It reduces
audio amplitude by half and produces 16 kHz mono / 16 kbps MP3. An input with
different timing is not silently accepted. ffmpeg/Pillow/zlib versions can
change output bytes; newly encoded media needs full verification and a fresh
memory-budget check.

## Compile and package

```powershell
./scripts/build.ps1 `
  -SdkRoot 'D:/sdk/action_technology_sdk' `
  -ToolchainRoot 'D:/sdk/armgcc9' `
  -Python 'D:/sdk/venv/Scripts/python.exe' `
  -CMake 'cmake' -Ninja 'ninja' -Package
```

The script embeds validated media, configures the SDK board, compiles into
`build/`, then optionally packages. SDK Python must also satisfy the SDK's
packaging imports. `build.ps1` does not install tools or flash a device.

To package an existing build independently:

```powershell
python scripts/package.py --sdk-root 'D:/sdk/action_technology_sdk'
python scripts/verify.py dist/wp2-badapple-ota.bin --report dist/verification.json
```

The resulting manifest contains SHA-256 identities and offline verification
results for `badapple14`. `hardware_tested` remains false.

For a local ATS3089C QEMU workspace with `probe.py` and prepared NAND, pass
`-Preflight -EmulatorRoot PATH` to verify the package and test its application
boot after packaging. This checks the virtual handoff and rendered display.

## Capacity

Measured limits on the development watch: app 3,452,928 bytes; SDFS 65,536 bytes;
TEMP staging 3,670,016 bytes. The linker is deliberately capped at 3,440,640 bytes.
The verifier also checks compressed chunk + header <= 1,572,864 bytes and raw
chunk <= 2,097,152 bytes. Do not substitute the larger SDK reference partition map.

See [validation](validation.md) for the checks run on this migration.
