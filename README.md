# AI Pilot — grounded AI copilot for drones, rovers, and ROVs

A local, tool-calling AI layer that sits above ArduPilot/PX4 (via MAVLink)
and adds natural-language control, camera perception, and voice commands —
designed to minimize hallucination by only ever acting through a fixed set
of real, verified functions.

## Architecture

```
[Your voice / text commands]
        ↓
[ai_pilot_perception.py]  ← grounded AI layer (Ollama, local LLM)
        ↓ tool calls only
[vehicle_tools.py / sensors.py / perception.py]
        ↓
[MAVSDK → MAVLink → ArduPilot/PX4]   [OpenCV/V4L2]   [sounddevice/ALSA]
        ↓
[Real vehicle: drone, rover, or ROV]
```

## Repo layout

```
.
├── ai_pilot.py                  flight-only AI pilot (no perception)
├── ai_pilot_perception.py       full version: flight + camera + mic
├── vehicle_tools.py             MAVLink tool functions + safety limits
├── sensors.py                   camera/microphone access (V4L2/ALSA)
├── perception.py                local vision (YOLOv8n) + speech (Whisper)
├── README.md                    detailed setup/usage guide
└── setup_tool/                  native C++ board configuration tool
    ├── Makefile
    └── src/
        ├── main.cpp
        ├── board_detect.hpp/.cpp
        ├── device_scan.hpp/.cpp
        ├── vehicle_profile.hpp/.cpp
        └── config_writer.hpp/.cpp
```

See `README.md` for full setup instructions (SITL simulation, real hardware,
dependencies) and `setup_tool/README.md` for the board configuration tool.

## Status

Early-stage / actively developed. Tested in this environment via ArduPilot
SITL simulation and syntax/logic checks; not yet flight-tested on real
hardware. Treat all safety limits as a starting point to review, not a
guarantee.

## Scope

This project is for navigation, monitoring, and general autonomy (search
and rescue, mapping, inspection, delivery-style tasks). It intentionally
does not include and will not be extended to include weapons, targeting,
or payload-release capability.

## License

Add a license of your choice (MIT is common for hobbyist robotics projects)
before making this repo public.
