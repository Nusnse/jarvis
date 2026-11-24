"""Pick the language model from settings."""

from __future__ import annotations

from jarvis_brain.config import Settings
from jarvis_brain.llm.base import LanguageModel
from jarvis_brain.llm.hermes import HermesModel
from jarvis_brain.llm.mock import MockLanguageModel
from jarvis_brain.llm.openai_compatible import OpenAICompatibleModel


def build_language_model(settings: Settings) -> LanguageModel:
    if settings.using_mock_llm or settings.llm_provider == "mock":
        return MockLanguageModel()
    if settings.llm_provider == "hermes":
        return HermesModel(
            base_url=settings.hermes_base_url,
            api_key=settings.hermes_api_key,
            model=settings.hermes_model,
            timeout=settings.llm_timeout,
            max_tokens=settings.llm_max_tokens,
            temperature=settings.llm_temperature,
        )
    model_name = "local" if settings.llm_provider == "local" else "openai"
    return OpenAICompatibleModel(
        name=model_name,
        base_url=settings.llm_base_url,
        api_key=settings.llm_api_key,
        model=settings.llm_model,
        timeout=settings.llm_timeout,
        max_tokens=settings.llm_max_tokens,
        temperature=settings.llm_temperature,
    )
