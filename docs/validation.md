# Migration validation

## Bad Apple v14

The Bad Apple-only source built from a fresh checkout with the SDK revision
and local inputs listed in the build guide. All five host tests and eight
Python tests passed. The repository audit found no local inputs or credentials
in the files to be committed.

The package passed CRC, ACTH checksum, partition and AGPS checks. The strict
QEMU preflight booted its exact app from virtual NOR and observed the rendered
player. This checks application startup. Physical flashing and sustained
playback remain unverified for v14.

The checked package is 3,851,336 bytes. Its SHA-256 is
`563b993656f8ebfc21d4e79e9eb7eb3c8b6ce483fcfbdb10198cffecfd8c4a88`.

## Earlier source migration

The organized project starts from the control13 source baseline, keeps hardware
behavior/configuration, and moves code into named modules. The video decoder
now has a normal `.c` implementation and a public header/reset function, rather
than defining all its storage and implementation inside the UI's header.

## Completed offline

- Fresh ARM configure/build from this folder with the existing matching SDK.
- Final ARM rebuild after separating the decoder; packaging completed.
- Five host tests passed: panel, control state machine, crown/tone, video, audio.
- Video: all 3,285 frames match thresholded source pixels; forward/backward seeks pass.
- Audio: full mono 16 kHz song decodes, has nonzero samples, correct duration and sample-accurate seek.
- Panel: 1,000 simulated lifecycle cycles match the stock-derived ordering/waits.
- Eight Python tests pass, including malformed/truncated chunks, receiver limits,
  header/data corruption and a file CRC failure hidden by a recomputed data CRC.
- New independent verifier accepts both the historical v13 image and the new image.
- Repository audit finds no firmware/media binaries, device MAC literals, private
  keys, GitHub credentials or developer-specific absolute paths in publishable files.

## Final local build

| Region | Used |
|---|---:|
| FLASH | 3,383,596 bytes / 3,440,640 |
| SRAM | 574,208 bytes / 911 KiB |
| PSRAM | 6,765,600 bytes / 8 MiB |
| SHARE_RAM | 20,948 bytes / 20,991 |

The shared RAM region remains very close to its baseline limit. This migration
does not create new media capacity or establish extra runtime headroom.

Local OTA: 3,851,336 bytes, SHA-256
`9d2c261ade8947468b193168c8730dc69ad1b7de3cec994572cb6cd14c7f03e9`.
App: 3,383,808 padded bytes. TEMP: 2,606,136 bytes. SDFS: 45,824 bytes.
These are validation identities, not published firmware downloads. Outputs and
full local logs are ignored under `dist/`, `build/` and `local/`.

## Hardware status

No BLE commands or flashes were performed during this migration. Earlier v13
transfer/commit completion is recorded separately in `reference-v13.json` and
does not verify this refactored image. Boot, sustained playback, fullscreen
transitions, battery behavior and display sleep/recovery still need an explicitly
authorized hardware test. The original black-display trigger remains unproven.

GitHub CI runs the source-only panel/control/crown and verifier tests without
vendor SDK or media. ARM builds and full media tests require the local inputs.
