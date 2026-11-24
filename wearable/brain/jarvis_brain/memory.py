"""Short conversation memory, kept in RAM for one run of the brain.

The watch sends a session id. Messages for that id are kept until the
brain process stops, or until the list gets longer than the limit.
"""

from __future__ import annotations

import threading
from collections import OrderedDict


class SessionMemory:
    def __init__(self, max_messages: int, max_sessions: int = 40) -> None:
        self.max_messages = max_messages
        self.max_sessions = max_sessions
        self._lock = threading.Lock()
        self._sessions: OrderedDict[str, list[dict[str, str]]] = OrderedDict()

    def history(self, session_id: str) -> list[dict[str, str]]:
        with self._lock:
            return list(self._sessions.get(session_id, []))

    def add(self, session_id: str, role: str, content: str) -> None:
        with self._lock:
            messages = self._sessions.get(session_id)
            if messages is None:
                messages = []
                self._sessions[session_id] = messages
            self._sessions.move_to_end(session_id)
            messages.append({"role": role, "content": content})
            overflow = len(messages) - self.max_messages
            if overflow > 0:
                del messages[:overflow]
            while len(self._sessions) > self.max_sessions:
                self._sessions.popitem(last=False)

    def clear(self, session_id: str) -> None:
        with self._lock:
            self._sessions.pop(session_id, None)
