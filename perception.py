"""
perception.py — local AI models that turn raw sensor data into grounded facts.

Same anti-hallucination principle as the rest of this project: these models
only report what they actually detect in the real frame/audio. If nothing is
detected, they say so — the AI pilot layer is instructed never to embellish
these results.

Two models, both small enough to run CPU-only (Pi-class hardware), faster
with a GPU (Jetson):

  - Vision: YOLOv8n (nano) — object/obstacle detection, ~6MB model
  - Speech: faster-whisper (tiny/base) — local speech-to-text, no internet

Requires:
    pip install ultralytics faster-whisper opencv-python
"""

import numpy as np
from ultralytics import YOLO
from faster_whisper import WhisperModel


# ---------------------------------------------------------------------------
# Vision: object/obstacle detection
# ---------------------------------------------------------------------------
class VisionModel:
    def __init__(self, model_size: str = "yolov8n.pt"):
        """
        model_size options (smallest/fastest to largest/most accurate):
          yolov8n.pt (nano, ~6MB)   — best for Pi/CPU
          yolov8s.pt (small)        — good balance on Jetson
          yolov8m.pt (medium)       — needs real GPU
        """
        self.model = YOLO(model_size)

    def analyze_frame(self, frame: np.ndarray, confidence_threshold: float = 0.4) -> dict:
        """
        Returns REAL detections only — empty list if nothing found above
        threshold. The AI layer must not describe objects beyond this list.
        """
        if frame is None:
            return {"success": False, "error": "No frame provided"}

        results = self.model(frame, verbose=False)[0]
        detections = []

        for box in results.boxes:
            conf = float(box.conf[0])
            if conf < confidence_threshold:
                continue
            cls_id = int(box.cls[0])
            label = self.model.names[cls_id]
            x1, y1, x2, y2 = [float(v) for v in box.xyxy[0]]
            detections.append({
                "label": label,
                "confidence": round(conf, 2),
                "bbox": {"x1": x1, "y1": y1, "x2": x2, "y2": y2},
            })

        return {"success": True, "detections": detections, "count": len(detections)}


# ---------------------------------------------------------------------------
# Speech: local speech-to-text
# ---------------------------------------------------------------------------
class SpeechModel:
    def __init__(self, model_size: str = "base"):
        """
        model_size options (smallest/fastest to largest/most accurate):
          tiny   — fastest, least accurate, fine for simple commands
          base   — good default for Pi-class hardware
          small  — better accuracy, needs more CPU/RAM or a GPU (Jetson)
        """
        # compute_type="int8" keeps this fast and light enough for CPU-only boards
        self.model = WhisperModel(model_size, device="cpu", compute_type="int8")

    def transcribe(self, audio: np.ndarray, sample_rate: int = 16000) -> dict:
        """
        Returns REAL transcribed text only. If audio is silent/unclear,
        returns an empty/uncertain result rather than a guessed sentence.
        """
        segments, info = self.model.transcribe(audio, language=None, vad_filter=True)
        text_parts = [seg.text.strip() for seg in segments]
        full_text = " ".join(text_parts).strip()

        return {
            "success": True,
            "text": full_text,
            "detected_language": info.language,
            "language_confidence": round(info.language_probability, 2),
            "empty": len(full_text) == 0,
        }
