// model_catalog.hpp — curated catalog of local models available through
// Ollama, with honest capability info per model.
//
// Important accuracy note: "capabilities" here means what the CHAT MODEL
// itself can natively process. None of these models handle audio directly —
// speech input in this project always goes through faster-whisper
// (see perception.py), regardless of which model is selected here. Only
// vision (image) input is a real native capability some of these models have.

#pragma once
#include <string>
#include <vector>

struct ModelCatalogEntry {
    std::string display_name;
    std::string ollama_tag;        // the actual tag passed to `ollama pull`
    std::string description;
    bool supports_vision;          // can natively process image input
    bool supports_tool_calling;    // required for this project's tool-use architecture
    double approx_size_gb;
    double min_ram_recommended_gb;
    std::string best_for;          // short guidance on hardware/use case fit
};

// Static, curated list — not queried from the internet, so it stays usable
// offline. Update this list by hand as new models become worth including.
const std::vector<ModelCatalogEntry>& get_model_catalog();
