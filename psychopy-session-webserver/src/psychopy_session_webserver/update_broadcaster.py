from asyncio.queues import Queue
from typing import Union


from psychopy_session_webserver.types import (
    Updatable,
    UpdateEvent,
    Experiment,
    Participant,
)


class UpdateBroadcaster:

    def __init__(self, loop=None):
        self._queues = []
        self._stores = {}
        self._loop = loop

    def _push_all(self, value):
        for q in self._queues:
            if self._loop is None:
                q.put_nowait(value)
            else:
                self._loop.call_soon_threadsafe(q.put_nowait, value)

    def close(self):
        self._push_all(None)

    def broadcastDict(self, name: str, key: str, value: Union[Experiment, Participant]):
        if name not in self._stores:
            self._stores[name] = {}
        if isinstance(self._stores, dict) == False:
            raise RuntimeError(f"'{name}' is not a dict")
        if value is None and key in self._stores[name]:
            del self._stores[name][key]
        else:
            self._stores[name][key] = value

        self._push_all(UpdateEvent(type=name + "Update", data={key: value}))

    def broadcast(self, name: str, value: Updatable):
        if isinstance(value, dict) and isinstance(self._stores.get(name, None), dict):
            deletedKeys = [
                k for k in self._stores.get(name, {}).keys() if k not in value.keys()
            ]
            for k in deletedKeys:
                self._push_all(UpdateEvent(type=name + "Update", data={k: None}))

        self._stores[name] = value
        event = UpdateEvent(type=name + "Update", data=value)
        self._push_all(event)

    async def updates(self):
        q = Queue()
        self._queues.append(q)
        for key in sorted(self._stores):
            q.put_nowait(UpdateEvent(type=key + "Update", data=self._stores[key]))
        try:
            while True:
                update = await q.get()
                if update is None:
                    return
                yield update
        finally:
            self._queues.remove(q)
