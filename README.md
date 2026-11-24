# wavmod

wavmod is a read-only command-line utility for examining RIFF/WAVE audio
files. It validates the file structure, prints the format properties, and
decodes supported samples into a temporary file-backed store of `double`
values.

The current reader supports:

- Unsigned 8-bit integer PCM
- Signed 16-, 24-, and 32-bit integer PCM
- 32- and 64-bit IEEE floating-point audio
- G.711 mu-law and A-law audio
- WAVE_FORMAT_EXTENSIBLE files
- Mono, stereo, and multichannel audio

RIFX and RF64 files are recognized but are not currently supported.

## Requirements

- A POSIX-compatible system
- A C11 compiler
- `gmake` or a compatible `make`

## Build

```sh
gmake
```

## Run

```sh
./wavmod file.wav
```

The utility accepts exactly one input filename and does not modify the input
file.

## Clean

```sh
gmake clean
```

## License

wavmod is free software licensed under the GNU General Public License,
version 3 or (at your option) any later version. See `LICENSE` for the full
license text.
