from functools import partial
from queue import Queue
from dataclasses import dataclass
import asyncio
from typing import Any, Awaitable, Callable, Optional

import structlog


class AsyncTaskRunner:
    """An object that runs a threadsafe loop, and make the bridge with async call"""

    def __init__(self, logger=None):
        self._tasks = Queue()
        self.logfer = (logger or structlog.get_logger()).bind(group="loop")

    def close(self):
        self._tasks.put(None)

    @dataclass
    class _Task:
        future: asyncio.Future
        task: Callable

    def run(self):

        while True:
            task = self._tasks.get()
            if task is None:
                return
            try:
                res = task.task()
                if task.future.done() is False:
                    task.future.get_loop().call_soon_threadsafe(
                        task.future.set_result, res
                    )
            except Exception as err:
                if task.future.done() is False:
                    task.future.get_loop().call_soon_threadsafe(
                        task.future.set_exception, err
                    )
                else:
                    self.logfer.error("task error", exc_info=err)

    def _put_task(self, fn, future):
        self.logfer.debug("add task", fn=fn, future=future)
        self._tasks.put(AsyncTaskRunner._Task(task=fn, future=future))

    @staticmethod
    def in_loop(*, runner=None):
        def decorator(fn) -> Callable[[], Awaitable]:
            async def call(*args, **kwargs):
                _runner = runner or args[0]
                print(runner, args, kwargs)
                future = asyncio.get_event_loop().create_future()
                _runner._put_task(partial(fn, *args, **kwargs), future)

                await future
                exc = future.exception()
                if exc is not None:
                    raise exc
                return future.result()

            return call

        return decorator
