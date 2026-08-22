// device_scan.hpp — enumerate REAL connected devices, never assumed ones.
//
// Checks actual /dev entries and actual group membership — the same
// grounding principle as the rest of this project applies to the setup
// tool too: report what's actually there, flag what's actually missing.

#pragma once
#include <string>
#include <vector>

struct SerialPort {
    std::string path;       // e.g. /dev/ttyACM0
    bool accessible;        // can the current user actually open it?
};

struct VideoDevice {
    std::string path;       // e.g. /dev/video0
    bool accessible;
};

struct AudioCheck {
    bool alsa_present;      // /proc/asound exists
    std::vector<std::string> capture_devices;  // from arecord -l parsing
};

struct PermissionCheck {
    std::string username;
    bool in_dialout_group;  // needed for serial/USB access
    bool in_video_group;    // needed for camera access
    bool in_audio_group;    // needed for microphone access
};

std::vector<SerialPort> scan_serial_ports();
std::vector<VideoDevice> scan_video_devices();
AudioCheck scan_audio_devices();
PermissionCheck check_permissions();
