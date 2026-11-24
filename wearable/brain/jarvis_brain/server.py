"""HTTP and WebSocket front door for the watch and the test page."""

from __future__ import annotations

import uuid
from fastapi import FastAPI, Request, WebSocket, WebSocketDisconnect
from fastapi.responses import FileResponse, JSONResponse
from fastapi.staticfiles import StaticFiles

from jarvis_brain import __version__
from jarvis_brain.config import BRAIN_DIR, Settings
from jarvis_brain.extensions import extension_status
from jarvis_brain.llm import build_language_model
from jarvis_brain.llm.base import LanguageModel
from jarvis_brain.llm.openai_compatible import ModelError
from jarvis_brain.memory import SessionMemory
from jarvis_brain.stt import build_transcriber
from jarvis_brain.stt.base import Transcriber
from jarvis_brain.tts import build_speaker

STATIC_DIR = BRAIN_DIR / "static"
MAX_TEXT = 500
MAX_AUDIO = 2_000_000


class Jarvis:
    def __init__(self, settings: Settings, model: LanguageModel, transcriber: Transcriber) -> None:
        self.settings = settings
        self.model = model
        self.transcriber = transcriber
        self.speaker = build_speaker(settings.tts_provider)
        self.memory = SessionMemory(settings.memory_messages)

    def reply(self, text: str, session_id: str, device_id: str) -> dict[str, object]:
        cleaned = " ".join(text.split())
        if not cleaned:
            return self._payload(session_id, device_id, "", "Say something first.", ok=False)
        if len(cleaned) > MAX_TEXT:
            cleaned = cleaned[:MAX_TEXT]

        history = self.memory.history(session_id)
        ok = True
        try:
            answer = self.model.complete(self.settings.persona, history, cleaned)
        except ModelError as exc:
            ok = False
            answer = f"The model did not answer. {exc}"
        except Exception:
            ok = False
            answer = "Something unexpected went wrong while asking the model. Check the brain window for details."

        self.memory.add(session_id, "user", cleaned)
        self.memory.add(session_id, "assistant", answer)
        return self._payload(session_id, device_id, "", answer, ok=ok)

    def hear(self, wav_bytes: bytes, session_id: str, device_id: str) -> dict[str, object]:
        if len(wav_bytes) < 44:
            return self._payload(session_id, device_id, "", "That recording was too short to use.", ok=False)
        if len(wav_bytes) > MAX_AUDIO:
            return self._payload(
                session_id, device_id, "", "That recording is too long. Keep Talk under a few seconds.", ok=False
            )
        try:
            transcript = self.transcriber.transcribe(wav_bytes)
        except ModelError as exc:
            return self._payload(session_id, device_id, "", f"I could not transcribe that. {exc}", ok=False)
        except Exception:
            return self._payload(session_id, device_id, "", "Speech-to-text failed unexpectedly.", ok=False)

        if not transcript:
            return self._payload(
                session_id,
                device_id,
                "",
                "I didn't catch any words. Try Talk again, a little closer to the watch.",
                ok=False,
            )
        result = self.reply(transcript, session_id, device_id)
        result["transcript"] = transcript
        return result

    def _payload(self, session_id: str, device_id: str, transcript: str, reply: str, ok: bool) -> dict[str, object]:
        speech = self.speaker.synthesize(reply) if ok else None
        return {
            "ok": ok,
            "reply": reply,
            "transcript": transcript or None,
            "session_id": session_id,
            "device_id": device_id,
            "provider": self.model.name,
            "speech_available": speech is not None,
        }


def _session(value: str | None) -> str:
    cleaned = (value or "").strip()
    if cleaned and len(cleaned) <= 80:
        return cleaned
    return "web-" + uuid.uuid4().hex[:12]


def create_app(settings: Settings | None = None) -> FastAPI:
    settings = settings or Settings.from_env()
    jarvis = Jarvis(settings, build_language_model(settings), build_transcriber(settings))
    app = FastAPI(title="Jarvis Brain", version=__version__)
    app.state.jarvis = jarvis
    app.state.settings = settings

    if STATIC_DIR.is_dir():
        app.mount("/static", StaticFiles(directory=STATIC_DIR), name="static")

    @app.get("/")
    def home() -> FileResponse:
        return FileResponse(STATIC_DIR / "index.html")

    @app.get("/health")
    def health() -> dict[str, object]:
        return {
            "ok": True,
            "service": "jarvis-brain",
            "version": __version__,
            "llm_provider": settings.llm_provider,
            "llm_model": settings.hermes_model if settings.llm_provider == "hermes" else settings.llm_model,
            "using_mock_llm": settings.using_mock_llm,
            "mock_llm_reason": settings.mock_llm_reason,
            "stt_provider": settings.stt_provider,
            "using_mock_stt": settings.using_mock_stt,
            "mock_stt_reason": settings.mock_stt_reason,
            "memory_messages": settings.memory_messages,
            **extension_status(settings),
        }

    @app.post("/v1/chat")
    async def chat(request: Request) -> JSONResponse:
        body = await request.json()
        text = str(body.get("text") or "")
        session_id = _session(body.get("session_id"))
        device_id = str(body.get("device_id") or "browser")[:40]
        return JSONResponse(jarvis.reply(text, session_id, device_id))

    @app.post("/v1/voice")
    async def voice(request: Request) -> JSONResponse:
        wav = await request.body()
        session_id = _session(request.headers.get("x-session-id"))
        device_id = (request.headers.get("x-device-id") or "browser")[:40]
        return JSONResponse(jarvis.hear(wav, session_id, device_id))

    @app.post("/v1/session/reset")
    async def reset_session(request: Request) -> dict[str, object]:
        body = await request.json()
        session_id = _session(body.get("session_id"))
        jarvis.memory.clear(session_id)
        return {"ok": True, "session_id": session_id}

    @app.websocket("/ws")
    async def ws(socket: WebSocket) -> None:
        await socket.accept()
        session_id = "ws-" + uuid.uuid4().hex[:12]
        try:
            while True:
                incoming = await socket.receive_json()
                if incoming.get("session_id"):
                    session_id = _session(str(incoming.get("session_id")))
                text = str(incoming.get("text") or "")
                device_id = str(incoming.get("device_id") or "websocket")[:40]
                await socket.send_json(jarvis.reply(text, session_id, device_id))
        except WebSocketDisconnect:
            return

    return app


app = create_app()
