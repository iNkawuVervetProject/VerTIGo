import asyncio
import unittest
import pytest

from psychopy_session_webserver.types import WindowParameters
from psychopy_session_webserver.update_broadcaster import UpdateBroadcaster
from pydantic import BaseModel


class UpdateBroadcasterTest(unittest.IsolatedAsyncioTestCase):

    class Nested(BaseModel):
        field: int

    async def asyncSetUp(self) -> None:
        self.loop = asyncio.get_running_loop()

    @pytest.mark.timeout(2, method="thread")
    async def test_receive_updates(self):
        b = UpdateBroadcaster(loop=self.loop)
        updates = b.updates()

        b.broadcast("foo", "bar")

        event = await anext(updates)
        self.assertEqual(event.type, "fooUpdate")
        self.assertEqual(event.data, "bar")

    @pytest.mark.timeout(2, method="thread")
    async def test_receive_immediatly_firstvalue(self):
        b = UpdateBroadcaster(loop=self.loop)
        updates = b.updates()

        b.broadcast("foo", "bar")

        event = await anext(updates)

        self.assertEqual(event.type, "fooUpdate")
        self.assertEqual(event.data, "bar")

        b.broadcast("foo", "baz")

        event = await anext(updates)
        self.assertEqual(event.type, "fooUpdate")
        self.assertEqual(event.data, "baz")

    @pytest.mark.timeout(2, method="thread")
    async def test_updates_cleanup(self):
        b = UpdateBroadcaster(loop=self.loop)
        updates = b.updates()

        b.broadcast("foo", "bar")

        event = await anext(updates)
        self.assertEqual(event.type, "fooUpdate")
        self.assertEqual(event.data, "bar")

        del updates

        # we need some time so the queue are indeed cleaned up. Garbage collection?
        self.loop.call_later(0.02, lambda: self.assertEqual(0, len(b._queues)))

    @pytest.mark.timeout(2, method="thread")
    async def test_updates_object(self):
        b = UpdateBroadcaster(loop=self.loop)
        updates = b.updates()

        b.broadcast("window", None)

        event = await anext(updates)
        self.assertEqual(event.type, "windowUpdate")
        self.assertIsNone(event.data)

        b.broadcast("window", WindowParameters())

        event = await anext(updates)
        self.assertEqual(event.type, "windowUpdate")
        self.assertEqual(event.data, WindowParameters())
