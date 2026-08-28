# toolchains/aarch64.cmake — tells CMake to cross-compile for 64-bit ARM.
#
# Usage: pass this on the FIRST cmake call in a fresh build directory:
#   cmake -B build-aarch64 -DCMAKE_TOOLCHAIN_FILE=toolchains/aarch64.cmake -DBOARD=rpi64
#
# CMake reads this file before it does anything else, which is why compiler
# selection has to happen here rather than in CMakeLists.txt itself — by the
# time your CMakeLists.txt runs, CMake has already "locked in" the compiler
# for this build directory.

# What kind of system we're building FOR (the target), not what we're
# building ON (the host, your Ubuntu VM). This is what tells CMake "this is
# a cross-compile" in the first place.
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# The actual cross-compilers (installed via install_toolchains.sh / apt)
set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Where to look for target libraries/headers, if you ever add dependencies
# beyond the standard library (e.g. if you later link against a library
# that needs to be the ARM version, not your VM's x86_64 version).
set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
