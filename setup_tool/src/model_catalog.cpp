#include "model_catalog.hpp"

const std::vector<ModelCatalogEntry>& get_model_catalog() {
    static const std::vector<ModelCatalogEntry> catalog = {
        {
            "Llama 3.2 (1B)",
            "llama3.2:1b",
            "Meta's smallest Llama 3.2. Text + tool-calling only. Fastest option, "
            "best for very constrained boards (Pi Zero/3-class, tight RAM).",
            false, true,
            1.3, 2.0,
            "Lightest option / most constrained boards"
        },
        {
            "Llama 3.2 (3B)",
            "llama3.2:3b",
            "Meta's Llama 3.2, good balance of speed and reasoning quality for "
            "a Pi 4/5-class board. Text + tool-calling only, no vision.",
            false, true,
            2.0, 4.0,
            "Recommended default for Pi 4/5, BeagleBone Blue"
        },
        {
            "Llama 3.2 Vision (11B)",
            "llama3.2-vision:11b",
            "Vision-capable Llama 3.2 variant — can process camera frames "
            "directly in the chat model itself, not just via a separate YOLO "
            "detector. Heavier; needs real GPU (Jetson-class) to be usable.",
            true, true,
            7.9, 16.0,
            "Jetson Orin / boards with a real GPU"
        },
        {
            "Qwen 2.5 (3B)",
            "qwen2.5:3b",
            "Alibaba's Qwen 2.5, strong tool-calling reliability at small size. "
            "Text + tool-calling only, no vision.",
            false, true,
            1.9, 4.0,
            "Alternative to Llama 3.2 3B, often stronger at structured tool use"
        },
        {
            "Qwen 2.5 (7B)",
            "qwen2.5:7b",
            "Larger Qwen 2.5 — noticeably better reasoning quality. Fine on CPU "
            "if you have RAM to spare; much better with a GPU.",
            false, true,
            4.7, 8.0,
            "Boards with 8GB+ RAM or a GPU (Jetson)"
        },
        {
            "Qwen 2-VL (7B)",
            "qwen2-vl:7b",
            "Vision-capable Qwen variant. Strong at describing camera scenes "
            "directly. Needs a real GPU to run at usable speed.",
            true, true,
            6.0, 12.0,
            "Jetson-class boards wanting native vision understanding"
        },
        {
            "Gemma 2 (2B)",
            "gemma2:2b",
            "Google's small Gemma 2. Text-only, and tool-calling support is "
            "weaker/less consistent than Llama or Qwen for this project's "
            "tool-use pattern — usable, but test carefully if you pick this.",
            false, true,
            1.6, 3.0,
            "Lightweight alternative; verify tool-calling reliability for your case"
        },
        {
            "Gemma 2 (9B)",
            "gemma2:9b",
            "Larger Gemma 2. Better reasoning than the 2B version but heavier; "
            "same tool-calling caveat as above.",
            false, true,
            5.4, 10.0,
            "Boards with more RAM, if you prefer Gemma's style over Llama/Qwen"
        },
        {
            "Moondream (1.8B)",
            "moondream",
            "Tiny, vision-focused model — very fast image description, but "
            "does NOT support this project's tool-calling architecture. Only "
            "useful as a supplementary describe-what-you-see model, not as "
            "the main AI pilot brain.",
            true, false,
            1.7, 3.0,
            "Supplementary vision-only helper, not a drop-in AI pilot brain"
        },
    };
    return catalog;
}
