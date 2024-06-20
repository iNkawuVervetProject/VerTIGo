import asyncio
import os
from functools import partial
from gettext import Catalog
from glob import glob
from pathlib import Path
from queue import Queue

import structlog
from watchdog import observers

from psychopy_session_webserver.dependency_checker import DependencyChecker
from psychopy_session_webserver.file_event_handler import FileEventHandler
from psychopy_session_webserver.types import Catalog, Experiment
from psychopy_session_webserver.update_broadcaster import UpdateBroadcaster


class Session:

    class _CurrentExperimentField:

        def _transform(self, value):
            if value is None:
                return ""
            elif isinstance(value, str):
                return str
            return value.name

        def __set__(self, obj, value):
            obj._currentExperiment = value
            if hasattr(obj, "_updates"):
                obj._updates.broadcast("experiment", self._transform(value))

        def __get__(self, obj, objType=None):
            return getattr(obj, "_currentExperiment", None)

    def __init__(self, root, session=None, loop=None, dataDir=None, logger=None):
        root = Path(root).resolve()
        self._root = str(root)
        self.logger = None
        self.logger = self._bind_logger(logger)

        self._resourceChecker = DependencyChecker(root)
        self._experiments = {}
        if session is None:
            from psychopy import session

            self._session = session.Session(root, dataDir=dataDir)
        else:
            self._session = session

        self._updates = UpdateBroadcaster(loop)
        self._tasks = Queue()

        # modify session so the currentExperiment modification triggers updates
        setattr(
            self._session.__class__,
            "currentExperiment",
            Session._CurrentExperimentField(),
        )
        setattr(self._session, "_updates", self._updates)
        # this will now broadcast no current experiment
        self._session.currentExperiment = None
        self._updates.broadcast("window", False)
        self._updates.broadcast("catalog", {})

        self._observer = observers.Observer()
        self._event_handler = FileEventHandler(session=self, root=root)
        self._observer.schedule(self._event_handler, root, recursive=True)
        self._observer.start()
        self._sessionLoop = None

        for p in glob("**/*.psyexp", root_dir=root, recursive=True):
            self.addExperiment(file=p)

    def _bind_logger(self, logger=None):
        if logger is not None:
            return logger.bind(module="Session", root=self._root)
        if self.logger is None:
            self.logger = structlog.get_logger().bind(module="Session", root=self._root)
        return self.logger

    @property
    def experiments(self) -> Catalog:
        return self._experiments

    def sessionLoop(self):
        """Runs the Session main loop.

        This method runs the session main loop, where the actual psychopy experiment
        will be run.

        Warnings
        --------
        This method **must** be called from threading.main_thread().

        """
        while True:
            task, future = self._tasks.get()
            if task is None:
                return
            try:
                res = task()
                if future is not None:
                    future.get_loop().call_soon_threadsafe(future.set_result, res)
            except Exception as err:
                if future is not None:
                    future.get_loop().call_soon_thredsafe(future.set_exception, err)
                else:
                    self._bind_logger().error("task error", exc_info=err)

    def _push_task(self, fn, future, *args, **kwargs):
        self._tasks.put((partial(fn, *args, **kwargs), future))

    def closeWindow(self, logger=None):
        if self._session.win is None:
            raise RuntimeError("window is already closed")

        if self._session.currentExperiment is not None:
            raise RuntimeError("an experiment is running")

        self._session.win.close()
        self._session.win = None
        self._bind_logger(logger).info("closed window")
        self._updates.broadcast("window", False)

    async def asyncCloseWindow(self, logger=None):
        future = asyncio.get_event_loop().create_future()

        self._push_task(self.closeWindow, future, logger=logger)

        await future
        exc = future.exception()
        if exc is not None:
            raise exc

    def addExperiment(self, file, key=None):
        if Path(file).is_absolute() is False:
            file = Path(self._session.root).joinpath(file)

        if key is None:
            key = str(Path(file).relative_to(self._session.root))

        self._session.addExperiment(file, key)
        resources = self._session.experimentObjects[key].getResourceFiles()

        self._resourceChecker.addDependencies(
            key,
            [
                r["rel"] if r["rel"].startswith("..") == False else r["abs"]
                for r in resources
            ],
        )
        self._experiments[key] = self._buildExperimentInfo(key)
        self._bind_logger().info("added experiment", key=key)
        self._updates.broadcast("catalog", self._experiments)

    def _buildExperimentInfo(self, key):
        expInfo = self._session.getExpInfoFromExperiment(key)

        return Experiment(
            key=key,
            resources=self._resourceChecker.collections[key].resources,
            parameters=[p for p in expInfo if str(p).endswith("|hid") is False],
        )

    def removeExperiment(self, key):
        if key not in self._experiments:
            raise KeyError(key)
        self._resourceChecker.removeDependencies(key)
        del self._experiments[key]
        del self._session.experimentObjects[key]
        del self._session.experiments[key]
        self._updates.broadcast("catalog", self._experiments)

    def runExperiment(self, key: str, logger=None, earlyFuture=None, **kwargs):
        if self._session.currentExperiment is not None:
            raise RuntimeError(
                f"experiment '{self._session.currentExperiment.name}' is already"
                " running"
            )

        missingParameters = [
            k for k in self._experiments[key].parameters if k not in kwargs
        ]
        if len(missingParameters) > 0:
            raise RuntimeError(f"missing required parameter(s) {missingParameters}")

        invalidParameters = [
            k for k in kwargs if k not in self._experiments[key].parameters
        ]
        if len(invalidParameters) > 0:
            raise RuntimeError(f"invalid experiment parameter(s) {invalidParameters}")

        if self._resourceChecker.collections[key].valid is False:
            raise RuntimeError(
                f"experiment '{key}' is missing the resource(s) "
                f"{self._resourceChecker.collections[key].missing}"
            )

        expInfo = self._session.getExpInfoFromExperiment(key)
        expInfo.update(kwargs)

        if self._session.win is None:
            self._session.setupWindowFromExperiment(key, blocking=True)
            self._updates.broadcast("window", True)

        self._bind_logger(logger).info("starting experiment", key=key)
        if earlyFuture is not None:
            # early future return
            earlyFuture.get_loop().call_soon_threadsafe(earlyFuture.set_result, None)
        self._session.runExperiment(key, expInfo, blocking=True)

    async def asyncRunExperiment(self, key: str, logger=None, **kwargs):
        future = asyncio.get_event_loop().create_future()
        self._push_task(
            self.runExperiment,
            future=None,
            key=key,
            logger=logger,
            earlyFuture=future,
            **kwargs,
        )
        await future
        exc = future.exception()
        if exc is not None:
            raise exc

    def stopExperiment(self, logger=None):
        if self._session.currentExperiment is None:
            raise RuntimeError("no experiment is running")

        self._bind_logger(logger).info("stopping experiment")
        self._session.stopExperiment()

    async def asyncStopExperiment(self, logger=None):
        future = asyncio.get_event_loop().create_future()
        self._push_task(self.stopExperiment, future, logger=logger)
        await future
        exc = future.exception()
        if exc is not None:
            raise exc

    def validateResources(self, paths):
        modifiedExperiments = self._resourceChecker.validate(paths)
        for key in modifiedExperiments:
            self._experiments[key].resources = self._resourceChecker.collections[
                key
            ].resources

        if len(modifiedExperiments) > 0:
            self._updates.broadcast("catalog", self._experiments)

    def updates(self):
        return self._updates.updates()

    def close(self):
        self._updates.close()
        try:
            self._session.stop()
        finally:
            pass
        self._observer.stop()
        self._observer.join()
