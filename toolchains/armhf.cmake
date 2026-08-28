# toolchains/armhf.cmake — tells CMake to cross-compile for 32-bit ARM
# (hard-float ABI, used by Raspberry Pi's 32-bit OS and BeagleBone).
#
# Usage:
#   cmake -B build-armhf -DCMAKE_TOOLCHAIN_FILE=toolchains/armhf.cmake -DBOARD=armhf

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER   arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

set(CMAKE_FIND_ROOT_PATH /usr/arm-linux-gnueabihf)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
