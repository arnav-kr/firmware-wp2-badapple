# Private media inputs

Put the following files in `assets/private/`, or produce them with
`python scripts/media.py encode --source <local-video>`:

- `badapple_192x144_15fps_sync.zframes`: zlib keyframe/XOR-delta stream.
- `badapple_192x144_15fps_sync.zindex`: 3,286 little-endian 32-bit offsets.
- `badapple_song_16k.mp3`: 219 seconds, mono 16 kHz / 16 kbps MP3.
- `source_192x144_15fps.gray`: optional host-test reference, 3,285 grayscale frames.

`python scripts/media.py embed` validates the compressed frames and creates C
arrays in `generated/`, along with hashes. These generated arrays are media too
and are deliberately ignored by Git.

The development media originated from the Bad Apple source in
`CDavidSV/Bad-Apple-in-ASCII` on GitHub. That is provenance, not a grant to
redistribute the animation/song. Supply media you are entitled to use; this
repository contains the encoder/player rather than the soundtrack or frames.
