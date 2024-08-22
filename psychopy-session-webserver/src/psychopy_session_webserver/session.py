import asyncio
import os
from functools import partial
from gettext import Catalog
from glob import glob
from pathlib import Path
from queue import Queue
from typing import Dict, Optional

import structlog
from watchdog import observers

from psychopy_session_webserver.async_task_runner import AsyncTaskRunner
from psychopy_session_webserver.dependency_checker import DependencyChecker
from psychopy_session_webserver.file_event_handler import FileEventHandler
from psychopy_session_webserver.participants_registry import ParticipantRegistry
from psychopy_session_webserver.types import Catalog, Experiment, Participant
from psychopy_session_webserver.update_broadcaster import UpdateBroadcaster


class Session(AsyncTaskRunner):

    def __init__(self, root, session=None, loop=None, dataDir=None, logger=None):
        root = Path(root).resolve()
        self._root = str(root)
        self.logger = None
        self.logger = self._bind_logger(logger)
        super(Session, self).__init__(logger=self.logger)

        self._resourceChecker = DependencyChecker(root)
        self._experiments = {}
        if session is None:
            from psychopy import session


            self._session = session.Session(root, dataDir=dataDir)
        else:
            self._session = session

        self._updates = UpdateBroadcaster(loop)
        self._tasks = Queue()

        self._currentExperiment = None
        self._updates.broadcast("experiment", "")
        self._updates.broadcast("window", False)
        self._updates.broadcast("catalog", {})
        self._participants = ParticipantRegistry(self._updates, dataDir=dataDir)

        self._observer = observers.Observer()
        self._event_handler = FileEventHandler(session=self, root=root)
        self._observer.schedule(self._event_handler, root, recursive=True)
        self._observer.start()

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

    @property
    def participants(self) -> Dict[str, Participant]:
        return self._participants._participants

    def closeWindow(self, logger=None):
        if self._session.win is None:
            raise RuntimeError("window is already closed")

        if self._currentExperiment is not None:
            raise RuntimeError("an experiment is running")

        self._session.win.close()
        self._session.win = None
        self._bind_logger(logger).info("closed window")
        self._updates.broadcast("window", False)

    @AsyncTaskRunner.in_loop()
    def asyncCloseWindow(self, logger=None):
        self.closeWindow(logger)

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
        self._updates.broadcastDict("catalog", key, self._experiments[key])

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
        self._updates.broadcastDict("catalog", key, None)

    def _prepareExperiment(self, key: str, *, logger, **kwargs):
        logger.debug("preparing", current=self._currentExperiment)
        if key not in self._experiments:
            raise RuntimeError(f"unknown experiment '{key}'")

        if self._currentExperiment is not None:
            raise RuntimeError(
                f"experiment '{self._currentExperiment}' is already running"
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

        if int(expInfo.get("session", 1)) < 1:
            raise RuntimeError(
                f"session value should be at least 1 (got: {expInfo['session']})"
            )

        if "participant" in expInfo and "session" in expInfo:
            self._participants[expInfo["participant"]] = int(expInfo["session"]) + 1

        return expInfo

    def _runExperiment(
        self, key, *, expInfo, logger, earlyFuture: Optional[asyncio.Future] = None
    ):
        if self._session.win is None:
            logger.debug("opening window")
            self._session.setupWindowFromExperiment(key, blocking=True)
            self._updates.broadcast("window", True)

            self._currentExperiment = key

        self._updates.broadcast("experiment", key)

        logger.info("starting", current=self._currentExperiment)

        if earlyFuture is not None:
            earlyFuture.get_loop().call_soon_threadsafe(earlyFuture.set_result, None)

        try:
            self._session.runExperiment(key, expInfo, blocking=True)
        finally:
            self._currentExperiment = None
            self._updates.broadcast("experiment", "")
            logger.debug("done", current=self._currentExperiment)

    def runExperiment(self, key: str, logger=None, **kwargs):
        logger = self._bind_logger(logger).bind(experiment=key)
        expInfo = self._prepareExperiment(key, logger=logger, **kwargs)
        self._runExperiment(key, logger=logger, expInfo=expInfo)

    async def asyncRunExperiment(self, key: str, logger=None, **kwargs):
        logger = self._bind_logger(logger).bind(experiment=key)
        # we make all necessary check before sending the task
        expInfo = self._prepareExperiment(key, logger=logger, **kwargs)

        # we inject our own future to return early from the experiment run
        future = asyncio.get_event_loop().create_future()

        @AsyncTaskRunner.in_loop(runner=self, future=future)
        def run():
            # by passing early future, it will be done before the completion of the
            # experiment.
            self._runExperiment(key, logger=logger, expInfo=expInfo, earlyFuture=future)

        await run()

    def stopExperiment(self, logger=None):
        if self._session.currentExperiment is None:
            raise RuntimeError("no experiment is running")

        self._bind_logger(logger).info("stopping experiment")
        self._session.stopExperiment()

    @AsyncTaskRunner.in_loop()
    def asyncStopExperiment(self, logger=None):
        self.stopExperiment(logger)

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
        super(Session, self).close()

        self._updates.close()
        try:
            self._session.stop()
        finally:
            pass
        self._observer.stop()
        self._observer.join()
