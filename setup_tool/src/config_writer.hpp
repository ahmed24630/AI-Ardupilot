// config_writer.hpp — writes a config.json that the Python AI pilot reads,
// so porting to a new board means "run this tool once, then run the AI
// pilot" instead of hand-editing connection strings.

#pragma once
#include <string>
#include "board_detect.hpp"
#include "device_scan.hpp"
#include "vehicle_profile.hpp"

struct SetupChoices {
    std::string mavlink_connection;   // e.g. "serial:///dev/ttyACM0:57600"
    std::string camera_device;        // e.g. "/dev/video0" or empty if none
    int camera_index = 0;
    std::string whisper_model_size;   // tiny/base/small
    std::string yolo_model_size;      // yolov8n.pt etc.
    VehicleProfile vehicle_profile;
    ModelTuning model_tuning;
};

// Writes config.json to the given path. Returns true on success.
bool write_config(const std::string& output_path,
                   const BoardInfo& board,
                   const SetupChoices& choices);
