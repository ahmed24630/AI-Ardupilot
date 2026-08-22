"""
ai_pilot.py — natural-language command layer for the vehicle.

Same grounded architecture as the file assistant:
  - The AI can ONLY call the functions in vehicle_tools.py
  - It cannot invent telemetry, positions, or outcomes — every fact comes
    from a real MAVLink call to the autopilot
  - High-risk actions (arm, takeoff) require an explicit human confirmation
    step that the AI cannot skip on its own

Run against simulation first:
    1. Install ArduPilot SITL (see README.md)
    2. Start SITL — it exposes udp://:14540 by default
    3. pip install mavsdk ollama
    4. python ai_pilot.py
"""

import asyncio
import json
import sys

from vehicle_tools import VehicleController, TOOL_SCHEMA

try:
    import ollama
except ImportError:
    print("Missing dependency. Run: pip install ollama")
    sys.exit(1)


MODEL = "llama3.2"

SYSTEM_PROMPT = """You are an AI copilot for an unmanned vehicle (drone, rover, or ROV).

CRITICAL RULES:
- You may ONLY act through the provided tools. Never claim an action happened unless the tool result confirms it.
- Never state a position, battery level, or vehicle status unless it came from get_telemetry in this conversation.
- Before calling arm or takeoff, you must ask the human to explicitly confirm in words (e.g. "yes, arm it").
  Only pass confirmed=true after they have done so in the current conversation.
- If a tool returns success=false, tell the user exactly why (using the error message) and do not retry blindly.
- If anything seems unsafe, low battery, out of bounds, unclear command, prefer 'hold' or 'return_to_launch' over guessing.
- You do not have authority to override the safety limits enforced in the tools. If a limit blocks an action, say so.
- You are not permitted to plan or discuss weapons, targeting, or payload delivery of any kind. If asked, refuse and
  explain this system is for navigation and monitoring only.
"""


class AIPilot:
    def __init__(self, connection_string: str = "udp://:14540"):
        self.vehicle = VehicleController(connection_string)
        self.messages = [{"role": "system", "content": SYSTEM_PROMPT}]

    async def start(self):
        print("Connecting to vehicle...")
        result = await self.vehicle.connect()
        print(f"Connected: {result}\n")

    async def call_tool(self, name: str, args: dict) -> dict:
        method = getattr(self.vehicle, name, None)
        if method is None:
            return {"success": False, "error": f"Unknown tool: {name}"}
        try:
            return await method(**args)
        except Exception as e:
            return {"success": False, "error": str(e)}

    async def handle_command(self, user_input: str) -> str:
        self.messages.append({"role": "user", "content": user_input})

        response = ollama.chat(model=MODEL, messages=self.messages, tools=TOOL_SCHEMA)
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

            follow_up = ollama.chat(model=MODEL, messages=self.messages, tools=TOOL_SCHEMA)
            self.messages.append(follow_up["message"])
            return follow_up["message"]["content"]
        else:
            return msg["content"]


async def main():
    pilot = AIPilot()
    await pilot.start()

    print("AI Pilot ready. Try: 'what's the current status?' or 'take off to 10 meters'")
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
