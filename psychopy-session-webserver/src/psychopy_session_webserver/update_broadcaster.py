from asyncio.queues import Queue
from typing import Any, Type

from pydantic import BaseModel


class UpdateEvent(BaseModel):
    type: str
    data: Any


class UpdateBroadcaster:
    class _Store:
        def __set_name__(self, obj, name):
            obj._stores[name] = self
            self._name = "_" + name

        def __get__(self, obj, objtype=None):
            return getattr(obj, self._name)

        def __set__(self, obj, value):
            old = getattr(obj, self._name, None)
            setattr(obj, self._name, value)
            if old != value:
                obj._emitUpdate(self)

        def __delete__(self, instance):
            del instance._stores[self._name[1:]]

    def __init__(self, loop=None):
        self._queues = []
        self._stores = {}
        self._loop = loop

    def __setattr__(self, __name: str, __value: Any) -> None:
        if __name.startswith("_"):
            super(UpdateBroadcaster, self).__setattr__(__name, __value)
            return
        s = UpdateBroadcaster._Store()
        s.__set_name__(self, __name)
        super(UpdateBroadcaster, self).__setattr__(__name, s)
        s.__set__(self, __value)

    async def updates(self):
        q = Queue()
        self._queues.append(q)
        for s in self._stores.values():
            q.put_nowait(self._buildUpdateEvent(s))
        try:
            while True:
                update = await q.get()
                yield update
        finally:
            self._queues.remove(q)

    def _buildUpdateEvent(self, s: _Store):
        return UpdateEvent(
            type=s._name[1:] + "Update",
            data=s.__get__(self),
        )

    def _emitUpdate(self, s: _Store):
        e = self._buildUpdateEvent(s)
        for q in self._queues:
            if self._loop is None:
                q.put_nowait(e)
            else:
                self._loop.call_soon_threadsafe(q.put_nowait, e)
