"""
ai_pilot_perception.py — ai_pilot.py + camera/microphone perception tools.

Adds two new grounded tools on top of the original flight tools:
  - look_around: captures a real camera frame, runs local vision detection,
    returns only what was actually detected (never invented objects)
  - listen_for_command: records real audio, transcribes it locally,
    returns the actual words heard (or empty if nothing was said)

Same core rule as everywhere else in this project: the AI only knows what a
tool result says. It cannot claim to see or hear something that wasn't
actually detected.

Requires everything from ai_pilot.py plus:
    pip install opencv-python sounddevice ultralytics faster-whisper numpy
"""

import asyncio
import json
import sys
from pathlib import Path

from vehicle_tools import VehicleController, TOOL_SCHEMA as FLIGHT_TOOLS, load_safety_limits, set_limits
from sensors import Camera, record_audio, list_cameras, list_audio_devices
from perception import VisionModel, SpeechModel

try:
    import ollama
except ImportError:
    print("Missing dependency. Run: pip install ollama")
    sys.exit(1)


def load_config(path: str = "config.json") -> dict:
    """
    Loads settings written by the ai_pilot_setup C++ tool. If no config
    exists yet (e.g. running against SITL for the first time without
    running the setup tool), falls back to sensible simulation defaults
    rather than failing.
    """
    config_path = Path(path)
    if not config_path.exists():
        print(f"No {path} found — run ai_pilot_setup first for real hardware, "
              f"or continuing with SITL simulation defaults.\n")
        return {
            "mavlink_connection": "udp://:14540",
            "camera_index": 0,
            "whisper_model_size": "base",
            "yolo_model_size": "yolov8n.pt",
        }

    with open(config_path) as f:
        config = json.load(f)
    print(f"Loaded config.json (board: {config.get('board', {}).get('type', 'unknown')})\n")
    return config


VEHICLE_SPECIFIC_NOTES = {
    "Drone": "This vehicle flies. Altitude and battery-for-flight are critical safety factors.",
    "Rover": "This vehicle drives on the ground. There is no altitude concept — do not "
             "attempt takeoff/land/altitude commands. Speed and terrain are the main risks.",
    "ROV": "This vehicle operates underwater. Depth is the critical hard limit, replacing "
           "altitude. Communication may be intermittent underwater — prefer caution.",
}


def build_system_prompt(vehicle_type: str) -> str:
    vehicle_note = VEHICLE_SPECIFIC_NOTES.get(vehicle_type, "")
    return f"""You are an AI copilot for an unmanned vehicle (a {vehicle_type or "generic vehicle"})
with a camera and microphone.

{vehicle_note}

CRITICAL RULES:
- You may ONLY act through the provided tools. Never claim an action, sighting, or
  spoken command happened unless a tool result confirms it.
- After calling look_around, only describe objects that appear in the detections list.
  If the list is empty, say you don't see anything notable — do not invent a scene.
- After calling listen_for_command, only act on the actual transcribed text. If it's
  empty or unclear, ask the human to repeat themselves rather than guessing intent.
- Before calling arm or takeoff, you must ask the human to explicitly confirm in words.
  Only pass confirmed=true after they have done so in the current conversation.
- If a tool returns success=false, tell the user exactly why and do not retry blindly.
- If anything seems unsafe, unclear, or low battery, prefer 'hold' or 'return_to_launch'.
- You are not permitted to plan or discuss weapons, targeting, or payload delivery of
  any kind, including using the camera for targeting purposes. If asked, refuse and
  explain this system is for navigation and monitoring only.
"""

# New perception tools, appended to the existing flight tool schema
PERCEPTION_TOOLS = [
    {"type": "function", "function": {
        "name": "look_around",
        "description": "Capture a real camera frame and detect real objects/obstacles in it.",
        "parameters": {"type": "object", "properties": {}},
    }},
    {"type": "function", "function": {
        "name": "listen_for_command",
        "description": "Record a few seconds of real audio from the microphone and transcribe it.",
        "parameters": {"type": "object", "properties": {
            "duration_seconds": {"type": "number", "description": "How long to listen, default 4"},
        }},
    }},
]

TOOL_SCHEMA = FLIGHT_TOOLS + PERCEPTION_TOOLS


