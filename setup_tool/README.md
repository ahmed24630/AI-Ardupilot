# ai_pilot_setup — C++ board configuration and porting tool

A native command-line tool that scans real hardware on a Linux board
(Raspberry Pi, BeagleBone Black/Blue, Jetson, or generic) and writes
`config.json`, which `ai_pilot_perception.py` reads on startup. This is the
"porting" step when moving the AI pilot to a new board — instead of
hand-editing Python connection strings, you run this tool once per board.

Nothing here modifies system state — it only reads `/proc`, `/dev`, and
group membership, and writes a single JSON file in the current directory.
Permission fixes (`usermod`) are printed as commands for you to run
yourself with `sudo`, never executed automatically.

## What it checks (all real, nothing assumed)

- **Board identity** — reads `/proc/device-tree/model` (or `/proc/cpuinfo`
  as fallback) to identify Raspberry Pi / BeagleBone Blue / BeagleBone Black
  / Jetson Nano / Jetson Orin / generic Linux
- **Permissions** — checks whether your user is actually in the `dialout`,
  `video`, and `audio` groups (needed for serial, camera, and mic access
  respectively), and prints the exact `usermod` commands to fix any gaps
- **Serial ports** — scans `/dev/ttyACM*`, `/dev/ttyUSB*`, `/dev/ttyS*` for
  a flight controller connection
- **Cameras** — scans `/dev/video*`
- **Audio** — checks ALSA presence and parses `arecord -l` for real capture
  devices
- **Vehicle type** (Drone/Rover/ROV) — sets matching default safety limits
  (altitude for drones, depth for ROVs, speed cap for rovers) which you can
  keep or customize; these are enforced in `vehicle_tools.py` at the code
  level, not just described to the AI
- **AI model selection** — queries `ollama list` for models actually
  installed and lets you pick one, or enter a name to pull later
- **Model tuning (temperature/top_p)** — lower values make the model stick
  closer to tool results instead of embellishing; a "strict/grounded"
  preset (temperature 0.1) is offered as the recommended default for a
  control system
- **Model catalog with real capability info** — a curated list (Llama 3.2,
  Qwen 2.5, Gemma 2, vision-capable variants, etc.) showing download size,
  RAM needs, whether each model supports vision, and whether it supports
  tool-calling (required for this project's architecture). Selecting a
  model that isn't yet installed triggers a real `ollama pull` with live
  progress output.

## Build

Requires g++ with C++17 support (already standard on Pi OS, Ubuntu,
Debian-based BeagleBone/Jetson images). No external libraries needed.

```bash
make
```

## Run

```bash
./ai_pilot_setup
```

Walk through the prompts — it'll show you what it actually found and ask
you to pick/confirm where there's more than one option (e.g. multiple
serial ports or cameras). At the end it writes `config.json` in the current
directory.

## Then run the AI pilot

Put `config.json` in the same directory as `ai_pilot_perception.py` (or
copy it there), then:

```bash
python3 ai_pilot_perception.py
```

It will load your board's settings automatically. If no `config.json` is
present, it falls back to SITL simulation defaults so you can still test
without hardware.

## Re-running on a new board

Copy this tool (source + Makefile) to the new board, `make`, run
`./ai_pilot_setup` again — it detects that board's real hardware fresh and
writes a new `config.json`. Nothing about the tool itself needs to change
between boards; that's the point of doing detection at runtime rather than
hardcoding per-board logic.

## On the "no hallucination" tuning

Temperature/top_p reduce how often the model produces confident-sounding but
made-up output — this is real and worth setting low (the "strict/grounded"
preset does this). But be clear-eyed: **no setting makes hallucination zero.**
The actual reliability comes from the grounded tool-calling architecture
itself (the model can only report what `look_around`/`get_telemetry`/etc.
actually returned) — temperature is a meaningful second layer on top of that,
not a replacement for it.

## Important: no local model handles audio natively

The model catalog's "capabilities" only cover text and vision (camera
images). **Speech input always goes through faster-whisper separately**
(see `perception.py`) — this is true regardless of which chat model you
pick here, because no general-purpose local LLM handles raw audio well.
Vision-capable models (marked "Vision: yes" in the catalog) can process
camera frames directly in addition to the YOLO detector already in this
project — they're not a replacement for the speech pipeline.

## Files

```
ai_pilot_setup/
├── Makefile
└── src/
    ├── main.cpp                interactive CLI flow
    ├── board_detect.hpp/.cpp   identifies the board from real system files
    ├── device_scan.hpp/.cpp    scans real serial/video/audio devices + permissions
    ├── vehicle_profile.hpp/.cpp  per-vehicle-type safety presets + Ollama pull/list
    ├── model_catalog.hpp/.cpp  curated model list with capability info
    └── config_writer.hpp/.cpp  writes config.json for the Python side to read
```
