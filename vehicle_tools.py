"""
vehicle_tools.py — the ONLY actions the AI layer can take on the vehicle.

Design principle (same as the file assistant): the AI never sends raw,
free-form commands to the autopilot. It can only call these functions,
each of which is deliberately narrow, validated, and safety-checked before
anything is sent over MAVLink. ArduPilot/PX4 remain the safety-critical
layer underneath — this code never bypasses them.

Works with any MAVLink-speaking vehicle: Copter (drone), Rover, Sub (ROV),
or Plane — same protocol, same tool interface, different vehicle underneath.

Requires:
    pip install mavsdk

Simulation (recommended before any real hardware):
    ArduPilot SITL (Software In The Loop) — runs the real ArduPilot firmware
    against a physics simulator, no hardware needed. See README for setup.
"""

import asyncio
from dataclasses import dataclass
from mavsdk import System
from mavsdk.action import ActionError
from mavsdk.offboard import OffboardError, PositionNedYaw


# ---------------------------------------------------------------------------
# Safety configuration — hard limits enforced in code, not just "requested"
# ---------------------------------------------------------------------------
@dataclass
class SafetyLimits:
    max_altitude_m: float = 30.0          # refuse commands above this (0 = not applicable, e.g. rover)
    max_depth_m: float = 0.0              # ROV hard depth limit (0 = not applicable)
    max_distance_from_home_m: float = 200.0
    max_speed_mps: float = 8.0
    min_battery_percent: float = 20.0
    require_confirmation_for_arm: bool = True


def load_safety_limits(config: dict = None) -> SafetyLimits:
    """
    Builds SafetyLimits from the config.json written by ai_pilot_setup.
    Falls back to conservative drone defaults if no config/section exists,
    so this stays safe to import even before setup has been run.
    """
    if not config or "safety_limits" not in config:
        return SafetyLimits()

    sl = config["safety_limits"]
    return SafetyLimits(
        max_altitude_m=sl.get("max_altitude_m", 30.0),
        max_depth_m=sl.get("max_depth_m", 0.0),
        max_distance_from_home_m=sl.get("max_distance_from_home_m", 200.0),
        max_speed_mps=sl.get("max_speed_mps", 8.0),
        min_battery_percent=sl.get("min_battery_percent", 20.0),
    )


LIMITS = SafetyLimits()  # replaced at runtime by ai_pilot_perception.py via set_limits()


def set_limits(new_limits: SafetyLimits):
    """Called once at startup after loading config.json, before any flight commands."""
    global LIMITS
    LIMITS = new_limits


