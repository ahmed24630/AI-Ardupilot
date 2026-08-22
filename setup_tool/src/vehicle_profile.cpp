#include "vehicle_profile.hpp"
#include <cstdio>
#include <memory>
#include <array>
#include <sstream>
#include <iostream>

std::string vehicle_type_to_string(VehicleType type) {
    switch (type) {
        case VehicleType::Drone: return "Drone";
        case VehicleType::Rover: return "Rover";
        case VehicleType::ROV:   return "ROV";
        default:                 return "Unknown";
    }
}

VehicleProfile default_profile_for(VehicleType type) {
    VehicleProfile p{};
    p.type = type;

    switch (type) {
        case VehicleType::Drone:
            // Conservative defaults for a first-flight drone setup.
            p.max_altitude_m = 30.0;
            p.max_depth_m = 0.0;                 // not applicable
            p.max_distance_from_home_m = 200.0;
            p.max_speed_mps = 8.0;
            p.min_battery_percent = 20.0;
            break;

        case VehicleType::Rover:
            // No altitude concept; distance and speed matter more, and
            // ground vehicles can typically run batteries lower safely
            // since there's no "falling out of the sky" failure mode.
            p.max_altitude_m = 0.0;              // not applicable
            p.max_depth_m = 0.0;                 // not applicable
            p.max_distance_from_home_m = 500.0;
            p.max_speed_mps = 3.0;
            p.min_battery_percent = 15.0;
            break;

        case VehicleType::ROV:
            // Depth replaces altitude as the critical hard limit; water
            // currents and comms latency argue for a tighter distance cap.
            p.max_altitude_m = 0.0;              // not applicable
            p.max_depth_m = 20.0;
            p.max_distance_from_home_m = 100.0;
            p.max_speed_mps = 1.5;
            p.min_battery_percent = 25.0;         // less margin for error underwater
            break;

        case VehicleType::Unknown:
        default:
            // Safest possible defaults when we don't know the vehicle type.
            p.max_altitude_m = 10.0;
            p.max_depth_m = 5.0;
            p.max_distance_from_home_m = 50.0;
            p.max_speed_mps = 1.0;
            p.min_battery_percent = 30.0;
            break;
    }

    return p;
}

std::vector<std::string> list_installed_ollama_models() {
    std::vector<std::string> models;

    std::array<char, 256> buffer{};
    std::string output;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("ollama list 2>/dev/null", "r"), pclose);
    if (!pipe) return models;  // ollama not installed or not on PATH

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        output += buffer.data();
    }

    // Parse `ollama list` output. Format is a header line (NAME ID SIZE
    // MODIFIED) followed by one line per model; we only need the first
    // whitespace-separated column (the model name).
    std::istringstream stream(output);
    std::string line;
    bool first_line = true;
    while (std::getline(stream, line)) {
        if (first_line) { first_line = false; continue; }  // skip header
        if (line.empty()) continue;
        std::istringstream line_stream(line);
        std::string name;
        line_stream >> name;
        if (!name.empty()) models.push_back(name);
    }

    return models;
}

bool pull_ollama_model(const std::string& tag) {
    std::string cmd = "ollama pull " + tag + " 2>&1";
    std::array<char, 512> buffer{};

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        std::cerr << "Could not run 'ollama pull' — is Ollama installed and on PATH?\n";
        return false;
    }

    // Stream real output live rather than waiting silently — pull can take
    // a while for larger models, and the user should see actual progress.
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        std::cout << buffer.data();
        std::cout.flush();
    }

    int exit_code = pclose(pipe.release());
    return exit_code == 0;
}
