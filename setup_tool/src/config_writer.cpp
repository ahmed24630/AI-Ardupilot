#include "config_writer.hpp"
#include <fstream>

// Minimal JSON string escaping — sufficient for the simple string values
// this tool writes (paths, model names). Not a general JSON writer.
static std::string json_escape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"' || c == '\\') out += '\\';
        out += c;
    }
    return out;
}

bool write_config(const std::string& output_path,
                   const BoardInfo& board,
                   const SetupChoices& choices) {
    std::ofstream out(output_path);
    if (!out.is_open()) return false;

    out << "{\n";
    out << "  \"_generated_by\": \"ai_pilot_setup C++ tool\",\n";
    out << "  \"board\": {\n";
    out << "    \"type\": \"" << json_escape(board_type_to_string(board.type)) << "\",\n";
    out << "    \"raw_model_string\": \"" << json_escape(board.raw_model_string) << "\",\n";
    out << "    \"kernel_version\": \"" << json_escape(board.kernel_version) << "\",\n";
    out << "    \"architecture\": \"" << json_escape(board.architecture) << "\"\n";
    out << "  },\n";
    out << "  \"mavlink_connection\": \"" << json_escape(choices.mavlink_connection) << "\",\n";
    out << "  \"camera_index\": " << choices.camera_index << ",\n";
    out << "  \"camera_device\": \"" << json_escape(choices.camera_device) << "\",\n";
    out << "  \"whisper_model_size\": \"" << json_escape(choices.whisper_model_size) << "\",\n";
    out << "  \"yolo_model_size\": \"" << json_escape(choices.yolo_model_size) << "\",\n";
    out << "  \"vehicle_type\": \"" << json_escape(vehicle_type_to_string(choices.vehicle_profile.type)) << "\",\n";
    out << "  \"safety_limits\": {\n";
    out << "    \"max_altitude_m\": " << choices.vehicle_profile.max_altitude_m << ",\n";
    out << "    \"max_depth_m\": " << choices.vehicle_profile.max_depth_m << ",\n";
    out << "    \"max_distance_from_home_m\": " << choices.vehicle_profile.max_distance_from_home_m << ",\n";
    out << "    \"max_speed_mps\": " << choices.vehicle_profile.max_speed_mps << ",\n";
    out << "    \"min_battery_percent\": " << choices.vehicle_profile.min_battery_percent << "\n";
    out << "  },\n";
    out << "  \"model_tuning\": {\n";
    out << "    \"model_name\": \"" << json_escape(choices.model_tuning.model_name) << "\",\n";
    out << "    \"temperature\": " << choices.model_tuning.temperature << ",\n";
    out << "    \"top_p\": " << choices.model_tuning.top_p << "\n";
    out << "  }\n";
    out << "}\n";

    return true;
}
