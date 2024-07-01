from glob import glob
from pathlib import Path

from numpy import true_divide
from psychopy_session_webserver.types import Participant
from psychopy_session_webserver.update_broadcaster import UpdateBroadcaster
from pydantic_core import to_json, from_json


import xdg


class ParticipantUpdater:

    def __init__(self, updates: UpdateBroadcaster, dataDir=None):
        self._updates = updates
        self._participants = dict[str, Participant]({})
        self._load_from_xdg()
        self._update_from_data(dataDir)
        self._updates.broadcast("participants", self._participants)

    def __setitem__(self, name: str, session: int) -> None:
        updated = self._participants.get(
            name, Participant(name=name, nextSession=0)
        ).update(session)

        if updated == False:
            return

        self._save_to_xdg()
        self._updates.broadcastDict("participants", name, session)

    def __getitem__(self, name: str) -> Participant:
        return self._participants[name]

    _filepath = Path(
        xdg.BaseDirectory.save_data_path("psychopy_session_webserver")
    ).joinpath("participants.json")

    def _load_from_xdg(self):
        if ParticipantUpdater._filepath.exists() == False:
            return

        with open(ParticipantUpdater._filepath, "r") as f:
            self._participants.update(
                {k: Participant(**v) for k, v in from_json(f.read()).items()}
            )

    def _save_to_xdg(self):
        with open(ParticipantUpdater._filepath, "wb") as f:
            f.write(to_json(self._participants, indent=2))

    def _update_from_data(self, dataDir):
        participants = {}
        for f in glob(str(Path(dataDir).joinpath("**/*.psydat"))):
            name = Path(f).stem.split("_")[0]
            participants[name] = participants.get(name, 1) + 1

        for name, nextSession in participants.items():
            self._participants.get(name, Participant(name=name, nextSession=1)).update(
                nextSession
            )
