import asyncio
import unittest

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

    async def test_works_with_nested_fields(self):
        b = UpdateBroadcaster(loop=self.loop)

        b.broadcast("nested", UpdateBroadcasterTest.Nested(field=1))

        updates = b.updates()

        event = await anext(updates)
        self.assertEqual(event.type, "nestedUpdate")
        self.assertEqual(event.data, UpdateBroadcasterTest.Nested(field=1))

        # Note we should reassign the field
        b.broadcast("nested", UpdateBroadcasterTest.Nested(field=10))

        event = await anext(updates)
        self.assertEqual(event.type, "nestedUpdate")
        self.assertEqual(event.data, UpdateBroadcasterTest.Nested(field=10))

    async def test_works_with_dictionary(self):
        b = UpdateBroadcaster(loop=self.loop)

        dic = {"a": 1, "b": 2}
        b.broadcast("dictionnary", dic)

        it = b.updates()
        event = await anext(it)
        self.assertEqual(event.type, "dictionnaryUpdate")
        self.assertDictEqual(event.data, dic)
        dic["b"] = 4

        # Note we should reassign the field
        b.broadcast("dictionnary", dic)
        event = await anext(it)
        self.assertEqual(event.type, "dictionnaryUpdate")
        self.assertDictEqual(event.data, dic)
