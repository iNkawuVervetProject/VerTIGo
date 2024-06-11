from pathlib import Path
from typing import Dict
from pydantic import BaseModel, computed_field


class DependencyChecker:
    class CollectionInfo(BaseModel):
        key: str
        resources: Dict[str, bool]

        def __init__(self, key, resources):
            resourcesDict = {str(r): os.Path(r).exists() for r in resources}
            super(DependencyChecker.CollectionInfo, self).__init__(
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
