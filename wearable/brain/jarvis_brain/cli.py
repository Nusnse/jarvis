"""Send one sentence to a brain that is already running.

    python -m jarvis_brain.cli Good morning, Jarvis
"""

from __future__ import annotations

import sys

import httpx


def main() -> None:
    text = " ".join(sys.argv[1:]).strip() or "Good morning, Jarvis"
    url = "http://127.0.0.1:8787/v1/chat"
    try:
        response = httpx.post(url, json={"text": text, "device_id": "cli"}, timeout=40)
        response.raise_for_status()
    except httpx.HTTPError as exc:
        print("Could not reach Jarvis Brain at", url)
        print("Start it first with run.bat, then try this again.")
        print(exc)
        sys.exit(1)
    body = response.json()
    if body.get("transcript"):
        print("Heard:", body["transcript"])
    print(body.get("reply") or "(no reply)")
    if not body.get("ok", True):
        sys.exit(2)


if __name__ == "__main__":
    main()
