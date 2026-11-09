<h1 align="center">Watch Pro 2 · Bad Apple</h1>

<p align="center">
  Source-built Zephyr firmware for the <b>CMF Watch Pro 2</b> —
  synchronized video and speaker audio, crown controls, and BLE OTA.
</p>

<p align="center">
  <img alt="Device" src="https://img.shields.io/badge/device-CMF%20Watch%20Pro%202-black?style=for-the-badge" />
  <img alt="Status" src="https://img.shields.io/badge/status-experimental-FB471F?style=for-the-badge" />
  <a href="https://github.com/arnav-kr/cmf-watch-utils"><img alt="Companion tooling" src="https://img.shields.io/badge/tooling-cmf--watch--utils-blue?style=for-the-badge" /></a>
</p>

<p align="center">
  <a href="docs/architecture.md">Architecture</a> ·
  <a href="docs/build.md">Build guide</a> ·
  <a href="docs/hardware.md">Hardware port</a> ·
  <a href="docs/ota.md">OTA and recovery</a>
</p>

---

This project runs Bad Apple on the CMF Watch Pro 2 (JX402). It builds a new
Zephyr application against the Actions ATS3089C SDK. The player has its own UI
and media. It still uses vendor libraries, an SDFS support image and the
watch's existing bootloader and recovery.

This is an unofficial firmware demo. Start with the virtual watch if you want
to try it without changing a physical device.

[Watch Bad Apple running on a real Watch Pro 2](https://youtu.be/npPfNpdD0O8).

## Try it in the emulator

Open the [Watch Pro 2 emulator](https://cmf.arnv.dev/emulator/watch-pro-2/),
choose **Bad Apple** under **Boot preset** and start the watch. If the emulator
asks for a file, select `wp2-badapple-ota.bin` from your build or the
[latest release](https://github.com/arnav-kr/firmware-wp2-badapple/releases/latest).
The emulator extracts the application and its resources from the package.
No physical watch is involved.

Tap the display to switch between normal and fullscreen. Press the crown to
pause or resume, hold it to restart playback, and rotate it to change volume.

## Features and status

| Area | Implementation / evidence |
|---|---|
| Video | 192 × 144 monochrome, 15 fps, 3,285 frames / 219 seconds |
| Compression | zlib keyframes and XOR deltas, decoded into RGB565 |
| Audio | 16 kHz mono MP3, speaker playback with a private 32 KiB PCM queue |
| Synchronization | DAC sample clock drives the video timeline |
| Controls | Rotate crown: volume; press: pause/resume; hold ~700 ms: restart playback |
| Fullscreen | Tap to toggle; uniform scaling fills the screen by cropping sides |
| OTA | SDK Actions OTA and BLE diagnostic shell retained |
| Hardware | Custom panel, touch, crown, charger/battery and DAC adaptations |

The release passed package validation and booted to the player in QEMU. It
has not been flashed to a physical watch. See [validation](docs/validation.md)
for what was tested and what still needs a hardware check.

## Build

Follow the [build guide](docs/build.md) to set up the Actions SDK, ARM compiler,
CMake, Ninja and Python. Prepare the [vendor files](vendor/README.md) and
[media inputs](assets/README.md) before running these commands. The Git source
tree does not include those inputs.

```powershell
python scripts/media.py embed

./scripts/build.ps1 `
  -SdkRoot 'D:/sdk/action_technology_sdk' `
  -ToolchainRoot 'D:/sdk/armgcc9' `
  -Python 'D:/sdk/venv/Scripts/python.exe' `
  -Package

python scripts/verify.py dist/wp2-badapple-ota.bin
```

The build leaves the SDK unchanged and writes the firmware package to
`dist/wp2-badapple-ota.bin`.

## Layout

```text
app/src/audio/       MP3 decoder wrapper, PCM worker, speaker test/volume
app/src/input/       MT3511 crown driver and control state machines
app/src/video/       video decoder and GPU instrumentation
app/src/ui/          LVGL view, input integration and diagnostic commands
app/sdk/             retained SDK application integration and support modules
board/wp2/           board/pin overrides and reconstructed charger profile
drivers/             display, battery/charger, DAC, touch and I2C adaptations
config/              power, input and media Kconfig fragments
scripts/             configurable build, media encoder, packager and verifier
tests/               host tests for controls, panel sequence, video and MP3
third_party/minimp3/ decoder source, original license and pinned provenance
docs/                design, hardware findings, build and OTA notes
vendor/              ignored local SDK/stock inputs
assets/private/      ignored local source/encoded media
```

Some SDK demo support remains because the existing configuration still enables it.
Keeping that boundary explicit avoids silently changing working initialization or
the OTA state machine while organizing the project.

## Tests

From a terminal with a host C compiler and CMake available (Developer PowerShell
for Visual Studio on Windows):

```powershell
cmake -S tests -B build-host
cmake --build build-host --config Debug
ctest --test-dir build-host -C Debug --output-on-failure
python -m unittest discover -s tests -p "test_*.py"
```

Use Debug so C assertions remain enabled. Panel/control tests need no vendor SDK
or media. Full video/audio tests also need generated assets and private media.

## Flashing

Build and verification commands never connect to a watch. If you choose to
install the firmware on hardware, read [OTA and recovery](docs/ota.md) first.
The package updates the application and its SDFS support partition. Recovery
with crown and charger has not been proven for every app failure.

## Attribution and licensing

Original SDK notices are retained. minimp3 is pinned and carries its CC0 license;
miniz/libspng notices remain in the SDK support tree. Vendor blobs and media are
local inputs rather than repository content. There is no blanket relicensing of
the mixed SDK-derived tree; see [LICENSE.md](LICENSE.md) and
[third-party notes](docs/third-party.md).

Companion protocol/container tooling: [CMF Watch Utils](https://github.com/arnav-kr/cmf-watch-utils).
