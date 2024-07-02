from glob import glob
from pathlib import Path

from numpy import true_divide
from pydantic import ValidationError
from structlog import get_logger
from psychopy_session_webserver.types import Participant
from psychopy_session_webserver.update_broadcaster import UpdateBroadcaster
from pydantic_core import to_json, from_json


from xdg import BaseDirectory


class ParticipantRegistry:

    def __init__(self, updates: UpdateBroadcaster, dataDir=None):
        self._logger = get_logger().bind(module="ParticipantRegistry")
        self._updates = updates
        self._participants = dict[str, Participant]({})
        self._load_from_xdg()
        self._update_from_data(dataDir)
        self._updates.broadcast("participants", self._participants)

    def __setitem__(self, name: str, session: int) -> None:
        updated = False
        if name not in self._participants:
            self._participants[name] = Participant(name=name, nextSession=session)
            updated = True

        if updated == False and self._participants[name].update(session) == False:
            return

        self._save_to_xdg()
        self._updates.broadcastDict("participants", name, self._participants[name])

    def __getitem__(self, name: str) -> Participant:
        return self._participants[name]

    _filepath = Path(
        BaseDirectory.save_data_path("psychopy_session_webserver")
    ).joinpath("participants.json")

    def _load_from_xdg(self):
        if ParticipantRegistry._filepath.exists() == False:
            return
        self._logger.info(
            "opening persistent file", path=str(ParticipantRegistry._filepath)
        )
        with open(ParticipantRegistry._filepath, "r") as f:
            for k, v in from_json(f.read()).items():
                try:
                    self._participants[k] = Participant(**v)
                except ValidationError as e:
                    self._logger.warn("invalid data", key=k, **v)

    def _save_to_xdg(self):
        with open(ParticipantRegistry._filepath, "wb") as f:
            f.write(to_json(self._participants, indent=2))

    def _update_from_data(self, dataDir):
        nextSessionBounds = {}
        for f in glob(str(Path(dataDir or "data").joinpath("**/*.psydat"))):
            # this formula below is only valid when Participant.name validation forbids
            # the use of '_' in the name, this is the case as it as a pattern
            # r'^[a-zA-Z0-9\-]+$' set.
            name = Path(f).stem.split("_")[0]

            nextSessionBounds[name] = nextSessionBounds.get(name, 1) + 1

        for name, nextSession in nextSessionBounds.items():
            if name not in self._participants:
                self._participants[name] = Participant(name=name, nextSession=1)
            self._participants[name].update(nextSession)
