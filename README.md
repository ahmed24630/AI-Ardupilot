# CMake build system — parameterized, multi-board

## Honest note before you start

I don't have `cmake` available in the environment I'm writing this in (no
network to install it, and it's not preinstalled), so **I wrote this
carefully by reasoning through CMake's actual behavior, but I have not
run it myself.** Try it on your VM and paste me any errors — there's a
decent chance something needs a small fix, and that's a normal part of
build-system work, not a sign something is deeply wrong.

## The one concept that matters most here

**CMake picks the compiler once, the first time you run `cmake` in a given
build directory, and caches it.** You cannot pass a parameter later to
switch from native g++ to a cross-compiler in the same build directory —
you have to use a **separate build directory per target**, each configured
once with the right toolchain file.

This is different from a plain Makefile (like the one you might have seen
elsewhere) where you can just do `make ARCH=aarch64` and it re-evaluates
everything every time. CMake's model is: configure once (expensive,
figures out compiler/platform details), build many times (cheap, just
recompiles changed files). That's *why* the target has to be locked in at
configure time.

## Files

```
.
├── CMakeLists.txt           the build definition
├── sources.txt              list of .cpp files to compile (edit as you add files)
├── toolchains/
│   ├── aarch64.cmake        for Raspberry Pi 4/5 (64-bit OS), Jetson
│   └── armhf.cmake          for Raspberry Pi (32-bit OS), BeagleBone
└── src/
    └── main.cpp             (currently just your hello-world, replace as you build)
```

## Usage

A separate build directory per target, each created once. All of them live
under one `build/` parent folder — this makes it trivial to `.gitignore`
(just ignore `build/` entirely) and keeps every target's output in a
predictable place: **`build/<target>/bin/ai_pilot_setup-<BOARD>`**, always,
regardless of how you invoke `cmake` — this is enforced by
`RUNTIME_OUTPUT_DIRECTORY` in `CMakeLists.txt`, not just a naming
convention you have to remember.

```bash
# Native (runs on your VM, for quick testing)
cmake -B build/native -DBOARD=native
cmake --build build/native
# binary: build/native/bin/ai_pilot_setup-native

# Raspberry Pi 4/5 (64-bit OS), or Jetson
cmake -B build/rpi64 -DCMAKE_TOOLCHAIN_FILE=toolchains/aarch64.cmake -DBOARD=rpi64
cmake --build build/rpi64
# binary: build/rpi64/bin/ai_pilot_setup-rpi64

cmake -B build/jetson -DCMAKE_TOOLCHAIN_FILE=toolchains/aarch64.cmake -DBOARD=jetson
cmake --build build/jetson
# binary: build/jetson/bin/ai_pilot_setup-jetson

# Raspberry Pi (32-bit OS)
cmake -B build/rpi32 -DCMAKE_TOOLCHAIN_FILE=toolchains/armhf.cmake -DBOARD=rpi32
cmake --build build/rpi32
# binary: build/rpi32/bin/ai_pilot_setup-rpi32

# BeagleBone Black/Blue
cmake -B build/beaglebone -DCMAKE_TOOLCHAIN_FILE=toolchains/armhf.cmake -DBOARD=beaglebone
cmake --build build/beaglebone
# binary: build/beaglebone/bin/ai_pilot_setup-beaglebone
```

Each produces a distinctly-named binary inside its own `bin/` subfolder, so
they never collide even though they all live under the same `build/` root.

**`BOARD` and `CMAKE_TOOLCHAIN_FILE` are two different things:**
- `CMAKE_TOOLCHAIN_FILE` picks the actual compiler (native vs. one of the
  two cross-compilers) — this is what makes the binary run on ARM at all.
- `BOARD` only adds CPU-specific tuning flags (like `-mcpu=cortex-a72`) on
  top of whichever compiler the toolchain file already selected — it makes
  the binary run *faster* on that specific board, but isn't what makes it
  runnable there in the first place.

If you pass a mismatched pair (e.g. `-DBOARD=rpi64` without the aarch64
toolchain file), `CMakeLists.txt` will stop with a clear error telling you
exactly what's wrong, rather than a cryptic compiler failure.

## Adding your own source files

Edit `sources.txt` — one path per line, relative to the project root. You
don't touch `CMakeLists.txt` at all when you add a new file; it re-reads
`sources.txt` every time you re-run `cmake --build`.

## Rebuilding after editing code

You only need to re-run the initial `cmake -B ... ` command if you change
`CMakeLists.txt` itself or add/remove files from `sources.txt`. For normal
code edits, just:

```bash
cmake --build build/native
```

## Verifying what you built

Same as with the plain Makefile version:

```bash
file build-native/ai_ARDUPILOT-native
file build-rpi64/ai_ARDUPILOT-rpi64
```

Confirm the architecture matches what you expect before copying anything
to a board.
