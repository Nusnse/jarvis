"""Chat completions against any server that speaks the OpenAI HTTP API.

That includes OpenAI itself, Groq, OpenRouter, and a local Ollama server.
Swap them by changing LLM_BASE_URL and LLM_MODEL in the .env file.
"""

from __future__ import annotations

import httpx


class ModelError(Exception):
    """The cloud model did not return a usable reply."""


class OpenAICompatibleModel:
    def __init__(self, name: str, base_url: str, api_key: str, model: str, timeout: float, max_tokens: int,
                 temperature: float) -> None:
        self.name = name
        self.base_url = base_url.rstrip("/")
        self.api_key = api_key
        self.model = model
        self.timeout = timeout
        self.max_tokens = max_tokens
        self.temperature = temperature

    def complete(self, persona: str, history: list[dict[str, str]], user_text: str) -> str:
        messages = [{"role": "system", "content": persona}]
        messages.extend(history)
        messages.append({"role": "user", "content": user_text})
        payload = {
            "model": self.model,
            "messages": messages,
            "max_tokens": self.max_tokens,
            "temperature": self.temperature,
        }
        headers = {"Authorization": f"Bearer {self.api_key}", "Content-Type": "application/json"}
        url = f"{self.base_url}/chat/completions"
        try:
            response = httpx.post(url, json=payload, headers=headers, timeout=self.timeout)
        except httpx.HTTPError as exc:
            raise ModelError(f"Could not reach the model at {self.base_url}") from exc
        if response.status_code >= 400:
            detail = response.text.strip().replace("\n", " ")
            if len(detail) > 180:
                detail = detail[:180] + "..."
            raise ModelError(f"The model returned HTTP {response.status_code}. {detail}")
        try:
            body = response.json()
            text = body["choices"][0]["message"]["content"]
        except (KeyError, IndexError, TypeError, ValueError) as exc:
            raise ModelError("The model answered in an unexpected shape.") from exc
        reply = (text or "").strip()
        if not reply:
            raise ModelError("The model sent an empty reply.")
        return reply
