"""The small contract every language-model backend has to follow.

A new backend (Hermes, a local model, something else) implements complete().
The web server and the watch do not change.
"""

from __future__ import annotations

from typing import Protocol


class LanguageModel(Protocol):
    name: str

    def complete(self, persona: str, history: list[dict[str, str]], user_text: str) -> str:
        """Return the assistant's reply as plain text."""
