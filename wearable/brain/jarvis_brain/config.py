"""Read settings from the environment and the .env file next to this package."""

from __future__ import annotations

import os
from dataclasses import dataclass
from pathlib import Path

from dotenv import load_dotenv

from jarvis_brain.persona import load_persona

BRAIN_DIR = Path(__file__).resolve().parents[1]


def _flag(name: str, default: bool = False) -> bool:
    raw = os.getenv(name)
    if raw is None or raw.strip() == "":
        return default
    return raw.strip().lower() in {"1", "true", "yes", "on"}


def _normalize_openai_base(url: str) -> str:
    cleaned = url.strip().rstrip("/")
    if not cleaned:
        return "https://api.openai.com/v1"
    if cleaned.endswith("/v1"):
        return cleaned
    return cleaned + "/v1"


@dataclass
class Settings:
    host: str
    port: int
    llm_provider: str
    llm_base_url: str
    llm_api_key: str
    llm_model: str
    llm_timeout: float
    llm_max_tokens: int
    llm_temperature: float
    stt_provider: str
    stt_base_url: str
    stt_api_key: str
    stt_model: str
    hermes_base_url: str
    hermes_api_key: str
    hermes_model: str
    tts_provider: str
    tailscale_enabled: bool
    memory_messages: int
    persona: str
    using_mock_llm: bool
    mock_llm_reason: str
    using_mock_stt: bool
    mock_stt_reason: str

    @classmethod
    def from_env(cls) -> "Settings":
        # Existing variables win, so a test can force practice mode
        # even if a real key is sitting in .env.
        load_dotenv(BRAIN_DIR / ".env", override=False)

        requested = os.getenv("LLM_PROVIDER", "openai").strip().lower() or "openai"
        api_key = os.getenv("LLM_API_KEY", "").strip()
        base_url = _normalize_openai_base(os.getenv("LLM_BASE_URL", "https://api.openai.com/v1"))
        model = os.getenv("LLM_MODEL", "gpt-4o-mini").strip() or "gpt-4o-mini"

        hermes_base = os.getenv("HERMES_BASE_URL", "").strip().rstrip("/")
        hermes_key = os.getenv("HERMES_API_KEY", "").strip()
        hermes_model = os.getenv("HERMES_MODEL", "hermes").strip() or "hermes"

        using_mock_llm = False
        mock_llm_reason = ""
        provider = requested
        if requested == "mock":
            using_mock_llm = True
            mock_llm_reason = "LLM_PROVIDER is mock"
        elif requested == "hermes":
            if not hermes_base:
                using_mock_llm = True
                provider = "mock"
                mock_llm_reason = "LLM_PROVIDER is hermes, but HERMES_BASE_URL is empty"
        elif requested in {"openai", "local", "openai_compatible"}:
            provider = "local" if requested == "local" else "openai"
            if not api_key:
                using_mock_llm = True
                provider = "mock"
                mock_llm_reason = "LLM_API_KEY is empty, so Jarvis is in practice mode"
        else:
            using_mock_llm = True
            provider = "mock"
            mock_llm_reason = f"Unknown LLM_PROVIDER '{requested}'. Using practice mode."

        stt_requested = os.getenv("STT_PROVIDER", "openai").strip().lower() or "openai"
        stt_key = os.getenv("STT_API_KEY", "").strip() or api_key
        stt_base = os.getenv("STT_BASE_URL", "").strip()
        stt_base = _normalize_openai_base(stt_base or base_url)
        stt_model = os.getenv("STT_MODEL", "whisper-1").strip() or "whisper-1"
        using_mock_stt = False
        mock_stt_reason = ""
        stt_provider = stt_requested
        if stt_requested == "mock" or not stt_key:
            using_mock_stt = True
            stt_provider = "mock"
            mock_stt_reason = (
                "STT_PROVIDER is mock"
                if stt_requested == "mock"
                else "No speech API key is set, so transcription is in practice mode"
            )

        try:
            memory_messages = int(os.getenv("JARVIS_MEMORY_MESSAGES", "8"))
        except ValueError:
            memory_messages = 8
        memory_messages = max(2, min(memory_messages, 40))

        try:
            port = int(os.getenv("JARVIS_PORT", "8787"))
        except ValueError:
            port = 8787

        try:
            timeout = float(os.getenv("LLM_TIMEOUT_SECONDS", "30"))
        except ValueError:
            timeout = 30.0

        try:
            max_tokens = int(os.getenv("LLM_MAX_TOKENS", "120"))
        except ValueError:
            max_tokens = 120

        try:
            temperature = float(os.getenv("LLM_TEMPERATURE", "0.7"))
        except ValueError:
            temperature = 0.7

        return cls(
            host=os.getenv("JARVIS_HOST", "0.0.0.0").strip() or "0.0.0.0",
            port=port,
            llm_provider=provider,
            llm_base_url=base_url,
            llm_api_key=api_key,
            llm_model=model,
            llm_timeout=timeout,
            llm_max_tokens=max(16, min(max_tokens, 1000)),
            llm_temperature=max(0.0, min(temperature, 2.0)),
            stt_provider=stt_provider,
            stt_base_url=stt_base,
            stt_api_key=stt_key,
            stt_model=stt_model,
            hermes_base_url=hermes_base,
            hermes_api_key=hermes_key,
            hermes_model=hermes_model,
            tts_provider=os.getenv("TTS_PROVIDER", "none").strip().lower() or "none",
            tailscale_enabled=_flag("TAILSCALE_ENABLED", False),
            memory_messages=memory_messages,
            persona=load_persona(os.getenv("JARVIS_PERSONA_FILE", "").strip()),
            using_mock_llm=using_mock_llm,
            mock_llm_reason=mock_llm_reason,
            using_mock_stt=using_mock_stt,
            mock_stt_reason=mock_stt_reason,
        )
