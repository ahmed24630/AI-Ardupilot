// board_detect.hpp — identify what Linux board this is running on.
//
// Uses standard Linux-provided identification files rather than guessing —
// /proc/device-tree/model exists on Pi/BeagleBone/Jetson (ARM boards),
// /proc/cpuinfo is a fallback. Never assumes a board type without evidence.

#pragma once
#include <string>

enum class BoardType {
    RaspberryPi,
    BeagleBoneBlue,
    BeagleBoneBlack,
    JetsonNano,
    JetsonOrin,
    GenericLinux,
    Unknown
};

struct BoardInfo {
    BoardType type;
    std::string raw_model_string;
    std::string kernel_version;
    std::string architecture;
};

// Reads real system files to identify the board. Returns Unknown (not a
// guess) if identification files aren't present or don't match known boards.
BoardInfo detect_board();

std::string board_type_to_string(BoardType type);
