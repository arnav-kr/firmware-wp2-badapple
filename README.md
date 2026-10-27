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

An independent custom application built with the Actions ATS3089C SDK and Zephyr.
It replaces the stock application with a resource-independent LVGL player; it is
**not a patched stock application binary**. Vendor libraries, a small SDFS image,
and the existing bootloader/recovery remain dependencies.

Unofficial project, unaffiliated with Nothing / CMF. Target: JX402 / Watch Pro 2.
This is an experimental player and hardware port, not a complete daily-use watch OS.

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

Earlier v10–v12 builds had user-confirmed crown/touch operation and audible sound;
v12 fullscreen audio was smooth, but it later developed a persistent black display.
The v13 baseline corrects differences in panel reset and shutdown sequencing.
Its OTA transfer/commit completed, but v13 boot and sustained playback verification
are still pending. This reorganized build has not been flashed.

See [validation](docs/validation.md) for offline checks and remaining hardware work.

## Build

You need the matching Actions SDK, its ARM GNU toolchain, CMake/Ninja, and an SDK
Python environment. This repository does not bundle the SDK, vendor binaries,
stock firmware or copyrighted media. Supply the inputs described in
[vendor/README.md](vendor/README.md) and [assets/README.md](assets/README.md).

```powershell
python scripts/media.py embed

./scripts/build.ps1 `
  -SdkRoot 'D:/sdk/action_technology_sdk' `
  -ToolchainRoot 'D:/sdk/armgcc9' `
  -Python 'D:/sdk/venv/Scripts/python.exe' `
  -Package

python scripts/verify.py dist/wp2-badapple-ota.bin
```

The scripts accept explicit tool paths and leave the SDK unchanged. Build output
goes to `build/`; generated media to `generated/`; verified OTA output to `dist/`.
The detailed [build guide](docs/build.md) explains how to prepare a new checkout.

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

Build and verification commands never connect to or flash a watch. Delivery is a
separate, explicitly chosen operation through the companion tooling; see
[OTA and recovery](docs/ota.md). Only application and SDFS partitions are packaged.
Bootloader, recovery, partition layout and large stock resource partitions are
preserved. Crown-plus-charger recovery is not proven independent of every app
failure, and no universal recovery guarantee exists.

## Attribution and licensing

Original SDK notices are retained. minimp3 is pinned and carries its CC0 license;
miniz/libspng notices remain in the SDK support tree. Vendor blobs and media are
local inputs rather than repository content. There is no blanket relicensing of
the mixed SDK-derived tree; see [LICENSE.md](LICENSE.md) and
[third-party notes](docs/third-party.md).

Companion protocol/container tooling: [CMF Watch Utils](https://github.com/arnav-kr/cmf-watch-utils).
