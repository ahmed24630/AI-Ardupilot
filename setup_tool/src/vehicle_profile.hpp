// vehicle_profile.hpp — vehicle-type presets and AI model selection.
//
// Two separate concerns bundled here:
//  1. Vehicle type (Drone/Rover/ROV) changes which safety limits and system
//     prompt apply — a rover has no altitude limit but needs a max speed;
//     a ROV needs a max depth instead of a max altitude.
//  2. Model tuning (which Ollama model + temperature) affects how prone the
//     model is to confident-sounding but wrong answers. Lower temperature
//     = more deterministic, less prone to creative/invented output — the
//     right direction for a control system. This does not eliminate
//     hallucination (nothing does), but it meaningfully reduces it.

#pragma once
#include <string>
#include <vector>

enum class VehicleType {
    Drone,
    Rover,
    ROV,
    Unknown
};

struct VehicleProfile {
    VehicleType type;
    double max_altitude_m;      // drones: hard ceiling. rovers/ROVs: unused (0)
    double max_depth_m;         // ROVs: hard depth limit. others: unused (0)
    double max_distance_from_home_m;
    double max_speed_mps;       // rovers especially; others can still cap it
    double min_battery_percent;
};

struct ModelTuning {
    std::string model_name;     // e.g. "llama3.2", "qwen2.5:3b"
    double temperature;         // 0.0-1.0; lower = more deterministic/grounded
    double top_p;               // nucleus sampling cutoff; lower = more focused
};

std::string vehicle_type_to_string(VehicleType type);
VehicleProfile default_profile_for(VehicleType type);

// Queries `ollama list` for REAL installed models — never assumes a model
// is present. Returns empty vector if ollama isn't installed or has no
// models pulled yet.
std::vector<std::string> list_installed_ollama_models();

// Runs `ollama pull <tag>` and streams its real progress output live to
// stdout as it downloads, so the user sees actual download progress rather
// than a silent wait. Returns true if the command exited successfully.
bool pull_ollama_model(const std::string& tag);
