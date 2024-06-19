import asyncio
from threading import Thread

from hypercorn.asyncio import serve
from hypercorn.config import Config


class BackgroundServer(Thread):
    """Serve an hypercorn server in background"""

    def __init__(self, app, config: Config, loop: asyncio.AbstractEventLoop = None):
        super(BackgroundServer, self).__init__()

        self._app = app
        self._config = config
        self._loop = loop
        self._shutdown = asyncio.Event()

    def stop(self):
        if self._loop is None:
            return

        self._loop.call_soon_threadsafe(self._shutdown.set)

    def run(self):
        if self._loop is None:
            self._loop = asyncio.new_event_loop()

        self._loop.run_until_complete(
            serve(self._app, self._config, shutdown_trigger=self._shutdown.wait)
        )
