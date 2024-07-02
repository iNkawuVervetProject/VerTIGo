import os
from tempfile import TemporaryDirectory
import unittest

from pathlib import Path
from unittest.mock import Mock

from pydantic import ValidationError

from psychopy_session_webserver.participants_registry import ParticipantRegistry
from psychopy_session_webserver.types import Participant


class ParticipantRegitryTest(unittest.TestCase):

    def setUp(self):
        tempdir = TemporaryDirectory()
        self.addCleanup(tempdir.cleanup)
        self.tempdir = tempdir.name

        actualXdg = ParticipantRegistry._filepath
        ParticipantRegistry._filepath = Path(self.tempdir).joinpath(
            "xdg_data_home/participants.json"
        )

        def reset():
            ParticipantRegistry._filepath = actualXdg

        self.addCleanup(reset)

        os.makedirs(ParticipantRegistry._filepath.parent)

        self.dataDir = Path(self.tempdir).joinpath("data")
        self.updates = Mock()
        self.registry = ParticipantRegistry(self.updates, self.dataDir)

    def test_next_session_only_increase(self):
        self.registry["foo"] = 1
        self.assertEqual(self.registry["foo"], Participant(name="foo", nextSession=1))

        self.registry["foo"] = 3
        self.assertEqual(self.registry["foo"], Participant(name="foo", nextSession=3))

        self.registry["foo"] = 2
        self.assertEqual(self.registry["foo"], Participant(name="foo", nextSession=3))

    def test_persist_data(self):
        self.registry["foo"] = 1
        self.assertEqual(self.registry["foo"], Participant(name="foo", nextSession=1))

        del self.registry

        self.registry = ParticipantRegistry(self.updates, self.dataDir)
        self.assertEqual(self.registry["foo"], Participant(name="foo", nextSession=1))

    def test_starts_empty(self):
        with self.assertRaises(KeyError):
            self.registry["does-not-exist"]

    def test_updates(self):
        self.updates.broadcast.assert_called_once_with("participants", {})

        self.registry["foo"] = 3

        self.updates.broadcastDict.assert_called_once_with(
            "participants", "foo", Participant(name="foo", nextSession=3)
        )

        self.updates.reset_mock()
        self.registry["foo"] = 2
        self.updates.broadcastDict.assert_not_called()

        del self.registry
        self.registry = ParticipantRegistry(self.updates, self.dataDir)

        self.updates.broadcast.assert_called_once_with(
            "participants", {"foo": Participant(name="foo", nextSession=3)}
        )

    def test_participant_name_validation(self):
        with self.assertRaises(ValidationError) as e:
            self.registry["Some weird_name#"] = 3

        self.assertRegex(str(e.exception), "1 validation error for Participant")

    def test_participant_session_validation(self):
        with self.assertRaises(ValidationError) as e:
            self.registry["foo"] = 0

        self.assertRegex(str(e.exception), "1 validation error for Participant")
