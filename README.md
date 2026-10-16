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

## Running ArduPilot SITL Simulation with MAVSDK

To run the integrated environment where the ArduPilot simulation communicates with this C++ codebase natively, follow these sequential steps in your Ubuntu VM:

### 1. First Time Setup (Dependencies)
If you haven't set up ArduPilot on your Ubuntu system yet, clone the repo and run the prerequisite script once:
```bash
# Clone ArduPilot repository
git clone --recurse-submodules https://github.com
cd ardupilot

# Install all Ubuntu prerequisites automatically
Tools/environment_install/install-prereqs-ubuntu.sh -y

# Reload profile to apply environment path updates
. ~/.profile
```

### 2. Launching ArduPilot SITL (Terminal 1)
Navigate to your preferred vehicle directory inside the `ardupilot` folder (e.g., ArduCopter) and launch the simulator. 

> **Crucial Network Rule:** Since this C++ application calls MAVSDK with `StartCommunication("udpin", 14540, 10.0);`, it sets up a local passive **Listener** on port 14540. Therefore, you must force SITL to act as the active **Sender** (`udpout`) to target localhost on that exact port:

```bash
cd ardupilot/ArduCopter

# Launch simulation with map, console, and targeted MAVLink routing
../Tools/autotest/sim_vehicle.py --map --console --out=udpout:127.0.0.1:14540
```
*Note: Use the `-w` flag (e.g., `sim_vehicle.py -w ...`) during the very first boot to wipe and clean the virtual EEPROM configurations.*

### 3. Compiling and Executing the Companion App (Terminal 2)
While the SITL simulation is fully running and active in Terminal 1 (showing the `MAV>` prompt), open a new terminal tab to build and execute your native C++ code:

```bash
# Build the native app using CMake
cmake -B build/native -DBOARD=native
cmake --build build/native

# Run your compiled binary
./build/native/bin/ai_pilot_setup-native
```
The application will safely bind to `0.0.0.0:14540`, consume the active telemetry streams coming out of the ArduPilot simulator, successfully resolve the `Autopilot found.` state via MAVSDK, and safely launch the multi-threaded state engine.