class AIPilot:
    def __init__(self, connection_string: str = "udp://:14540",
                 camera_index: int = 0, whisper_size: str = "base",
                 vehicle_type: str = "", model_name: str = "llama3.2",
                 temperature: float = 0.2, top_p: float = 0.9):
        self.vehicle = VehicleController(connection_string)
        self.camera = Camera(camera_index)
        self.vision = None    # lazy-loaded — first use downloads/loads the model
        self.speech = None
        self.whisper_size = whisper_size
        self.model_name = model_name
        self.model_options = {"temperature": temperature, "top_p": top_p}
        self.messages = [{"role": "system", "content": build_system_prompt(vehicle_type)}]

    async def start(self):
        print("Connecting to vehicle...")
        result = await self.vehicle.connect()
        print(f"Vehicle connected: {result}")

        cam_result = self.camera.open()
        print(f"Camera: {cam_result}\n")

    # -----------------------------------------------------------------
    # Perception tool implementations
    # -----------------------------------------------------------------
    def look_around(self) -> dict:
        if self.vision is None:
            print("  [loading vision model...]")
            self.vision = VisionModel()

        frame = self.camera.capture_frame()
        if frame is None:
            return {"success": False, "error": "Camera capture failed"}

        return self.vision.analyze_frame(frame)

    def listen_for_command(self, duration_seconds: float = 4.0) -> dict:
        if self.speech is None:
            print("  [loading speech model...]")
            self.speech = SpeechModel(self.whisper_size)

        print(f"  [listening for {duration_seconds}s...]")
        audio = record_audio(duration_seconds)
        return self.speech.transcribe(audio)

    # -----------------------------------------------------------------
    # Tool dispatch — flight tools are async (vehicle), perception tools
    # are sync (local models); both funnel through the same interface
    # -----------------------------------------------------------------
    async def call_tool(self, name: str, args: dict) -> dict:
        if name in ("look_around", "listen_for_command"):
            method = getattr(self, name)
            try:
                return method(**args)
            except Exception as e:
                return {"success": False, "error": str(e)}

        method = getattr(self.vehicle, name, None)
        if method is None:
            return {"success": False, "error": f"Unknown tool: {name}"}
        try:
            return await method(**args)
        except Exception as e:
            return {"success": False, "error": str(e)}

    async def handle_command(self, user_input: str) -> str:
        self.messages.append({"role": "user", "content": user_input})

        response = ollama.chat(model=self.model_name, messages=self.messages,
                                tools=TOOL_SCHEMA, options=self.model_options)
        msg = response["message"]
        self.messages.append(msg)

        if msg.get("tool_calls"):
            for tool_call in msg["tool_calls"]:
                fn_name = tool_call["function"]["name"]
                fn_args = tool_call["function"]["arguments"]
                print(f"  [executing: {fn_name}({fn_args})]")

                tool_result = await self.call_tool(fn_name, fn_args)
                print(f"  [result: {tool_result}]")

                self.messages.append({"role": "tool", "content": json.dumps(tool_result)})

            follow_up = ollama.chat(model=self.model_name, messages=self.messages,
                                     tools=TOOL_SCHEMA, options=self.model_options)
            self.messages.append(follow_up["message"])
            return follow_up["message"]["content"]
        else:
            return msg["content"]


async def main():
    config = load_config()

    # Apply vehicle-specific safety limits (max altitude/depth/speed/battery)
    # before anything else — these are enforced in vehicle_tools.py itself,
    # not just suggested to the model.
    set_limits(load_safety_limits(config))

    tuning = config.get("model_tuning", {})

    print("Available cameras:", list_cameras())
    print("Available audio inputs:", list_audio_devices())
    print()

    pilot = AIPilot(
        connection_string=config["mavlink_connection"],
        camera_index=config.get("camera_index", 0),
        whisper_size=config.get("whisper_model_size", "base"),
        vehicle_type=config.get("vehicle_type", ""),
        model_name=tuning.get("model_name", "llama3.2"),
        temperature=tuning.get("temperature", 0.2),
        top_p=tuning.get("top_p", 0.9),
    )
    await pilot.start()

    print("AI Pilot ready. Try: 'what do you see?' or 'listen for my command'")
    print("Type 'quit' to exit.\n")

    while True:
        user_input = input("You: ").strip()
        if user_input.lower() in ("quit", "exit"):
            break
        if not user_input:
            continue

        reply = await pilot.handle_command(user_input)
        print(f"Pilot: {reply}\n")


if __name__ == "__main__":
    asyncio.run(main())
