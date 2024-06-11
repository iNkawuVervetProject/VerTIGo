from pathlib import Path
from typing import Dict

from pydantic import BaseModel, computed_field


class DependencyChecker:
    """DependencyChecker keep record of presence of Dependencies for a collection of keys"""

    class CollectionInfo(BaseModel):
        key: str
        root: str
        resources: Dict[str, bool]

        def __init__(self, key, root, resources):
            resourcesDict = {str(r): Path(root).joinpath(r).exists() for r in resources}
            super(DependencyChecker.CollectionInfo, self).__init__(
                key=key, root=str(root), resources=resourcesDict
            )

        def validate(self):
            self.resources = dict(
                {r: Path(self.root).joinpath(r).exists() for r in self.resources}
            )

        @computed_field
        @property
        def valid(self) -> bool:
            return all(exists for exists in self.resources.values())

    def __init__(self, root):
        """Intializer

        Parameters
        ----------
        root : str | pathlib.Path
            the root for all dependencies to track
        """
        self._root = Path(root).resolve()
        self._resources = {}
        self.collections = {}

    def _ensure_relative(self, p):
        if Path(p).is_absolute() is False:
            return p
        return Path(p).resolve().relative_to(self._root)

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
            resources=[self._ensure_relative(r) for r in resources],
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
                if r not in self._resources:
                    self._resources[r] = []
                self._resources[r].append(key)

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
            for exp in self._resources.get(self._ensure_relative(p), []):
                exps[exp] = True

        for key in exps:
            self.collections[key].validate()
