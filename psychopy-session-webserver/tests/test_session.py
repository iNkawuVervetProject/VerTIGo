import asyncio
import os
import tempfile
import time
import unittest
from contextlib import contextmanager
from pathlib import Path

from psychopy_session_webserver.session import Session
from psychopy_session_webserver.types import Experiment
from psychopy_session_webserver.update_broadcaster import UpdateEvent

from tests.logutils import intercept_structlog
from tests.mock_session import build_mock_session


@intercept_structlog
class SessionTest(unittest.TestCase):
    maxDiff = None

    def setUp(self):
        self.tempdir = tempfile.TemporaryDirectory()
        self.sessionDir = Path(self.tempdir.name).joinpath("session")
        self.psy_session = build_mock_session(self.sessionDir)

        os.makedirs(self.sessionDir)
        self.local_filepath("foo.psyexp").touch()

        self.session = Session(root=self.sessionDir, session=self.psy_session)

    def local_filepath(self, path):
        return self.sessionDir.joinpath(path)

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
                resources={"foo.png": False},
                parameters=["participant", "session"],
            ),
        )

    def test_experiment_list_updates(self):
        self.local_filepath("bar.psyexp").touch()
        time.sleep(0.02)
        self.assertIn("bar.psyexp", self.session.experiments)
        expected = Experiment(
            key="bar.psyexp",
            resources={
                "baz/bar.png": False,
                str(Path(self.tempdir.name).joinpath("absolute.png")): False,
            },
            parameters=["participant", "session", "rewards"],
        )
        got = self.session.experiments["bar.psyexp"]
        self.assertEqual(expected, got, msg=f"expected: {expected}, got: {got}")

    def test_validity_updates(self):
        self.assertFalse(self.session._resourceChecker.collections["foo.psyexp"].valid)
        self.local_filepath("foo.png").touch()
        time.sleep(0.02)
        self.assertTrue(self.session._resourceChecker.collections["foo.psyexp"].valid)
        os.remove(self.local_filepath("foo.png"))
        time.sleep(0.02)
        self.assertFalse(self.session._resourceChecker.collections["foo.psyexp"].valid)

    @contextmanager
    def with_window(self):
        try:
            self.session.openWindow()
            yield
        finally:
            self.session.closeWindow()

    @contextmanager
    def with_file(self, path):
        path = self.local_filepath(path)
        path.touch()
        time.sleep(0.02)
        yield
        os.remove(path)
        time.sleep(0.02)

    def test_run_experiment_asserts_an_opened_window(self):
        with self.assertRaises(RuntimeError) as e:
            self.session.runExperiment("foo.psyexp")

        self.assertEqual(
            str(e.exception), "window is not opened. Call Session.openWindow() first"
        )

    def test_run_experiment_asserts_required_parameters(self):
        with self.with_window(), self.assertRaises(RuntimeError) as e:
            self.session.runExperiment("foo.psyexp")

        self.assertRegex(str(e.exception), "missing required parameter\\(s\\) \\[.*\\]")

    def test_run_experiment_asserts_missing_parameters(self):
        with self.with_window(), self.assertRaises(RuntimeError) as e:
            self.session.runExperiment(
                "foo.psyexp",
                participant="Lolo",
                session=2,
                rewards=5,
            )

        self.assertRegex(
            str(e.exception), "invalid experiment parameter\\(s\\) \\['rewards'\\]"
        )

    def test_run_experiment_assert_resources(self):
        with self.with_window(), self.assertRaises(RuntimeError) as e:
            self.session.runExperiment(
                "foo.psyexp",
                participant="Lolo",
                session=2,
            )

        self.assertEqual(
            str(e.exception),
            "experiment 'foo.psyexp' is missing the resource(s) ['foo.png']",
        )

    def test_run_experiment_assert_none_running(self):
        with self.with_window(), self.with_file("foo.png"):
            self.session.runExperiment(
                "foo.psyexp",
                participant="Lolo",
                session=2,
            )
            with self.assertRaises(RuntimeError) as e:
                self.session.runExperiment(
                    "foo.psyexp",
                    participant="Lolo",
                    session=3,
                )
        self.assertEqual(str(e.exception), "experiment 'foo.psyexp' is already running")

    def test_run_experiment(self):
        with self.with_window(), self.with_file("foo.png"):
            self.session.runExperiment(
                "foo.psyexp",
                participant="Lolo",
                session=2,
            )
        self.psy_session.runExperiment.assert_called_once_with(
            "foo.psyexp",
            {"participant": "Lolo", "session": 2, "frameRate": 30.0},
            blocking=False,
        )


@intercept_structlog
class SessionEventTest(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self) -> None:
        self.loop = asyncio.get_running_loop()

        self.tempdir = tempfile.TemporaryDirectory()
        self.sessionDir = Path(self.tempdir.name).joinpath("session")

        os.makedirs(self.sessionDir)

        self.psy_session = build_mock_session(self.sessionDir)

        self.local_filepath("foo.psyexp").touch()

        self.session = Session(
            root=self.sessionDir, session=self.psy_session, loop=self.loop
        )
        self.updates = self.session.updates()

        event = await anext(self.updates)
        self.assertEqual(event.type, "catalogUpdate")
        self.assertDictEqual(
            event.data,
            {
                "foo.psyexp": Experiment(
                    key="foo.psyexp",
                    parameters=["participant", "session"],
                    resources={"foo.png": False},
                )
            },
        )
        event = await anext(self.updates)
        self.assertEqual(UpdateEvent(type="experimentUpdate", data=""), event)

        event = await anext(self.updates)
        self.assertEqual(UpdateEvent(type="windowUpdate", data=False), event)

    async def asyncTearDown(self) -> None:
        self.session.close()
        self.tempdir.cleanup()

    def local_filepath(self, path):
        return self.sessionDir.joinpath(path)

    async def test_catalog_updates(self):

        self.local_filepath("foo.png").touch()
        event = await anext(self.updates)
        self.assertEqual(event.type, "catalogUpdate")
        self.assertDictEqual(
            event.data,
            {
                "foo.psyexp": Experiment(
                    key="foo.psyexp",
                    parameters=["participant", "session"],
                    resources={"foo.png": True},
                )
            },
        )

        os.remove(self.local_filepath("foo.psyexp"))
        event = await anext(self.updates)
        self.assertEqual(event.type, "catalogUpdate")
        self.assertDictEqual({}, event.data)

    async def test_window_updates(self):

        self.session.openWindow()
        event = await anext(self.updates)
        self.assertEqual(event.type, "windowUpdate")
        self.assertEqual(event.data, True)

        self.session.closeWindow()
        event = await anext(self.updates)
        self.assertEqual(event.type, "windowUpdate")
        self.assertEqual(event.data, False)

    async def test_experiment_update(self):
        self.local_filepath("foo.png").touch()
        event = await anext(self.updates)
        self.assertEqual(event.type, "catalogUpdate")

        self.session.openWindow()

        event = await anext(self.updates)
        self.assertEqual(event.type, "windowUpdate")
        self.assertEqual(event.data, True)

        self.session.runExperiment(
            "foo.psyexp",
            participant="Lolo",
            session=2,
        )

        event = await anext(self.updates)
        self.assertEqual(event.type, "experimentUpdate")
        self.assertEqual(event.data, "foo.psyexp")

        event = await anext(self.updates)
        self.assertEqual(event.type, "experimentUpdate")
        self.assertEqual(event.data, "")
