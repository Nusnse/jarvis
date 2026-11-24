"""Plug-in point for a Hermes-style agent.

If the agent already speaks the OpenAI chat-completions format, set
LLM_PROVIDER=hermes and HERMES_BASE_URL to its /v1 address. No other
code has to change.

If it speaks a different protocol, replace complete() below. Keep the
same arguments and return a plain string. The watch will not notice.
"""

from __future__ import annotations

from jarvis_brain.llm.openai_compatible import OpenAICompatibleModel


class HermesModel(OpenAICompatibleModel):
    def __init__(self, base_url: str, api_key: str, model: str, timeout: float, max_tokens: int,
                 temperature: float) -> None:
        cleaned = base_url.strip().rstrip("/")
        if cleaned and not cleaned.endswith("/v1"):
            cleaned = cleaned + "/v1"
        super().__init__(
            name="hermes",
            base_url=cleaned,
            api_key=api_key or "hermes",
            model=model,
            timeout=timeout,
            max_tokens=max_tokens,
            temperature=temperature,
        )
