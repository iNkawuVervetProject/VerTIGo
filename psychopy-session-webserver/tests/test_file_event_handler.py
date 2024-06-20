import io
import os
import tempfile
import time
import unittest
from pathlib import Path
from unittest.mock import Mock, call

from psychopy_session_webserver.file_event_handler import FileEventHandler
from watchdog.observers import Observer

from tests.logutils import intercept_structlog


@intercept_structlog
class FileEventHandlerTest(unittest.TestCase):

    def setUp(self):
        self.tempdir = tempfile.TemporaryDirectory()
        self.observer = Observer()
        self.mock = Mock()
        self.handler = FileEventHandler(
            session=self.mock,
            root=self.tempdir.name,
        )
        self.observer = Observer()
        self.observer.schedule(self.handler, self.tempdir.name, recursive=True)
        self.observer.start()

    def tearDown(self):
        self.observer.stop()
        self.observer.join()

        self.tempdir.cleanup()
        del self.mock

    def local_filepath(self, path):
        return Path(self.tempdir.name).joinpath(path)

    def modify(self, path):
        with open(self.local_filepath(path), "w+") as f:
            f.write("One more thing\n")

    def test_add_experiment(self):
        self.local_filepath("blue.psyexp").touch()
        time.sleep(0.02)

        self.mock.addExperiment.assert_called_once_with(file="blue.psyexp")

    def test_add_experiment_only_once(self):
        p = self.local_filepath("once.psyexp")

        with open(p, "w+") as f:
            f.write("One line\n")

        with open(p, "r") as f:
            self.assertEqual(f.readlines(), ["One line\n"])

        time.sleep(0.02)
        self.mock.addExperiment.assert_called_once_with(file="once.psyexp")

    def test_remove_experiment(self):
        p = self.local_filepath("remove.psyexp")
        p.touch()
        os.remove(p)
        time.sleep(0.02)
        self.mock.addExperiment.assert_called_once()
        self.mock.removeExperiment.assert_called_once_with(key="remove.psyexp")

    def test_move_experiment(self):
        src = self.local_filepath("src.psyexp")

        dest = self.local_filepath("dest.psyexp")
        src.touch()
        os.rename(src, dest)
        time.sleep(0.02)
        self.mock.assert_has_calls([
            call.addExperiment(file="src.psyexp"),
            call.removeExperiment(key="src.psyexp"),
            call.addExperiment(file="dest.psyexp"),
        ])

    def test_add_non_experiment(self):
        self.local_filepath("foo").touch()
        time.sleep(0.02)
        self.mock.validateResources.assert_called_with(paths=["foo"])

    def test_remove_non_experiment(self):
        self.local_filepath("foo").touch()
        os.remove(self.local_filepath("foo"))
        time.sleep(0.02)
        self.mock.validateResources.assert_has_calls(
            [call(paths=["foo"]), call(paths=["foo"])]
        )

    def test_move_non_experiment(self):
        self.local_filepath("foo").touch()
        os.rename(src=self.local_filepath("foo"), dst=self.local_filepath("bar"))
        time.sleep(0.02)
        self.mock.assert_has_calls([
            call.validateResources(paths=["foo"]),
            call.validateResources(paths=["foo", "bar"]),
        ])


if __name__ == "__main__":
    unittest.main(verbosity=42)