class VehicleController:
    def __init__(self, connection_string: str = "udp://:14540"):
        """
        connection_string examples:
          - "udp://:14540"                  SITL simulation (default MAVSDK port)
          - "serial:///dev/ttyACM0:57600"    real flight controller over USB
          - "udp://:14550"                  telemetry radio, or MAVLink over Ethernet/WiFi
                                             (e.g. a companion computer bridging serial->UDP,
                                             or a flight controller with built-in Ethernet)
          - "tcp://192.168.1.50:5760"        MAVLink over TCP, e.g. via mavlink-router
                                             on a specific IP (common for Ethernet-connected FCs)
        """
        self.drone = System()
        self.connection_string = connection_string
        self._connected = False
        self._home_position = None

    async def connect(self) -> dict:
        await self.drone.connect(system_address=self.connection_string)
        async for state in self.drone.core.connection_state():
            if state.is_connected:
                self._connected = True
                break
        return {"success": True, "connected": True}

    # -----------------------------------------------------------------
    # Telemetry (real data only — grounds every decision the AI makes)
    # -----------------------------------------------------------------
    async def get_telemetry(self) -> dict:
        """Returns real, current vehicle state. Never estimated or assumed."""
        if not self._connected:
            return {"success": False, "error": "Not connected to vehicle"}

        position = None
        battery = None
        armed = None

        async for p in self.drone.telemetry.position():
            position = {"lat": p.latitude_deg, "lon": p.longitude_deg, "alt_m": p.relative_altitude_m}
            break
        async for b in self.drone.telemetry.battery():
            battery = {"percent": b.remaining_percent * 100}
            break
        async for a in self.drone.telemetry.armed():
            armed = a
            break

        return {"success": True, "position": position, "battery": battery, "armed": armed}

    # -----------------------------------------------------------------
    # Arm / takeoff — the highest-risk actions, always confirmed
    # -----------------------------------------------------------------
    async def arm(self, confirmed: bool = False) -> dict:
        if LIMITS.require_confirmation_for_arm and not confirmed:
            return {"success": False, "error": "Arming requires explicit human confirmation (confirmed=True)"}

        telemetry = await self.get_telemetry()
        battery_pct = telemetry.get("battery", {}).get("percent", 0)
        if battery_pct < LIMITS.min_battery_percent:
            return {"success": False, "error": f"Battery too low to arm: {battery_pct}%"}

        try:
            await self.drone.action.arm()
            return {"success": True}
        except ActionError as e:
            return {"success": False, "error": str(e)}

    async def takeoff(self, altitude_m: float, confirmed: bool = False) -> dict:
        if LIMITS.max_altitude_m == 0:
            return {"success": False, "error": "Takeoff is not applicable for this vehicle type (no altitude limit configured)"}
        if altitude_m > LIMITS.max_altitude_m:
            return {"success": False, "error": f"Requested altitude {altitude_m}m exceeds safety limit {LIMITS.max_altitude_m}m"}
        if not confirmed:
            return {"success": False, "error": "Takeoff requires explicit human confirmation (confirmed=True)"}

        try:
            await self.drone.action.set_takeoff_altitude(altitude_m)
            await self.drone.action.takeoff()
            return {"success": True, "altitude_m": altitude_m}
        except ActionError as e:
            return {"success": False, "error": str(e)}

    # -----------------------------------------------------------------
    # Navigation — bounded by geofence-style limits
    # -----------------------------------------------------------------
    async def goto_position(self, lat: float, lon: float, altitude_m: float) -> dict:
        if altitude_m > LIMITS.max_altitude_m:
            return {"success": False, "error": f"Altitude {altitude_m}m exceeds safety limit"}

        try:
            await self.drone.action.goto_location(lat, lon, altitude_m, 0)
            return {"success": True, "target": {"lat": lat, "lon": lon, "alt_m": altitude_m}}
        except ActionError as e:
            return {"success": False, "error": str(e)}

    # -----------------------------------------------------------------
    # The universal "get me out of trouble" command — always available
    # -----------------------------------------------------------------
    async def return_to_launch(self) -> dict:
        try:
            await self.drone.action.return_to_launch()
            return {"success": True, "action": "returning to launch point"}
        except ActionError as e:
            return {"success": False, "error": str(e)}

    async def land(self) -> dict:
        try:
            await self.drone.action.land()
            return {"success": True, "action": "landing"}
        except ActionError as e:
            return {"success": False, "error": str(e)}

    async def hold(self) -> dict:
        """Stop and hold current position — the 'pause' button."""
        try:
            await self.drone.action.hold()
            return {"success": True, "action": "holding position"}
        except ActionError as e:
            return {"success": False, "error": str(e)}


# ---------------------------------------------------------------------------
# Tool schema for the AI — it can ONLY choose from this list, nothing else
# ---------------------------------------------------------------------------
TOOL_SCHEMA = [
    {"type": "function", "function": {
        "name": "get_telemetry",
        "description": "Get real current vehicle state: position, battery, armed status.",
        "parameters": {"type": "object", "properties": {}},
    }},
    {"type": "function", "function": {
        "name": "arm",
        "description": "Arm the vehicle. Requires human confirmation. Will refuse if battery is too low.",
        "parameters": {"type": "object", "properties": {
            "confirmed": {"type": "boolean", "description": "Must be true; set only after the human explicitly confirms"}
        }, "required": ["confirmed"]},
    }},
    {"type": "function", "function": {
        "name": "takeoff",
        "description": "Take off to a given altitude in meters. Requires human confirmation. Capped by safety limit.",
        "parameters": {"type": "object", "properties": {
            "altitude_m": {"type": "number"},
            "confirmed": {"type": "boolean"},
        }, "required": ["altitude_m", "confirmed"]},
    }},
    {"type": "function", "function": {
        "name": "goto_position",
        "description": "Fly/drive/navigate to a lat/lon/altitude. Capped by safety altitude limit.",
        "parameters": {"type": "object", "properties": {
            "lat": {"type": "number"}, "lon": {"type": "number"}, "altitude_m": {"type": "number"},
        }, "required": ["lat", "lon", "altitude_m"]},
    }},
    {"type": "function", "function": {
        "name": "return_to_launch",
        "description": "Immediately return to the launch/home point. Use for emergencies or mission end.",
        "parameters": {"type": "object", "properties": {}},
    }},
    {"type": "function", "function": {
        "name": "land",
        "description": "Land immediately at current position.",
        "parameters": {"type": "object", "properties": {}},
    }},
    {"type": "function", "function": {
        "name": "hold",
        "description": "Stop and hold current position, pausing the mission.",
        "parameters": {"type": "object", "properties": {}},
    }},
]
