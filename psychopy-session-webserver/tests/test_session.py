import os
import tempfile
import time
import unittest
from ctypes import ArgumentError
from pathlib import Path
from unittest.mock import Mock, PropertyMock

from src.psychopy_session_webserver import Experiment, Session


class SessionTest(unittest.TestCase):
    def setUp(self):
        self.tempdir = tempfile.TemporaryDirectory()
        self.psy_session = Mock()
        self.psy_session.root = self.tempdir.name

        foo = Mock()
        bar = Mock()
        self.psy_session.experimentObjects = {
            "foo.psyexp": foo,
            "bar.psyexp": bar,
        }
        foo.getResourceFiles.return_value = ["foo"]
        bar.getResourceFiles.return_value = ["bar"]

        expInfos = {
            "foo.psyexp": {
                "participant": 123,
                "session": 1,
                "date|hid": "foo",
                "psychopy_version|hid": "bar",
            },
            "bar.psyexp": {
                "participant": 123,
                "session": 1,
                "rewards": 3,
                "date|hid": "foo",
                "psychopy_version|hid": "bar",
            },
        }

        def getExpInfos(key):
            return expInfos[key]

        self.psy_session.getExpInfoFromExperiment.side_effect = getExpInfos

        self.local_filepath("foo.psyexp").touch()

        self.session = Session(root=self.tempdir.name, session=self.psy_session)

    def local_filepath(self, path):
        return Path(self.tempdir.name).joinpath(path)

    def tearDown(self):
        self.session.close()
        self.tempdir.cleanup()
        del self.session

    def test_existing_experiment_are_listed(self):
        self.assertIn("foo.psyexp", self.session.experiments)
        self.assertEqual(
            self.session.experiments["foo.psyexp"],
            Experiment(
                key="foo.psyexp",
                resources={"foo": False},
                parameters=["participant", "session"],
            ),
        )

    def test_experiment_list_updates(self):
        self.local_filepath("bar.psyexp").touch()
        time.sleep(0.02)
        self.assertIn("bar.psyexp", self.session.experiments)
        self.assertEqual(
            self.session.experiments["bar.psyexp"],
            Experiment(
                key="bar.psyexp",
                resources={"bar": False},
                parameters=["participant", "session", "rewards"],
            ),
        )

    def test_validity_updates(self):
        self.assertFalse(self.session._resourceChecker.collections["foo.psyexp"].valid)
        self.local_filepath("foo").touch()
        time.sleep(0.02)
        self.assertTrue(self.session._resourceChecker.collections["foo.psyexp"].valid)
        os.remove(self.local_filepath("foo"))
        time.sleep(0.02)
        self.assertFalse(self.session._resourceChecker.collections["foo.psyexp"].valid)

    def test_can_run_experiment(self):
        with self.assertRaises(ArgumentError) as e:
            self.session.runExperiment("foo.psyexp")

        self.assertRegex(str(e.exception), "missing required parameter\\(s\\) \\[.*\\]")

        with self.assertRaises(ArgumentError) as e:
            self.session.runExperiment(
                "foo.psyexp",
                participant="Lolo",
                session=2,
                rewards=5,
            )

        self.assertRegex(
            str(e.exception), "invalid experiment parameter\\(s\\) \\['rewards'\\]"
        )

        self.session.runExperiment(
            "foo.psyexp",
            participant="Lolo",
            session=2,
        )
