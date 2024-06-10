import dataclasses
from ctypes import ArgumentError
from pathlib import Path
from typing import Dict, List

from psychopy import session


class ResourceChecker:

    @dataclasses.dataclass
    class CollectionInfo:
        key: str
        resources: Dict[str,bool]

        def validate(self):
            for r in self.resources:
                exists = Path(r).exists()
                self.resources[r] = exists

        @property
        def valid(self):
            all(Path(r).exists() for r in self.resources)

    def __init__(self,root):
        self._root = Path(root).resolve()
        self._resources = {}
        self.collections = {}

    def addResources(self,key,resources):
        self.collections[key] = ResourceChecker.CollectionInfo(
            key= key,resources = {str(Path(r).relative_to(self._root)):False for r in resources})

        self._rebuildReverseMap()
        self.collections[key].validate()

    def removeResources(self,key):
        if key not in self.collections:
            return
        del self.collections[key]
        self._rebuildReverseMap()

    def _rebuildReverseMap(self):
        self._resources = {}
        for [key,resources] in self.collections:
            for r in resources:
                r = Path(r).relative_to(self._root)
                if r not  in self._resources:
                    self._resources[r] = []
                self._resources[r].append(key)

    def revalidate(self,path):
        for key in self._resources.get(path,[]):
            self.collections[key].validate()



class Session:
    def __init__(self, root, _session: session.Session | None = None):
        self._resourceChecker = ResourceChecker(root)
        self._experiments = {}
        if _session is None:
            self._session = session.Session(Path(root).resolve())
        else:
            self._session = _session

    @property
    def experiments(self):
        res = {}
        return res



    def openWindow(self, framerate: int | float | None, **kwargs):
        """Opens the window for the session

        Parameters
        ----------
        Those are passed to psychopy.visual.Window initializor.
        size : tuple
            the size of the window

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
        self._resourceChecker.addResources(key,resources)



    def removeExperiment(self, key):
        if key not in self._experiments:
            raise KeyError(key)
        self._resourceChecker.removeResources(key)
        del self._experiments[key]
        del self._session.experimentObjects[key]
        del self._session.experiments[key]

    def runExperiment(self, key, **kwargs)
        for k in self._experiments[key]:
            if k not in kwargs:
                raise ArgumentError(f"missing expinfo argument '{k}'")

        for k in kwargs:
            if k not in self._experiments[key]:
                raise ArgumentError(f"Invalid expinfo argument '{k}'")


        kwargs['frameRate'] = self._framerate
        self._session.runExperiment(key,kwargs,blocking = False)

    def stopExperiment(self):
        self._session.stopExperiment()

    def close(self):
        try:
            self._session.stop()
        finally:
            pass
