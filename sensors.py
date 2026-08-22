"""
sensors.py — access to camera and microphone via standard Linux drivers.

Portability note: this deliberately uses only V4L2 (video4linux2, the
standard Linux camera driver interface) and ALSA/PortAudio (standard Linux
audio driver interface) through cross-platform Python libraries. This means
the SAME code works unchanged on Raspberry Pi, BeagleBone Black/Blue, Jetson,
or a generic x86 Linux board — because they all expose cameras as
/dev/video0, /dev/video1, etc. and audio through ALSA, regardless of the
underlying board.

Requires:
    pip install opencv-python sounddevice numpy

Linux permissions (one-time setup, needed on most boards):
    sudo usermod -a -G video $USER    # camera access
    sudo usermod -a -G audio $USER    # microphone access
    # then log out/in for group changes to apply
"""

import cv2
import sounddevice as sd
import numpy as np
from pathlib import Path


# ---------------------------------------------------------------------------
# Device discovery — never assume, always check what's actually connected
# ---------------------------------------------------------------------------
def list_cameras(max_check: int = 5) -> list[dict]:
    """Probe /dev/video* devices and report which ones actually open successfully."""
    found = []
    for i in range(max_check):
        cap = cv2.VideoCapture(i)
        if cap.isOpened():
            width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
            height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
            found.append({"index": i, "device": f"/dev/video{i}", "resolution": f"{width}x{height}"})
            cap.release()
    return found


def list_audio_devices() -> list[dict]:
    """Report real ALSA/PortAudio input devices — never invented."""
    devices = sd.query_devices()
    inputs = []
    for idx, dev in enumerate(devices):
        if dev["max_input_channels"] > 0:
            inputs.append({
                "index": idx,
                "name": dev["name"],
                "sample_rate": dev["default_samplerate"],
            })
    return inputs


# ---------------------------------------------------------------------------
# Camera capture
# ---------------------------------------------------------------------------
class Camera:
    def __init__(self, index: int = 0):
        self.index = index
        self.cap = None

    def open(self) -> dict:
        self.cap = cv2.VideoCapture(self.index)
        if not self.cap.isOpened():
            return {"success": False, "error": f"Could not open camera at index {self.index}"}
        return {"success": True}

    def capture_frame(self) -> np.ndarray | None:
        """Returns a real captured frame, or None if capture failed. Never a fake/blank image."""
        if self.cap is None or not self.cap.isOpened():
            return None
        ret, frame = self.cap.read()
        return frame if ret else None

    def save_snapshot(self, path: str) -> dict:
        frame = self.capture_frame()
        if frame is None:
            return {"success": False, "error": "Failed to capture frame"}
        cv2.imwrite(path, frame)
        return {"success": True, "path": path}

    def close(self):
        if self.cap is not None:
            self.cap.release()


# ---------------------------------------------------------------------------
# Microphone capture
# ---------------------------------------------------------------------------
def record_audio(duration_seconds: float = 4.0, sample_rate: int = 16000, device: int = None) -> np.ndarray:
    """
    Records real audio from the microphone. Blocking call.
    16kHz is the standard input rate Whisper expects.
    """
    recording = sd.rec(
        int(duration_seconds * sample_rate),
        samplerate=sample_rate,
        channels=1,
        dtype="float32",
        device=device,
    )
    sd.wait()
    return recording.flatten()
