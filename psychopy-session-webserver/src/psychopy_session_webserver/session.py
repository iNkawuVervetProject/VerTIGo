from ctypes import ArgumentError
from gettext import Catalog
from glob import glob
from pathlib import Path
from watchdog import observers

from psychopy_session_webserver.dependency_checker import DependencyChecker
from psychopy_session_webserver.file_event_handler import FileEventHandler
from psychopy_session_webserver.update_broadcaster import UpdateBroadcaster
from psychopy_session_webserver.types import Experiment, Catalog


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
            obj._updates.broadcast("experiment", self._transform(value))

        def __get__(self, obj, objType=None):
            return getattr(obj, "_currentExperiment", None)

    def __init__(self, root, session=None, loop=None):

        root = Path(root).resolve()
        self._resourceChecker = DependencyChecker(root)
        self._experiments = {}
        if session is None:
            from psychopy import session

            self._session = session.Session(root)
        else:
            self._session = session

        self._updates = UpdateBroadcaster(loop)

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

        for p in glob("**/*.psyexp", root_dir=root, recursive=True):
            self.addExperiment(file=p)

    @property
    def experiments(self) -> Catalog:
        return self._experiments

    def openWindow(self, framerate: int | float | None = None, **kwargs):
        """Opens the window for the session

        Parameters
        ----------
        Those are passed to psychopy.visual.Window initializor.
        size : tuple
            the size of the window

        3
        """
        if self._session.win is not None:
            return

        self._session.setupWindowFromParams(
            kwargs, blocking=False, measureFrameRate=False
        )
        self._framerate = (
            self._session.win.getActualFrameRate() if framerate is None else framerate
        )
        self._updates.broadcast("window", True)

    def closeWindow(self):
        if self._session.win is None:
            return
        self._session.win.close()
        self._session.win = None
        self._updates.broadcast("window", False)

    def sessionLoop(self):
        """Runs the Session main loop.

        This method runs the session main loop, where the actual psychopy experiment
        will be run.

        Warnings
        --------
        This method **must** be called from threading.main_thread().

        """
        self._session.start()

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

    def runExperiment(self, key, **kwargs):
        if self._session.currentExperiment is not None:
            raise RuntimeError(
                f"experiment '{self._session.currentExperiment.name}' is already"
                " running"
            )

        if self._session.win is None:
            raise RuntimeError("window is not opened. Call Session.openWindow() first")

        missingParameters = [
            k for k in self._experiments[key].parameters if k not in kwargs
        ]
        if len(missingParameters) > 0:
            raise ArgumentError(f"missing required parameter(s) {missingParameters}")

        invalidParameters = [
            k for k in kwargs if k not in self._experiments[key].parameters
        ]
        if len(invalidParameters) > 0:
            raise ArgumentError(f"invalid experiment parameter(s) {invalidParameters}")

        if self._resourceChecker.collections[key].valid is False:
            raise RuntimeError(
                f"experiment '{key}' is missing the resource(s) "
                f"{self._resourceChecker.collections[key].missing}"
            )

        kwargs["frameRate"] = self._framerate
        self._session.runExperiment(key, kwargs, blocking=False)

    def stopExperiment(self):
        if self._session.currentExperiment is None:
            raise RuntimeError("no experiment is running")

        self._session.stopExperiment()

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
        try:
            self._session.stop()
        finally:
            pass
        self._observer.stop()
        self._observer.join()
