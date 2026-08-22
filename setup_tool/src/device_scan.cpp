#include "device_scan.hpp"
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <memory>
#include <array>
#include <unistd.h>
#include <grp.h>
#include <pwd.h>

namespace fs = std::filesystem;

// Runs a shell command and captures stdout — used only for read-only,
// standard Linux introspection commands (arecord -l), never for anything
// that modifies system state.
static std::string run_command(const std::string& cmd) {
    std::array<char, 256> buffer{};
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen((cmd + " 2>/dev/null").c_str(), "r"), pclose);
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

static bool path_accessible(const std::string& path) {
    return access(path.c_str(), R_OK | W_OK) == 0;
}

std::vector<SerialPort> scan_serial_ports() {
    std::vector<SerialPort> ports;
    if (!fs::exists("/dev")) return ports;

    for (const auto& entry : fs::directory_iterator("/dev")) {
        std::string name = entry.path().filename().string();
        // Common patterns for USB-serial / ACM flight controller devices
        bool is_candidate =
            name.rfind("ttyACM", 0) == 0 ||   // most Pixhawk/Cube boards over USB
            name.rfind("ttyUSB", 0) == 0 ||   // USB-to-serial adapters, telemetry radios
            name.rfind("ttyS", 0) == 0;       // hardware UARTs (e.g. GPIO serial)

        if (is_candidate) {
            ports.push_back({entry.path().string(), path_accessible(entry.path().string())});
        }
    }
    return ports;
}

std::vector<VideoDevice> scan_video_devices() {
    std::vector<VideoDevice> devices;
    if (!fs::exists("/dev")) return devices;

    for (const auto& entry : fs::directory_iterator("/dev")) {
        std::string name = entry.path().filename().string();
        if (name.rfind("video", 0) == 0) {
            devices.push_back({entry.path().string(), path_accessible(entry.path().string())});
        }
    }
    return devices;
}

AudioCheck scan_audio_devices() {
    AudioCheck check;
    check.alsa_present = fs::exists("/proc/asound");

    std::string output = run_command("arecord -l");
    // Parse lines like: "card 1: Device [USB Audio Device], device 0: ..."
    std::istringstream stream(output);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.rfind("card", 0) == 0) {
            check.capture_devices.push_back(line);
        }
    }
    return check;
}

PermissionCheck check_permissions() {
    PermissionCheck result;

    uid_t uid = getuid();
    struct passwd* pw = getpwuid(uid);
    result.username = pw ? pw->pw_name : "(unknown)";

    result.in_dialout_group = false;
    result.in_video_group = false;
    result.in_audio_group = false;

    int ngroups = 64;
    std::vector<gid_t> groups(ngroups);
    if (pw && getgrouplist(pw->pw_name, pw->pw_gid, groups.data(), &ngroups) >= 0) {
        groups.resize(ngroups);
        for (gid_t gid : groups) {
            struct group* gr = getgrgid(gid);
            if (!gr) continue;
            std::string gname = gr->gr_name;
            if (gname == "dialout") result.in_dialout_group = true;
            if (gname == "video") result.in_video_group = true;
            if (gname == "audio") result.in_audio_group = true;
        }
    }

    return result;
}
