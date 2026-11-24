"""Practice brain. Used when no cloud key is configured, and for tests.

The replies are written here on purpose so the watch can be tried before
an API key exists. They always say that practice mode is on.
"""

from __future__ import annotations


class MockLanguageModel:
    name = "mock"

    def complete(self, persona: str, history: list[dict[str, str]], user_text: str) -> str:
        del persona, history
        text = user_text.lower()
        if "joke" in text:
            return (
                "Practice mode is on, so this one is from the laptop. "
                "I told the cloud it was late. It said it was still buffering."
            )
        if "morning" in text or "start the day" in text:
            return (
                "Good morning. Practice mode is on, so no cloud model was called. "
                "Drink some water, pick one important thing, and start there."
            )
        if "focus" in text:
            return "One thing, for ten minutes, with no new tabs. Practice mode, but the advice still stands."
        if "question" in text or "useful question" in text:
            return "What is the one thing that would make today feel finished? (Practice mode is answering.)"
        if any(word in text for word in ("hello", "hi", "there", "online", "jarvis")):
            return (
                "I'm here. This is practice mode on the laptop, so the watch-to-brain path "
                "is working and no cloud model was called."
            )
        clipped = user_text.strip().replace("\n", " ")
        if len(clipped) > 160:
            clipped = clipped[:160] + "..."
        return (
            "Practice mode is on, so I answered on the laptop instead of calling a cloud model. "
            f"You said: {clipped}"
        )
