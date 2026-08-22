# AI Pilot — general framework for AI-controlled drone/rover/ROV

## Architecture

```
[Your AI layer: ai_pilot.py]
        ↕  (grounded tool calls only — see vehicle_tools.py)
[MAVSDK]
        ↕  MAVLink protocol
[ArduPilot or PX4 firmware]  ← real, battle-tested safety-critical flight control
        ↕
[Simulator (SITL) now  →  real vehicle later]
```

You are NOT replacing ArduPilot/PX4's flight control — that stays as the
safety-critical layer. Your AI only sends high-level commands (goto, land,
RTL) and reads telemetry. This is the same pattern used in real
companion-computer setups (search-and-rescue drones, survey rovers, etc.).

Because everything goes through MAVLink, **the same code works for Copter
(drone), Rover, or Sub (ROV)** — you just point it at a different SITL
vehicle type or a different real flight controller. Nothing in `ai_pilot.py`
or `vehicle_tools.py` needs to change.

## Step 1 — Simulate before touching real hardware

**ArduPilot SITL** runs the actual ArduPilot firmware against a physics
simulator — no hardware needed, and it behaves like the real thing.

```bash
# Clone ArduPilot (Linux/macOS — for Windows, use WSL2)
git clone --recurse-submodules https://github.com/ArduPilot/ardupilot.git
cd ardupilot
Tools/environment_install/install-prereqs-ubuntu.sh -y   # or the mac equivalent script
. ~/.profile

# Run a simulated Copter (drone)
sim_vehicle.py -v ArduCopter --console --map

# Or a simulated Rover
sim_vehicle.py -v Rover --console --map

# Or a simulated Sub (ROV)
sim_vehicle.py -v ArduSub --console --map
```

This starts SITL listening on `udp://:14540` by default — exactly what
`vehicle_tools.py` connects to out of the box.

## Step 2 — Install dependencies

```bash
pip install mavsdk ollama
ollama pull llama3.2
```

## Step 3 — Run the AI pilot against the simulation

```bash
python ai_pilot.py
```

Try:
```
You: what's the current status?
You: take off to 10 meters
Pilot: I need you to confirm — should I arm and take off to 10m? (yes/no)
You: yes, confirmed
```

Notice it won't arm/take off without you explicitly saying so — that's
enforced in code (`vehicle_tools.py`), not just requested in the prompt, so
the AI can't talk its way around it.

## Step 4 — Real hardware, when you're ready

Change the connection string in `VehicleController()`:
```python
VehicleController("serial:///dev/ttyACM0")   # USB-connected flight controller
# or
VehicleController("udp://:14550")            # telemetry radio link
```

Any board running Linux works as the companion computer — Raspberry Pi,
Jetson (adds GPU for vision models), or a generic mini-PC. Install the same
Python dependencies there.

## Safety design (please don't remove these)

- **Hard-coded limits** in `SafetyLimits` (altitude, battery) — enforced in
  code, so the AI cannot override them by being convinced or "clever."
- **Confirmation required** for arm/takeoff — the AI cannot self-confirm.
- **`return_to_launch` and `hold`** are always available as an escape hatch.
- **Grounded telemetry** — the AI only knows what `get_telemetry()` actually
  returns, never assumed state.

If you extend this with a geofence (recommended before real flight), add it
as another hard check inside `goto_position`/`takeoff` in `vehicle_tools.py`,
not as a prompt instruction — code enforcement, not the AI's judgment, is
what should stop unsafe actions.

## What I won't help extend this into
Payload release mechanisms, weapons targeting, or any lethal/offensive
capability — regardless of framing. Everything above is scoped to
navigation, monitoring, and general autonomy (search-and-rescue, mapping,
inspection, delivery), which covers the large majority of real-world
companion-computer use cases.

## Perception layer (camera + microphone)

`sensors.py` + `perception.py` + `ai_pilot_perception.py` add local, offline
camera and audio understanding on top of the flight core — same grounded
architecture: the AI only knows what these tools actually detected.

**Install:**
```bash
pip install opencv-python sounddevice ultralytics faster-whisper numpy
```

**Linux driver permissions** (needed once per board — Pi, BeagleBone, Jetson,
or any generic Linux board, since this goes through standard V4L2/ALSA):
```bash
sudo usermod -a -G video $USER
sudo usermod -a -G audio $USER
# log out and back in for group membership to take effect
```

**Run:**
```bash
python ai_pilot_perception.py
```

This prints the real cameras/microphones detected on the board, then gives
you a chat loop with two new tools alongside flight control:
```
You: what do you see?
  [executing: look_around({})]
  [result: {'success': True, 'detections': [{'label': 'person', 'confidence': 0.87, ...}], 'count': 1}]
Pilot: I can see one person in view.

You: listen for my command
  [listening for 4.0s...]
Pilot: I heard: "return to launch point"
```

**Performance notes for Pi-class hardware:**
- Vision: `yolov8n.pt` (nano) is the right choice — larger models will be too
  slow on CPU. On a Jetson, `yolov8s.pt` runs comfortably with GPU acceleration.
- Speech: `WhisperModel("base", ...)` is a good default; drop to `"tiny"` if
  transcription feels too slow for real-time use.
- Both models download automatically on first use (need internet once, then
  fully offline after).

**MAVLink over Ethernet:** if your flight controller or companion setup
bridges MAVLink over network instead of USB (common with `mavlink-router` or
boards with built-in Ethernet), use:
```python
VehicleController("tcp://192.168.1.50:5760")   # or udp://<ip>:<port>
```

## Next steps
1. Get this running against SITL — get comfortable with basic commands first
2. Add a geofence hard-limit in `vehicle_tools.py`
3. Test perception tools with a USB webcam before mounting a real gimbal camera
4. Only then move to real hardware, starting with a tethered/controlled test environment
