import asyncio
import unittest

from psychopy_session_webserver.types import Participant
from psychopy_session_webserver.update_broadcaster import UpdateBroadcaster
from pydantic import BaseModel


class UpdateBroadcasterTest(unittest.IsolatedAsyncioTestCase):

    class Nested(BaseModel):
        field: int

    async def asyncSetUp(self) -> None:
        self.loop = asyncio.get_running_loop()

    async def test_receive_updates(self):
        b = UpdateBroadcaster(loop=self.loop)

        updates = b.updates()

        b.broadcast("foo", "bar")

        event = await anext(updates)
        self.assertEqual(event.type, "fooUpdate")
        self.assertEqual(event.data, "bar")

    async def test_receive_immediatly_firstvalue(self):
        b = UpdateBroadcaster(loop=self.loop)

        b.broadcast("foo", "bar")

        updates = b.updates()

        event = await anext(updates)
        self.assertEqual(event.type, "fooUpdate")
        self.assertEqual(event.data, "bar")

        b.broadcast("foo", "baz")

        event = await anext(updates)
        self.assertEqual(event.type, "fooUpdate")
        self.assertEqual(event.data, "baz")

    async def test_updates_cleanup(self):
        b = UpdateBroadcaster(loop=self.loop)

        b.broadcast("foo", "bar")

        updates = b.updates()

        event = await anext(updates)
        self.assertEqual(event.type, "fooUpdate")
        self.assertEqual(event.data, "bar")

        del updates

        # we need some time so the queue are indeed cleaned up. Garbage collection?
        self.loop.call_later(0.02, lambda: self.assertEqual(0, len(b._queues)))
