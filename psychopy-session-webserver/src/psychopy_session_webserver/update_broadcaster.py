from asyncio.queues import Queue
from typing import Any, Type

from pydantic import BaseModel


class UpdateEvent(BaseModel):
    type: str
    data: Any


class UpdateBroadcaster:

    def __init__(self, loop=None):
        self._queues = []
        self._stores = {}
        self._loop = loop

    def broadcast(self, name: str, value: Any):
        self._stores[name] = value
        event = UpdateEvent(type=name + "Update", data=value)
        for q in self._queues:
            if self._loop is None:
                q.put_nowait(event)
            else:
                self._loop.call_soon_threadsafe(q.put_nowait, event)

    async def updates(self):
        q = Queue()
        self._queues.append(q)
        for key in sorted(self._stores):
            q.put_nowait(UpdateEvent(type=key + "Update", data=self._stores[key]))
        try:
            while True:
                update = await q.get()
                yield update
        finally:
            self._queues.remove(q)
