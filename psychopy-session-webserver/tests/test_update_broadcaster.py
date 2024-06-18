import asyncio
import unittest

from psychopy_session_webserver.update_broadcaster import UpdateBroadcaster
from pydantic import BaseModel


class azip:
    def __init__(self, aiter, *args):
        self._aiterable = aiter
        self._iterables = [iterable for iterable in args]

    def __aiter__(self):
        self._ait = self._aiterable.__aiter__()
        self._it = [it.__iter__() for it in self._iterables]
        return self

    async def __anext__(self):
        try:
            others = [it.__next__() for it in self._it]
        except StopIteration:
            raise StopAsyncIteration

        val = await self._ait.__anext__()
        return val, *others


class UpdateBroadcasterTest(unittest.IsolatedAsyncioTestCase):

    class Nested(BaseModel):
        field: int

    async def asyncSetUp(self) -> None:
        self.loop = asyncio.get_running_loop()

    def test_dynamically_add_attributes(self):
        b = UpdateBroadcaster()
        b.foo = "something"
        self.assertTrue(isinstance(getattr(b, "foo"), UpdateBroadcaster._Store))

    async def test_receive_updates(self):
        b = UpdateBroadcaster(loop=self.loop)

        def add_foobar():
            b.foo = "bar"

        self.loop.call_later(0.02, add_foobar)
        async for e in b.updates():
            self.assertEqual(e.type, "fooUpdate")
            self.assertEqual(e.data, "bar")
            break

    async def test_receive_immediatly_firstvalue(self):
        b = UpdateBroadcaster(loop=self.loop)

        b.foo = "bar"

        def update_foo():
            b.foo = "baz"

        self.loop.call_later(0.02, update_foo)
        expectedData = ["bar", "baz"]
        async for event, expected in azip(b.updates(), expectedData):
            self.assertEqual(event.type, "fooUpdate")
            self.assertEqual(event.data, expected)

    async def test_updates_cleanup(self):
        b = UpdateBroadcaster(loop=self.loop)

        b.foo = "bar"

        expectedData = ["bar"]

        async for event, expected in azip(b.updates(), expectedData):
            self.assertEqual(event.type, "fooUpdate")
            self.assertEqual(event.data, expected)

        # we need some time so the queue are indeed cleaned up
        self.loop.call_later(0.02, lambda: self.assertEqual(0, len(b._queues)))

    async def test_works_with_nested_fields(self):
        b = UpdateBroadcaster(loop=self.loop)

        b.nested = UpdateBroadcasterTest.Nested(field=1)

        expectedData = [
            UpdateBroadcasterTest.Nested(field=1),
            UpdateBroadcasterTest.Nested(field=10),
        ]

        def update_nested():
            b.nested = UpdateBroadcasterTest.Nested(field=10)

        self.loop.call_later(0.02, update_nested)
        async for event, expected in azip(b.updates(), expectedData):
            self.assertEqual(event.type, "nestedUpdate")
            self.assertEqual(event.data.field, expected.field)
