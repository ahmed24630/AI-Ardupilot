# CMakeLists.txt — builds ai_ARDUPILOT, reading sources from sources.txt
# and applying board-specific compiler options via the BOARD variable.
#
# IMPORTANT CMake concept: the compiler itself (native g++ vs a cross
# compiler) is chosen by a TOOLCHAIN FILE, passed on the first `cmake` call
# in a given build directory — not by a variable read down here. By the
# time this file runs, that choice is already locked in. BOARD here only
# controls *extra tuning flags* (like -mcpu) on top of whichever compiler
# was already selected. See README.md for why.

cmake_minimum_required(VERSION 3.22.1)
project(ai_ARDUPILOT)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Read the source file list from sources.txt --------------------------
file(STRINGS "${CMAKE_SOURCE_DIR}/sources.txt" RAW_SOURCE_LINES)

set(PROJECT_SOURCES "")
foreach(LINE ${RAW_SOURCE_LINES})
    string(STRIP "${LINE}" TRIMMED_LINE)
    if(TRIMMED_LINE STREQUAL "")
        continue()
    endif()
    if(TRIMMED_LINE MATCHES "^#")
        continue()
    endif()
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
target_include_directories(ai_ARDUPILOT PRIVATE ${CMAKE_SOURCE_DIR}/Src/Config)

# --- MAVSDK integration -----------------------------------------------------
# MAVSDK is pulled in as a git submodule at Src/MAVSDK (see .gitmodules).
# These options must be set BEFORE add_subdirectory() runs: MAVSDK's own
# CMakeLists.txt uses option(...), which only takes effect the FIRST time a
# cache variable is set — pre-setting it here with CACHE + FORCE means our
# value wins instead of MAVSDK's own default.
#
# Confirmed directly from Src/MAVSDK/cpp/third_party/CMakeLists.txt:
#   - grpc, absl, protobuf, re2, c-ares only build when BUILD_MAVSDK_SERVER is on
#   - openssl, zlib-ng, curl only build when BUILD_WITHOUT_CURL is off
#   - gtest only builds when BUILD_TESTING is on
# With the settings below, ALL of those are skipped — the only third-party
# deps actually built are: fmt, asio, mavlink (headers), libevents,
# picosha2, tinyxml2, libmavlike, nlohmann_json, liblzma.
set(BUILD_MAVSDK_SERVER OFF CACHE BOOL "Disable gRPC backend - not needed for pure C++ use" FORCE)
set(BUILD_WITHOUT_CURL ON CACHE BOOL "Disable curl/openssl/zlib - not downloading camera definitions over HTTP" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Skip MAVSDK's own test suite - not needed on target boards" FORCE)
set(SUPERBUILD ON CACHE BOOL "Build MAVSDK's own lightweight deps from source" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Static linking - simpler to deploy to boards" FORCE)

add_subdirectory(${CMAKE_SOURCE_DIR}/Src/MAVSDK/cpp)

target_link_libraries(ai_ARDUPILOT PRIVATE mavsdk)

# --- Common flags for every target ----------------------------------------
target_compile_options(ai_ARDUPILOT PRIVATE -Wall -Wextra)

# --- Debug vs Release build type -------------------------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(ai_ARDUPILOT PRIVATE -g -O0)
else()
    target_compile_options(ai_ARDUPILOT PRIVATE -O2)
endif()

target_link_options(ai_ARDUPILOT PRIVATE -static-libgcc -static-libstdc++)

# --- Board-specific tuning -------------------------------------------------
if(NOT DEFINED BOARD)
    set(BOARD "native")
endif()

message(STATUS "BOARD = ${BOARD}")

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
    target_compile_options(ai_ARDUPILOT PRIVATE -mcpu=cortex-a72)
elseif(BOARD STREQUAL "rpi32")
    target_compile_options(ai_ARDUPILOT PRIVATE -mcpu=cortex-a53 -mfpu=neon-fp-armv8)
elseif(BOARD STREQUAL "beaglebone")
    target_compile_options(ai_ARDUPILOT PRIVATE -mcpu=cortex-a8 -mfpu=vfpv3)
elseif(BOARD STREQUAL "jetson")
    # No extra flag needed beyond the toolchain's default aarch64 baseline.
elseif(BOARD STREQUAL "native")
    # No special tuning.
else()
    message(FATAL_ERROR "Unknown BOARD '${BOARD}'. Valid values: native, rpi64, rpi32, beaglebone, jetson")
endif()

# --- Output naming and location --------------------------------------------
set_target_properties(ai_ARDUPILOT PROPERTIES
    OUTPUT_NAME "ai_ARDUPILOT-${BOARD}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)