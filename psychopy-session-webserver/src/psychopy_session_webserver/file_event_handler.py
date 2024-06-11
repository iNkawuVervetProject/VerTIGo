from pathlib import Path

from structlog import configure, get_logger
from structlog.stdlib import LoggerFactory
from watchdog import events


class FileEventHandler(events.FileSystemEventHandler):
    def __init__(self, session, root):
        configure(logger_factory=LoggerFactory())
        self.session = session
        self.root = root
        self.modified = {}
        self.logger = get_logger()

    def on_any_event(self, event: events.FileSystemEvent) -> None:
        deletedPaths = []
        modifiedPaths = []
        self.logger.debug("new event", file_event=event)

        if event.is_directory == True:
            return

        if isinstance(event, events.FileMovedEvent):
            deletedPaths.append(event.src_path)
            modifiedPaths.append(event.dest_path)
        elif isinstance(event, events.FileDeletedEvent):
            deletedPaths.append(event.src_path)
        elif isinstance(event, events.FileModifiedEvent) or isinstance(
            event, events.FileCreatedEvent
        ):
            self.modified[event.src_path] = True
        elif isinstance(event, events.FileClosedEvent):
            if event.src_path in self.modified:
                modifiedPaths.append(event.src_path)
                del self.modified[event.src_path]

        toValidate = []

        for p in deletedPaths:
            if Path(p).suffix == ".psyexp":
                self.session.removeExperiment(key=str(Path(p).relative_to(self.root)))
            else:
                toValidate.append(p)

        for p in modifiedPaths:
            if Path(p).suffix == ".psyexp":
                self.session.addExperiment(file=str(Path(p).relative_to(self.root)))
            else:
                toValidate.append(p)

        if len(toValidate) > 0:
            self.session.validateResources(
                paths=[str(Path(p).relative_to(self.root)) for p in toValidate]
            )
