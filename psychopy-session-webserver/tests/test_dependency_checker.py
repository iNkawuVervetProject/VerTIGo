import os
import tempfile
import unittest
from pathlib import Path

from src.psychopy_session_webserver.dependency_checker import DependencyChecker


class DependencyCheckerTest(unittest.TestCase):
    def setUp(self):
        self.tmpdir = tempfile.TemporaryDirectory()
        for f in ["a", "b", "c"]:
            Path(self.tmpdir.name).joinpath(f).touch()

        deps = {
            "a": ["a"],
            "b": ["b"],
            "c": ["a", "b", "c"],
        }

        self.checker = DependencyChecker(self.tmpdir.name)
        for [key, files] in deps.items():
            self.checker.addDependencies(
                key, [Path(self.tmpdir.name).joinpath(f) for f in files]
            )

    def tearDown(self):
        self.tmpdir.cleanup()

    def test_validates_on_creation(self):
        self.assertTrue(self.checker.collections["a"].valid)
        self.assertTrue(self.checker.collections["b"].valid)
        self.assertTrue(self.checker.collections["c"].valid)

    def test_revalidate_a_path(self):
        os.remove(Path(self.tmpdir.name).joinpath("a"))
        self.assertTrue(self.checker.collections["a"].valid)
        self.assertTrue(self.checker.collections["b"].valid)
        self.assertTrue(self.checker.collections["c"].valid)
        self.checker.validate("a")
        self.assertFalse(self.checker.collections["a"].valid)
        self.assertTrue(self.checker.collections["b"].valid)
        self.assertFalse(self.checker.collections["c"].valid)

    def test_revalidate_a_list_of_path(self):
        paths = ["a", "c"]
        for p in paths:
            os.remove(Path(self.tmpdir.name).joinpath(p))
        self.assertTrue(self.checker.collections["a"].valid)
        self.assertTrue(self.checker.collections["b"].valid)
        self.assertTrue(self.checker.collections["c"].valid)
        self.checker.validate(paths)
        self.assertFalse(self.checker.collections["a"].valid)
        self.assertTrue(self.checker.collections["b"].valid)
        self.assertFalse(self.checker.collections["c"].valid)
