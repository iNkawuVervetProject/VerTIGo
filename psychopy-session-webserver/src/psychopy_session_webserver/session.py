from ctypes import ArgumentError
from pathlib import Path
from typing import Dict, List
from pydantic import BaseModel, computed_field

from psychopy import session
from watchdog import Observer, events


class DependencyChecker:
    class CollectionInfo(BaseModel):
        key: str
        resources: Dict[str, bool]

        def __init__(self, key, resources):
            resourcesDict = {str(r): os.Path(r).exists() for r in resources}
            self.super(DependencyChecker.CollectionInfo, self).__init__(
                key=key, resources=resourcesDict
            )

        def validate(self):
            self.resources = dict({r: Path(r).exists() for r in self.resources})

        @property
        @computed_field
        def valid(self):
            all(exists for exists in self.resources.values())

    def __init__(self, root):
        self._root = Path(root).resolve()
        self._resources = {}
        self.collections = {}

    def addDependencies(self, key, resources):
        self.collections[key] = DependencyChecker.CollectionInfo(
            key=key,
            resources={str(Path(r).relative_to(self._root)): False for r in resources},
        )

        self._rebuildReverseMap()

    def removeResources(self, key):
        if key not in self.collections:
            return
        del self.collections[key]
        self._rebuildReverseMap()

    def _rebuildReverseMap(self):
        self._resources = {}
        for [key, resources] in self.collections:
            for r in resources:
                r = Path(r).relative_to(self._root)
                if r not in self._resources:
                    self._resources[r] = []
                self._resources[r].append(key)

    def revalidate(self, paths):
        if not isinstance(paths, list):
            paths = [paths]
        exps = {exp: True for exp in self._resources.get(p, []) for p in paths}
        for key in exps:
            self.collections[key].validate()


class Experiment(BaseModel):
    key: str
    resources: Dict[str, bool]
    parameter: List[str]


class Session:
    class EventHandler(events.FileSystemEventHandler):
        def __init__(self, session):
            self.session = session

        def on_any_event(self, event: events.FileSystemEvent) -> None:
            deletedPaths = []
            modifiedPaths = []
            if isinstance(event, events.FileMovedEvent):
                deletedPaths.append(event.src_path)
                modifiedPaths.append(event.dest_path)
            elif isinstance(event, events.FileDeletedEvent):
                deletedPaths.append(event.src_path)
            else:
                modifiedPaths.append(event.src_path)

            for p in deletedPaths:
                if Path(p).suffix == ".psyexp":
                    self.session.removeExperiment(p)
                else:
                    self.session._resourceChecker.validate(p)

            for p in modifiedPaths:
                if Path(p).suffix == ".psyexp":
                    self.session.addExperiment(p)
                else:
                    self.session._resourceChecker.validate(p)

    def __init__(self, root, _session: session.Session | None = None):
        root = Path(root).resolve()
        self._resourceChecker = DependencyChecker(root)
        self._experiments = {}
        if _session is None:
            self._session = session.Session(root)
        else:
            self._session = _session

        self._observer = Observer()
        self._observer.schedule(self, root, recursive=True)
        self._observer.start()

    @property
    def experiments(self) -> dict[str, Experiment]:
        return self._experiments

    def openWindow(self, framerate: int | float | None, **kwargs):
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

    def closeWindow(self):
        if self._session.win is None:
            return
        self._session.win.close()
        self._session.win = None

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
        if key is None:
            key = str(Path(file).relative_to(self._session.root))

        self._session.addExperiment(file, key)
        resources = self._session.experimentObjects[key].getResourceFiles()
        self._resourceChecker.addDependencies(key, resources)
        self._experiments[key] = self._buildExperimentInfo(key)

    def _buildExperimentInfo(self, key):
        expInfo = self._session.getExpInfoFromExperiment(key)

        return Experiment(
            key=key,
            resources=self._resourceChecker.collections[key].resources,
            parameter=[p for p in expInfo if str(p).endswith("|hid") == False],
        )

    def removeExperiment(self, key):
        if key not in self._experiments:
            raise KeyError(key)
        self._resourceChecker.removeResources(key)
        del self._experiments[key]
        del self._session.experimentObjects[key]
        del self._session.experiments[key]

    def runExperiment(self, key, **kwargs):
        missingParameters = [
            k for k in self._experiments[key].parameters if k not in kwargs
        ]
        if len(missingParameters) > 0:
            raise ArgumentError(f"missing required parameter(s) '{missingParameters}'")

        invalidParameters = [
            k for k in kwargs if k not in self._experiments[key].parameters
        ]
        if len(invalidParameters) > 0:
            raise ArgumentError(
                f"invalid experiment parameter(s) '{invalidParameters}'"
            )

        kwargs["frameRate"] = self._framerate
        self._session.runExperiment(key, kwargs, blocking=False)

    def stopExperiment(self):
        self._session.stopExperiment()

    def close(self):
        try:
            self._session.stop()
        finally:
            pass
        self._observer.stop()
        self._observer.join()
