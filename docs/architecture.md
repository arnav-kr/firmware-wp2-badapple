# Architecture

The firmware is compiled from source against the matching Actions Zephyr SDK.
`app/sdk/main` starts the SDK system services and launches the OTA application as
the foreground player. `app/sdk/ota/ota_app_foreground.c` owns the update state
machine; `app/src/ui/player_view.c` supplies its custom LVGL view.

## Playback

`audio/codec.c` wraps minimp3 using a fixed 128 KiB index arena. `audio/music.c`
decodes mono 16 kHz samples into a private 32 KiB ring, prefills roughly 512 ms,
and starts the DAC track. The music thread runs at priority 2, ahead of the UI
thread at priority 3. The video follows consumed DAC sample count rather than a
separate wall-clock timer. OTA activity suspends music/playback.

`video/player.c` owns the zlib decoder, previous monochrome frame, delta scratch
buffer, and RGB565 output. `player.h` is the public interface. Index bit 31 marks
a keyframe; lower 31 bits give the compressed offset. Other frames XOR against
the previous frame. A backward or large forward seek starts from a keyframe.
Only the UI thread may use this module and its image buffer.

The LVGL view uses uniform scaling: normal mode fits the image, fullscreen mode
fills the square display and crops the sides. It does not stretch the picture.
VGLite wrapper counters report calls/errors but do not prove the video uses that
GPU: the SDK also has an Actions 2D path. Backend utilization remains unproven.

## Inputs and diagnostics

`input/crown.c` polls the MT3511 on I2C2, converts signed displacement, and feeds
the volume accumulator. `media_controls.h` handles debounced press/release,
700 ms holds, signed displacement and volume limits. The view samples the PMU
crown button and receives touch events through the CST820 driver.

The `pro2` shell exposes status, view switching and the synthetic speaker test.
The view still contains legacy `bringup_*` identifiers at SDK integration points;
these are compatibility symbols, not separate historical firmware versions.

## Build boundaries

`app/CMakeLists.txt` replaces selected SDK driver sources with files in `drivers/`
and `board/wp2/`; it does not patch the SDK in place. Generated wrapper headers
reference the selected SDK board headers without embedding a developer's paths
in tracked source. The obsolete `hardware13`/`media11.marker` selection mechanism
has been replaced by explicit target sources and configuration fragments.

SDK application support lives under `app/sdk/`. Retained modules include the
launcher/demo components enabled by the baseline Kconfig, even though boot goes
directly to the player. Trimming those features is separate behavior-changing
work and is not part of this migration.
