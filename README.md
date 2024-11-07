# massget

A tiny program written for the express purpose of bulk downloading files from HTTP Servers and saving it as files

Licensed under the "Unlicense"
All code is thus dedicated to the public domain

## Usage

See `massget` itself

## Building

### Linux/BSD

We have a couple dependencies:

- `libcurl`
- `gcc>=12`/`clang>=16` (build-only)
- `make>=4` (build-only)
- `bash` (build-only)
- `python3>=3.8` (build-only)

Install these, then run `make configure`.

IMPORTANT: If this fails, you probably forgot to install something

Now, run `make release`

Enjoy your public domain utility in dist/massget

### Any Other Unix (i.e. Solaris, AIX)

Any C compiler compatible with C99 should work, although keep in mind that you will need to edit the `Makefile` to set a correct `CC`.
It may also be required to modify the arguments provided, in `COREFLAGS`, `RELEASEFLAGS`, `LINKFLAGS` and `DEBUGFLAGS`

_Note for old AIX: `strtoimax` must exist and be functional, else argument parsing breaks. This is checked for in the headers_

### macOS

Untested, but should work with no changes

### Windows

Use msys2
