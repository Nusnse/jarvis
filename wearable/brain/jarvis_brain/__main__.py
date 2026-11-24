"""Start Jarvis Brain. Run from the brain folder: python -m jarvis_brain"""

from __future__ import annotations

import socket

import uvicorn

from jarvis_brain.config import Settings
from jarvis_brain.server import create_app


def _lan_ip() -> str:
    try:
        probe = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        probe.connect(("8.8.8.8", 80))
        ip = probe.getsockname()[0]
        probe.close()
        return ip
    except OSError:
        return ""


def main() -> None:
    settings = Settings.from_env()
    app = create_app(settings)
    print()
    print("Jarvis Brain is running.")
    print(f"  On this laptop:  http://127.0.0.1:{settings.port}")
    lan = _lan_ip()
    if lan and lan != "127.0.0.1":
        print(f"  From the watch:  http://{lan}:{settings.port}")
    if settings.using_mock_llm:
        print(f"  Practice mode:   {settings.mock_llm_reason}")
    else:
        print(f"  Model:           {settings.llm_provider} / {settings.llm_model}")
    print("Leave this window open while you use the watch. Ctrl+C stops it.")
    print()
    uvicorn.run(app, host=settings.host, port=settings.port, log_level="info")


if __name__ == "__main__":
    main()
