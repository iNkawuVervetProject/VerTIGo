import os
import tempfile
import unittest
from pathlib import Path

from psychopy_session_webserver.dependency_checker import DependencyChecker


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

    def local_filepath(self, p):
        p = Path(p)
        if p.is_absolute():
            return p
        return Path(self.tmpdir.name).joinpath(p)

    def test_validates_on_creation(self):
        self.assertTrue(self.checker.collections["a"].valid)
        self.assertTrue(self.checker.collections["b"].valid)
        self.assertTrue(self.checker.collections["c"].valid)

    def test_revalidate_a_path(self):
        os.remove(self.local_filepath("a"))
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
            os.remove(self.local_filepath(p))
        self.assertTrue(self.checker.collections["a"].valid)
        self.assertTrue(self.checker.collections["b"].valid)
        self.assertTrue(self.checker.collections["c"].valid)
        self.checker.validate(paths)
        self.assertFalse(self.checker.collections["a"].valid)
        self.assertTrue(self.checker.collections["b"].valid)
        self.assertFalse(self.checker.collections["c"].valid)

    def test_works_with_absolute_path(self):
        paths = [
            self.local_filepath("a"),
            "b",
            self.local_filepath("c"),
        ]
        self.checker.addDependencies("d", paths)
        self.assertTrue(self.checker.collections["d"].valid)
        self.assertDictEqual(
            self.checker.collections["d"].resources, {str(p): True for p in paths}
        )

    def test_notifies_validity_changes(self):
        self.assertFalse(self.checker.validate("a"))
        self.assertFalse(self.checker.validate("c"))
        os.remove(self.local_filepath("a"))
        self.assertTrue(self.checker.validate("a"))
        self.assertFalse(self.checker.validate("b"))
        self.assertFalse(self.checker.validate("c"))

        self.assertFalse(self.checker.validate("a"))
        self.assertFalse(self.checker.validate("b"))
        self.assertFalse(self.checker.validate("c"))
