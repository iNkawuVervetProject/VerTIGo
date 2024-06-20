import contextlib
import threading
import unittest
from unittest.mock import Mock
from psychopy_session_webserver.async_task_runner import AsyncTaskRunner


class MyClass(AsyncTaskRunner):
    cleanUp = Mock()

    def __init__(self):
        super(MyClass, self).__init__()

        self.command = Mock()
        self.cleanUp = Mock()

    @AsyncTaskRunner.in_loop()
    def asyncCommand(self, *args, **kwargs):
        self.command(*args, **kwargs)

    async def nestedCommand(self, *args, **kwargs):
        @AsyncTaskRunner.in_loop(runner=self)
        def fn(*args, **kwargs):
            self.command(*args, **kwargs)

        await fn(*args, **kwargs)


class AsyncRunnerTestCase(unittest.IsolatedAsyncioTestCase):

    def setUp(self):
        self.obj = MyClass()

    def test_run_synchronously(self):
        self.obj.command()

        self.obj.command.assert_called_once()

    @contextlib.contextmanager
    def with_loop(self):
        thread = threading.Thread(target=self.obj.run)
        thread.start()
        try:
            yield
        finally:
            self.obj.close()
            thread.join()

    async def test_run_direct(self):
        with self.with_loop():

            await self.obj.asyncCommand(1, 12.0)

        self.obj.command.assert_called_once_with(1, 12.0)

    async def test_run_nested(self):
        with self.with_loop():

            await self.obj.nestedCommand("foo")

        self.obj.command.assert_called_once_with("foo")
