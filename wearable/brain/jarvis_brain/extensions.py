"""Notes for the phases that are intentionally not built yet.

The watch and the brain already have places to hang these on:

- Bluetooth fallback: ../../firmware/src/brain_link.h (BleBrainLink).
- Tailscale: the TAILSCALE_ENABLED setting below. The server already
  listens on the laptop's network. Away from home, the watch's "laptop
  address" can be the laptop's Tailscale IP. A later phase can turn
  that into a real on/off that starts and stops the path.
- Spoken replies: jarvis_brain/tts/__init__.py and the speaker pins
  documented in ../../firmware/src/voice.cpp.
"""

from __future__ import annotations

from jarvis_brain.config import Settings


def extension_status(settings: Settings) -> dict[str, object]:
    return {
        "tts_provider": "none",
        "tts_ready": False,
        "tailscale_enabled": settings.tailscale_enabled,
        "tailscale_note": (
            "The flag is saved. This version does not open a Tailscale path by itself. "
            "Away from home, put the laptop's Tailscale address into the watch setup page."
            if settings.tailscale_enabled
            else "Off. Leave it off until you want the watch to reach this laptop away from home."
        ),
        "ble_fallback": "not built yet; the watch talks over Wi-Fi only",
    }
