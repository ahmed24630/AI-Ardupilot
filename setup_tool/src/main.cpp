// main.cpp — ai_pilot_setup: interactive board configuration and porting tool.
//
// Purpose: instead of a user hand-editing Python connection strings for each
// new board, this tool scans the REAL hardware present (board type, serial
// ports, cameras, mic, permissions), asks the user to confirm choices where
// there's ambiguity, and writes config.json for ai_pilot_perception.py to
// load. Every piece of information shown is read from the real system —
// nothing here is guessed or assumed.

#include <iostream>
#include <iomanip>
#include <string>
#include <limits>
#include <algorithm>
#include <cstdlib>
#include "board_detect.hpp"
#include "device_scan.hpp"
#include "config_writer.hpp"
#include "vehicle_profile.hpp"
#include "model_catalog.hpp"

static void print_header(const std::string& title) {
    std::cout << "\n=== " << title << " ===\n";
}

static int prompt_choice(const std::string& question, int max_valid) {
    int choice = -1;
    while (true) {
        std::cout << question << " ";
        if (std::cin >> choice && choice >= 1 && choice <= max_valid) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return choice;
        }
        if (std::cin.eof()) {
            std::cerr << "\nInput ended unexpectedly. Exiting.\n";
            std::exit(1);
        }
        std::cout << "Invalid choice, try again.\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

// Safely reads a double, re-prompting on invalid input instead of silently
// leaving the target variable unset (which caused a real bug: a bad
// keystroke here used to corrupt every subsequent numeric field).
static double prompt_double(const std::string& question) {
    double value;
    while (true) {
        std::cout << question;
        if (std::cin >> value) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }
        if (std::cin.eof()) {
            std::cerr << "\nInput ended unexpectedly. Exiting.\n";
            std::exit(1);
        }
        std::cout << "Invalid number, try again.\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

int main() {
    std::cout << "AI Pilot Setup — board configuration and porting tool\n";
    std::cout << "This scans your real hardware and writes config.json for the AI pilot.\n";

    // -------------------------------------------------------------
    // 1. Board identification
    // -------------------------------------------------------------
    print_header("Board Identification");
    BoardInfo board = detect_board();
    std::cout << "Detected board:  " << board_type_to_string(board.type) << "\n";
    std::cout << "Model string:    " << board.raw_model_string << "\n";
    std::cout << "Kernel:          " << board.kernel_version << "\n";
    std::cout << "Architecture:    " << board.architecture << "\n";

    if (board.type == BoardType::Unknown || board.type == BoardType::GenericLinux) {
        std::cout << "\nNote: board not specifically recognized. Setup will still work,\n"
                     "but double-check device paths below carefully.\n";
    }

    // -------------------------------------------------------------
    // 2. Permission check
    // -------------------------------------------------------------
    print_header("Permission Check");
    PermissionCheck perms = check_permissions();
    std::cout << "User: " << perms.username << "\n";
    std::cout << "  dialout (serial/USB access): " << (perms.in_dialout_group ? "OK" : "MISSING") << "\n";
    std::cout << "  video (camera access):       " << (perms.in_video_group ? "OK" : "MISSING") << "\n";
    std::cout << "  audio (microphone access):   " << (perms.in_audio_group ? "OK" : "MISSING") << "\n";

    if (!perms.in_dialout_group || !perms.in_video_group || !perms.in_audio_group) {
        std::cout << "\nTo fix missing permissions, run:\n";
        if (!perms.in_dialout_group) std::cout << "  sudo usermod -a -G dialout " << perms.username << "\n";
        if (!perms.in_video_group)   std::cout << "  sudo usermod -a -G video "   << perms.username << "\n";
        if (!perms.in_audio_group)   std::cout << "  sudo usermod -a -G audio "   << perms.username << "\n";
        std::cout << "Then log out and back in, and re-run this tool.\n";
    }

    // -------------------------------------------------------------
    // 3. Serial port scan (for MAVLink connection)
    // -------------------------------------------------------------
    print_header("Serial Ports (flight controller connection)");
    auto serial_ports = scan_serial_ports();
    std::string mavlink_connection;

    if (serial_ports.empty()) {
        std::cout << "No serial devices found (no /dev/ttyACM*, /dev/ttyUSB*, /dev/ttyS*).\n";
        std::cout << "If your flight controller is connected, check the USB cable and re-run.\n";
        std::cout << "You can still proceed and manually set the connection string later,\n";
        std::cout << "or use SITL simulation.\n";
        int choice = prompt_choice("Use SITL simulation connection instead? (1=yes, 2=enter manually)", 2);
        if (choice == 1) {
            mavlink_connection = "udp://:14540";
        } else {
            std::cout << "Enter MAVLink connection string: ";
            std::getline(std::cin, mavlink_connection);
        }
    } else {
        for (size_t i = 0; i < serial_ports.size(); ++i) {
            std::cout << "  [" << (i + 1) << "] " << serial_ports[i].path
                      << (serial_ports[i].accessible ? " (accessible)" : " (NO PERMISSION — fix above first)")
                      << "\n";
        }
        std::cout << "  [" << (serial_ports.size() + 1) << "] Use SITL simulation instead (udp://:14540)\n";
        std::cout << "  [" << (serial_ports.size() + 2) << "] Enter a connection string manually (e.g. Ethernet/TCP)\n";

        int choice = prompt_choice("Select flight controller connection:", (int)serial_ports.size() + 2);
        if (choice <= (int)serial_ports.size()) {
            mavlink_connection = "serial://" + serial_ports[choice - 1].path + ":57600";
        } else if (choice == (int)serial_ports.size() + 1) {
            mavlink_connection = "udp://:14540";
        } else {
            std::cout << "Enter MAVLink connection string (e.g. tcp://192.168.1.50:5760): ";
            std::getline(std::cin, mavlink_connection);
        }
    }

    // -------------------------------------------------------------
    // 4. Camera scan
    // -------------------------------------------------------------
    print_header("Cameras");
    auto video_devices = scan_video_devices();
    std::string camera_device;
    int camera_index = 0;

    if (video_devices.empty()) {
        std::cout << "No /dev/video* devices found. Camera tools will be unavailable\n";
        std::cout << "until a camera is connected.\n";
    } else {
        for (size_t i = 0; i < video_devices.size(); ++i) {
            std::cout << "  [" << (i + 1) << "] " << video_devices[i].path
                      << (video_devices[i].accessible ? " (accessible)" : " (NO PERMISSION — fix above first)")
                      << "\n";
        }
        int choice = prompt_choice("Select camera to use:", (int)video_devices.size());
        camera_device = video_devices[choice - 1].path;

        // Extract the trailing number from a path like /dev/video0 -> 0
        size_t last_digit_start = camera_device.find_last_not_of("0123456789");
        std::string index_str = camera_device.substr(last_digit_start + 1);
        camera_index = index_str.empty() ? 0 : std::stoi(index_str);
    }

    // -------------------------------------------------------------
    // 5. Audio scan
    // -------------------------------------------------------------
    print_header("Audio Input");
    AudioCheck audio = scan_audio_devices();
    if (!audio.alsa_present) {
        std::cout << "ALSA not detected on this system (/proc/asound missing).\n";
    } else if (audio.capture_devices.empty()) {
        std::cout << "ALSA present but no capture devices found via 'arecord -l'.\n";
        std::cout << "Check that a microphone is connected.\n";
    } else {
        std::cout << "Capture devices found:\n";
        for (const auto& dev : audio.capture_devices) {
            std::cout << "  " << dev << "\n";
        }
    }

    // -------------------------------------------------------------
    // 6. Vehicle type selection (changes safety limits + system prompt)
    // -------------------------------------------------------------
    print_header("Vehicle Type");
    std::cout << "This determines which safety limits apply (e.g. altitude for\n";
    std::cout << "drones, depth for ROVs, speed cap for rovers).\n\n";
    std::cout << "  [1] Drone (aerial)\n";
    std::cout << "  [2] Rover (ground)\n";
    std::cout << "  [3] ROV (underwater)\n";
    int vtype_choice = prompt_choice("Select vehicle type:", 3);
    VehicleType vtype = vtype_choice == 1 ? VehicleType::Drone
                      : vtype_choice == 2 ? VehicleType::Rover
                                          : VehicleType::ROV;

    VehicleProfile profile = default_profile_for(vtype);
    std::cout << "\nDefault safety limits for " << vehicle_type_to_string(vtype) << ":\n";
    if (profile.max_altitude_m > 0) std::cout << "  max altitude:     " << profile.max_altitude_m << " m\n";
    if (profile.max_depth_m > 0)    std::cout << "  max depth:        " << profile.max_depth_m << " m\n";
    std::cout << "  max distance:     " << profile.max_distance_from_home_m << " m from home\n";
    std::cout << "  max speed:        " << profile.max_speed_mps << " m/s\n";
    std::cout << "  min battery:      " << profile.min_battery_percent << "%\n";

    int keep_defaults = prompt_choice("\nUse these defaults? (1=yes, 2=customize)", 2);
    if (keep_defaults == 2) {
        profile.max_distance_from_home_m = prompt_double("Enter max distance from home (meters): ");
        profile.max_speed_mps = prompt_double("Enter max speed (m/s): ");
        profile.min_battery_percent = prompt_double("Enter min battery percent to allow arming: ");
        if (profile.max_altitude_m > 0) {
            profile.max_altitude_m = prompt_double("Enter max altitude (meters): ");
        }
        if (profile.max_depth_m > 0) {
            profile.max_depth_m = prompt_double("Enter max depth (meters): ");
        }
    }

    // -------------------------------------------------------------
    // 7. AI model selection and tuning
    // -------------------------------------------------------------
    print_header("AI Model Catalog");
    std::cout << "Note: none of these models handle audio directly — spoken\n";
    std::cout << "commands always go through faster-whisper separately, regardless\n";
    std::cout << "of which model you pick here. \"Vision\" below means the model can\n";
    std::cout << "natively process camera images itself (in addition to the YOLO\n";
    std::cout << "object detector this project already uses).\n\n";

    const auto& catalog = get_model_catalog();
    for (size_t i = 0; i < catalog.size(); ++i) {
        const auto& m = catalog[i];
        std::cout << "  [" << (i + 1) << "] " << m.display_name
                   << " (" << m.ollama_tag << ")\n";
        std::cout << "        " << m.description << "\n";
        std::cout << "        Vision: " << (m.supports_vision ? "yes" : "no")
                   << "  |  Tool-calling: " << (m.supports_tool_calling ? "yes" : "no")
                   << "  |  ~" << m.approx_size_gb << "GB download"
                   << "  |  recommends ~" << m.min_ram_recommended_gb << "GB+ RAM\n";
        std::cout << "        Best for: " << m.best_for << "\n\n";
    }

    auto installed_models = list_installed_ollama_models();
    int already_installed_option = (int)catalog.size() + 1;
    int manual_entry_option = (int)catalog.size() + 2;

    std::cout << "  [" << already_installed_option << "] Choose from models already installed on this board";
    if (!installed_models.empty()) {
        std::cout << " (" << installed_models.size() << " found)";
    }
    std::cout << "\n";
    std::cout << "  [" << manual_entry_option << "] Enter a custom Ollama model tag manually\n";

    int model_choice = prompt_choice("\nSelect a model:", manual_entry_option);

    ModelTuning tuning;
    bool needs_pull = false;

    if (model_choice <= (int)catalog.size()) {
        const auto& selected = catalog[model_choice - 1];
        tuning.model_name = selected.ollama_tag;

        if (!selected.supports_tool_calling) {
            std::cout << "\nWarning: " << selected.display_name << " does not support "
                      << "tool-calling, which this project's AI pilot architecture "
                      << "requires (it's how the AI calls flight/perception functions "
                      << "instead of just talking). This model will likely not work "
                      << "correctly as the main pilot brain.\n";
            int proceed = prompt_choice("Proceed anyway? (1=yes, 2=pick a different model)", 2);
            if (proceed == 2) {
                std::cout << "Re-run the tool to pick again.\n";
                return 1;
            }
        }

        bool already_have_it = std::find(installed_models.begin(), installed_models.end(),
                                          selected.ollama_tag) != installed_models.end();
        needs_pull = !already_have_it;

    } else if (model_choice == already_installed_option) {
        if (installed_models.empty()) {
            std::cout << "No models currently installed. Enter a tag to pull manually: ";
            std::getline(std::cin, tuning.model_name);
        } else {
            for (size_t i = 0; i < installed_models.size(); ++i) {
                std::cout << "  [" << (i + 1) << "] " << installed_models[i] << "\n";
            }
            int choice = prompt_choice("Select installed model:", (int)installed_models.size());
            tuning.model_name = installed_models[choice - 1];
        }
    } else {
        std::cout << "Enter Ollama model tag (e.g. mistral:7b): ";
        std::getline(std::cin, tuning.model_name);
        needs_pull = true;  // unknown to us whether it's installed; try pulling, ollama no-ops if already present
    }

    if (needs_pull) {
        std::cout << "\nDownloading " << tuning.model_name << " via 'ollama pull'...\n";
        bool ok = pull_ollama_model(tuning.model_name);
        if (!ok) {
            std::cout << "\nDownload failed or Ollama isn't installed/running. "
                       "You can pull it manually later with:\n";
            std::cout << "  ollama pull " << tuning.model_name << "\n";
            std::cout << "Continuing setup — config.json will reference this model name "
                       "regardless, so it'll work once pulled.\n";
        } else {
            std::cout << "Download complete.\n";
        }
    }

    std::cout << "\nTemperature controls how deterministic vs. creative responses are.\n";
    std::cout << "For a control system, LOWER is safer — it makes the model stick\n";
    std::cout << "closely to tool results instead of embellishing. This does not\n";
    std::cout << "eliminate hallucination entirely, but reduces it meaningfully.\n\n";
    std::cout << "  [1] Strict/grounded (temperature 0.1) — recommended for this use case\n";
    std::cout << "  [2] Balanced (temperature 0.4)\n";
    std::cout << "  [3] Enter custom value\n";
    int temp_choice = prompt_choice("Select tuning preset:", 3);
    if (temp_choice == 1) {
        tuning.temperature = 0.1;
        tuning.top_p = 0.85;
    } else if (temp_choice == 2) {
        tuning.temperature = 0.4;
        tuning.top_p = 0.9;
    } else {
        tuning.temperature = prompt_double("Enter temperature (0.0-1.0, lower = more deterministic): ");
        tuning.top_p = prompt_double("Enter top_p (0.0-1.0, lower = more focused): ");
    }

    // -------------------------------------------------------------
    // 8. Local perception model sizes (based on detected board's compute)
    // -------------------------------------------------------------
    print_header("Local Perception Model Sizes");
    std::string suggested_whisper = "base";
    std::string suggested_yolo = "yolov8n.pt";
    if (board.type == BoardType::JetsonOrin || board.type == BoardType::JetsonNano) {
        std::cout << "Jetson detected — GPU acceleration available, can use larger models if desired.\n";
        suggested_yolo = "yolov8s.pt";
    } else {
        std::cout << "CPU-only board assumed — suggesting smallest/fastest models.\n";
    }
    std::cout << "Suggested whisper model: " << suggested_whisper << "\n";
    std::cout << "Suggested YOLO model:    " << suggested_yolo << "\n";
    std::cout << "(These can be changed later by editing config.json directly.)\n";

    // -------------------------------------------------------------
    // 7. Write config
    // -------------------------------------------------------------
    print_header("Writing Configuration");
    SetupChoices choices;
    choices.mavlink_connection = mavlink_connection;
    choices.camera_device = camera_device;
    choices.camera_index = camera_index;
    choices.whisper_model_size = suggested_whisper;
    choices.yolo_model_size = suggested_yolo;
    choices.vehicle_profile = profile;
    choices.model_tuning = tuning;

    std::string output_path = "config.json";
    if (write_config(output_path, board, choices)) {
        std::cout << "Wrote " << output_path << " successfully.\n";
        std::cout << "\nNext step: run the Python AI pilot, which will read this config:\n";
        std::cout << "  python3 ai_pilot_perception.py\n";
    } else {
        std::cerr << "Failed to write " << output_path << " — check write permissions in this directory.\n";
        return 1;
    }

    return 0;
}
