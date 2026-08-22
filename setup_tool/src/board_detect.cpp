#include "board_detect.hpp"
#include <fstream>
#include <sstream>
#include <sys/utsname.h>
#include <algorithm>
#include <cctype>

static std::string read_file_trimmed(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::stringstream ss;
    ss << f.rdbuf();
    std::string content = ss.str();
    // Strip trailing null bytes / whitespace some device-tree files include
    while (!content.empty() && (content.back() == '\0' || std::isspace((unsigned char)content.back()))) {
        content.pop_back();
    }
    return content;
}

static std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

BoardInfo detect_board() {
    BoardInfo info;
    info.type = BoardType::Unknown;

    // Primary source: device-tree model string (present on ARM SBCs)
    std::string model = read_file_trimmed("/proc/device-tree/model");

    // Fallback: /proc/cpuinfo "Hardware"/"Model" line (older kernels, some boards)
    if (model.empty()) {
        std::ifstream cpuinfo("/proc/cpuinfo");
        std::string line;
        while (std::getline(cpuinfo, line)) {
            if (line.find("Model") != std::string::npos || line.find("Hardware") != std::string::npos) {
                auto pos = line.find(':');
                if (pos != std::string::npos) {
                    model = line.substr(pos + 1);
                    // trim leading space
                    while (!model.empty() && model.front() == ' ') model.erase(model.begin());
                    break;
                }
            }
        }
    }

    info.raw_model_string = model.empty() ? "(unknown — no device-tree or cpuinfo model string found)" : model;

    std::string lower_model = to_lower(model);
    if (lower_model.find("raspberry pi") != std::string::npos) {
        info.type = BoardType::RaspberryPi;
    } else if (lower_model.find("beaglebone blue") != std::string::npos) {
        info.type = BoardType::BeagleBoneBlue;
    } else if (lower_model.find("beaglebone") != std::string::npos) {
        info.type = BoardType::BeagleBoneBlack;
    } else if (lower_model.find("jetson orin") != std::string::npos) {
        info.type = BoardType::JetsonOrin;
    } else if (lower_model.find("jetson nano") != std::string::npos || lower_model.find("jetson") != std::string::npos) {
        info.type = BoardType::JetsonNano;
    } else if (!model.empty()) {
        info.type = BoardType::GenericLinux;
    }

    struct utsname uts{};
    if (uname(&uts) == 0) {
        info.kernel_version = uts.release;
        info.architecture = uts.machine;
    } else {
        info.kernel_version = "(unavailable)";
        info.architecture = "(unavailable)";
    }

    return info;
}

std::string board_type_to_string(BoardType type) {
    switch (type) {
        case BoardType::RaspberryPi:      return "Raspberry Pi";
        case BoardType::BeagleBoneBlue:   return "BeagleBone Blue";
        case BoardType::BeagleBoneBlack:  return "BeagleBone Black";
        case BoardType::JetsonNano:       return "Jetson Nano";
        case BoardType::JetsonOrin:       return "Jetson Orin";
        case BoardType::GenericLinux:     return "Generic Linux board (unrecognized model)";
        case BoardType::Unknown:
        default:                          return "Unknown";
    }
}
