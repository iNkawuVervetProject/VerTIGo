from pathlib import Path
from typing import Any
from watchdog import observers
from watchdog.events import FileDeletedEvent, FileSystemEvent, FileSystemEventHandler
import yaml


class ResourceManager:

    class EventHandler(FileSystemEventHandler):
        def __init__(self, m):
            self._manager = m

        def on_any_events(self, event: FileSystemEvent):
            p = Path(str(event.src_path))

            if p.suffix != ".yml" and p.suffix != ".yaml":
                return

            if isinstance(event, FileDeletedEvent):
                self._manager.remove(p.stem)
            else:
                self._manager.update(p.stem, p)

    def __init__(self, basedir):
        self._data = {}
        self._observer = observers.Observer()
        self._event_handler = ResourceManager.EventHandler(self)
        self._observer.schedule(self._event_handler, basedir)
        self._observer.start()
        for p in Path(basedir).glob("*.yaml"):
            self.update(p.stem, str(p))
        for p in Path(basedir).glob("*.yml"):
            self.update(p.stem, str(p))

    def close(self):
        self._observer.stop()
        self._observer.join()

    def getResource(self, key: str) -> Any:
        return self._data[key]

    def update(self, key: str, path: str):
        try:
            with open(path, "r") as file:
                self._data[key] = yaml.safe_load(file)
        finally:
            pass

    def remove(self, key: str):
        if key not in self._data:
            return
        del self._data[key]
