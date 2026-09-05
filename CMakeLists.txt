# CMakeLists.txt — builds ai_ARDUPILOT, reading sources from sources.txt
# and applying board-specific compiler options via the BOARD variable.
#
# IMPORTANT CMake concept: the compiler itself (native g++ vs a cross
# compiler) is chosen by a TOOLCHAIN FILE, passed on the first `cmake` call
# in a given build directory — not by a variable read down here. By the
# time this file runs, that choice is already locked in. BOARD here only
# controls *extra tuning flags* (like -mcpu) on top of whichever compiler
# was already selected. See README.md for why.

cmake_minimum_required(VERSION 3.16)
project(ai_ARDUPILOT)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Read the source file list from sources.txt --------------------------
# file(STRINGS ...) reads the file one line into one list entry per line.
# We then drop blank lines and comment lines (starting with #) ourselves,
# since file(STRINGS) doesn't filter those automatically.
file(STRINGS "${CMAKE_SOURCE_DIR}/sources.txt" RAW_SOURCE_LINES)

set(PROJECT_SOURCES "")
foreach(LINE ${RAW_SOURCE_LINES})
    string(STRIP "${LINE}" TRIMMED_LINE)
    if(TRIMMED_LINE STREQUAL "")
        continue()  # skip blank lines
    endif()
    if(TRIMMED_LINE MATCHES "^#")
        continue()  # skip comment lines
    endif()
    # Extra safety net: only accept lines that actually look like a source
    # or header path. file(STRINGS) can split a comment line into fragments
    # in unexpected places (e.g. around certain punctuation), which would
    # otherwise let a stray fragment slip through even though it never
    # started with #. Requiring a .cpp/.hpp ending catches that case too.
    # (.hpp files won't be compiled as their own translation units — CMake
    # only does that for .cpp — but listing them here makes IDEs like CLion
    # or VS Code show them in the project tree alongside their .cpp files.)
    if(NOT TRIMMED_LINE MATCHES "\\.(cpp|hpp)$")
        message(WARNING "Ignoring unexpected line in sources.txt (doesn't end in .cpp or .hpp): ${TRIMMED_LINE}")
        continue()
    endif()
    list(APPEND PROJECT_SOURCES "${CMAKE_SOURCE_DIR}/${TRIMMED_LINE}")
endforeach()

if(NOT PROJECT_SOURCES)
    message(FATAL_ERROR "No valid source files found in sources.txt. Check the file has at least one line ending in .cpp or .hpp that isn't commented out.")
endif()

message(STATUS "Building with sources: ${PROJECT_SOURCES}")

add_executable(ai_ARDUPILOT ${PROJECT_SOURCES})

target_include_directories(ai_ARDUPILOT PRIVATE ${CMAKE_SOURCE_DIR}/Src/App_Transitions)

# --- Common flags for every target ----------------------------------------
target_compile_options(ai_ARDUPILOT PRIVATE -Wall -Wextra -O2)

# Statically link the C++ runtime so the binary doesn't depend on the
# target board having a matching libstdc++/libgcc version — same reasoning
# as the plain-Makefile version of this project.
target_link_options(ai_ARDUPILOT PRIVATE -static-libgcc -static-libstdc++)

# --- Board-specific tuning -------------------------------------------------
# BOARD is set by you on the command line: -DBOARD=native / rpi64 / rpi32 /
# beaglebone / jetson. This only adds CPU tuning flags (-mcpu, -mfpu) on top
# of whatever compiler the toolchain file already chose — it does NOT
# select the compiler itself.
if(NOT DEFINED BOARD)
    set(BOARD "native")
endif()

message(STATUS "BOARD = ${BOARD}")

# Sanity check: catch a mismatched BOARD/toolchain-file pairing early, with
# a clear message, instead of a confusing compiler error later. This works
# because CMAKE_SYSTEM_PROCESSOR was already set by whichever toolchain
# file you passed in (or left as the host's own architecture if you didn't
# pass one, i.e. a native build).
set(_AARCH64_BOARDS rpi64 jetson)
set(_ARMHF_BOARDS rpi32 beaglebone)

if(BOARD IN_LIST _AARCH64_BOARDS AND NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    message(FATAL_ERROR
        "BOARD=${BOARD} needs the aarch64 toolchain, but CMAKE_SYSTEM_PROCESSOR "
        "is '${CMAKE_SYSTEM_PROCESSOR}'. Did you forget "
        "-DCMAKE_TOOLCHAIN_FILE=toolchains/aarch64.cmake ?")
endif()

if(BOARD IN_LIST _ARMHF_BOARDS AND NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "arm")
    message(FATAL_ERROR
        "BOARD=${BOARD} needs the armhf toolchain, but CMAKE_SYSTEM_PROCESSOR "
        "is '${CMAKE_SYSTEM_PROCESSOR}'. Did you forget "
        "-DCMAKE_TOOLCHAIN_FILE=toolchains/armhf.cmake ?")
endif()

if(BOARD STREQUAL "rpi64")
    # Raspberry Pi 4/5, 64-bit OS — Cortex-A72 (Pi 4) / Cortex-A76 (Pi 5).
    # Cortex-A72 flags run correctly on a Pi 5 too (just under-tuned by a
    # little), so this is a safe default if you're not sure which Pi model.
    target_compile_options(ai_ARDUPILOT PRIVATE -mcpu=cortex-a72)

elseif(BOARD STREQUAL "rpi32")
    # Raspberry Pi, 32-bit OS — Cortex-A7/A53 family depending on model.
    target_compile_options(ai_ARDUPILOT PRIVATE -mcpu=cortex-a53 -mfpu=neon-fp-armv8)

elseif(BOARD STREQUAL "beaglebone")
    # BeagleBone Black/Blue — Cortex-A8, no NEON on the original Black.
    target_compile_options(ai_ARDUPILOT PRIVATE -mcpu=cortex-a8 -mfpu=vfpv3)

elseif(BOARD STREQUAL "jetson")
    # Jetson Nano/Orin — aarch64, generic tuning is safe across the family
    # since Nano (Cortex-A57) and Orin (Carmel/Cortex-A78AE) differ enough
    # that one specific -mcpu would be wrong for the other.
    # No extra flag needed beyond the toolchain's default aarch64 baseline.

elseif(BOARD STREQUAL "native")
    # No special tuning — let the native compiler pick what's safe for
    # whatever CPU is running this VM.

else()
    message(FATAL_ERROR "Unknown BOARD '${BOARD}'. Valid values: native, rpi64, rpi32, beaglebone, jetson")
endif()

# --- Output naming and location --------------------------------------------
# Force the binary into <build-dir>/bin/, regardless of how this build
# directory was created or named. This makes "where did my binary go"
# unambiguous — it's always <build-dir>/bin/ai_ARDUPILOT-<BOARD>, whether
# you ran `cmake -B build/native` or `cd somewhere && cmake /path/to/project`.
set_target_properties(ai_ARDUPILOT PROPERTIES
    OUTPUT_NAME "ai_ARDUPILOT-${BOARD}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
