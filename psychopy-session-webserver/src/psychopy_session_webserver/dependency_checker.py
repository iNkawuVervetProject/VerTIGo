from pathlib import Path
from typing import Dict, List

from pydantic import BaseModel, computed_field


class DependencyChecker:
    """DependencyChecker efficiently keep record of presence of Dependencies for a
    collection of keys."""

    class CollectionInfo(BaseModel):
        """CollectionInfo lists the dependencies for a resource.

        Attributes
        ----------
        key: str
            the key this CollectionInfo refers to.
        root: pathlib.Path
            the filesystem root the dependencies are (optionally) relative to.

        resources: Dict[str,bool]
            the list of resources this key depends on, a value of true indicates the
            dependencies exists on the filesystem.
        """

        key: str
        root: Path
        resources: Dict[str, bool]

        def __init__(self, key, root, resources):
            super(DependencyChecker.CollectionInfo, self).__init__(
                key=key,
                root=Path(root).resolve(),
                resources={str(r): False for r in resources},
            )
            self.validate()

        def validate(self):
            """(re)Validate presence of dependencies on the filesystem."""
            oldValid = self.valid
            for r in self.resources:
                self.resources[r] = self.filepath(r).exists()
            return oldValid != self.valid

        def filepath(self, path):
            """Given a path-like object, returns its absolute path.

            If the given path is relative, express it relatively from self.root,
            otherwise return the absolute path.

            Arguments
            ---------
            path:
                pathlib.Path like object that is either relative (to the root) or absolute.
            """
            p = Path(path)
            if p.is_absolute():
                return p
            return Path(self.root).joinpath(path)

        @computed_field
        @property
        def valid(self) -> bool:
            return all(exists for exists in self.resources.values())

        @computed_field
        @property
        def missing(self) -> List[str]:
            return [str(r) for r, ok in self.resources.items() if ok is False]

    def __init__(self, root):
        """Intializer

        Parameters
        ----------
        root :
            a pathlib.Path like object that is the root for all dependencies to track.
        """
        self._root = Path(root).resolve()
        self._resources = {}
        self.collections = {}

    def addDependencies(self, key, resources):
        """Adds or updates dependencies for a key.

        Parameter
        ---------
        key: str
            the key to add / update dependencies
        resources: List[str]
            list of relative path to check for presence to consider the key valid.
        """
        self.collections[key] = DependencyChecker.CollectionInfo(
            key=key,
            root=self._root,
            resources=resources,
        )

        self._rebuildReverseMap()

    def removeDependencies(self, key):
        """Removes tracking of dependencies for a key"""
        if key not in self.collections:
            return
        del self.collections[key]
        self._rebuildReverseMap()

    def _rebuildReverseMap(self):
        self._resources = {}
        for [key, info] in self.collections.items():
            for r in info.resources:
                r = self._filepath(r)
                if r not in self._resources:
                    self._resources[r] = []
                self._resources[r].append(key)

    def _filepath(self, path):
        if Path(path).is_absolute() is True:
            return str(path)
        return str(self._root.joinpath(path))

    def validate(self, paths):
        """Validate all keys that depends on a list of paths

        Parameters
        ----------
        paths: str | List[str]
            a str or list of relative path to recompute validity of dependant keys
        """
        if not isinstance(paths, list):
            paths = [paths]
        exps = {}

        for p in paths:
            p = self._filepath(p)
            for exp in self._resources.get(self._filepath(p), []):
                exps[exp] = True

        return [key for key in exps if self.collections[key].validate() is True]
