from pathlib import Path

import structlog
from watchdog import events


class FileEventHandler(events.FileSystemEventHandler):
    """A watchdog.events.FileEventHandler for a psychopy_session_webserver.Session

    FileEventHandler monitors an experiment directory and add/remove experiment from the
    session as it triggers resource file validation by monitoring the underlying
    FileSystem change.

    Attributes
    ----------
    session: Session
        the Session that will be notified of any change to the Filesystem
    root: str | pathlib.Path
        the folder to watch for change. any experiment or resource notification will be
    relative path to root

    """

    def __init__(self, session, root, logger=None):
        if logger is None:
            logger = structlog.get_logger()
        self.session = session
        self.root = root
        self.modified = {}
        self.logger = logger.bind(module="FileEvent", root=root)

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
                try:
                    key = str(Path(p).relative_to(self.root))
                    self.session.removeExperiment(key=key)
                except Exception as err:
                    self.logger.error(
                        "could not remove experiment:", error=err, key=key
                    )

            else:
                toValidate.append(p)

        for p in modifiedPaths:
            if Path(p).suffix == ".psyexp":
                try:
                    key = str(Path(p).relative_to(self.root))
                    self.session.addExperiment(file=str(Path(p).relative_to(self.root)))
                except Exception as err:
                    self.logger.error(
                        "could not remove experiment:", error=err, key=key
                    )
            else:
                toValidate.append(p)

        if len(toValidate) > 0:
            try:
                self.session.validateResources(
                    paths=[str(Path(p).relative_to(self.root)) for p in toValidate]
                )
            except Exception as err:
                self.logger.error("could not validate resources", error=err)
