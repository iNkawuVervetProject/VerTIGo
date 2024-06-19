import unittest

from psychopy_session_webserver.main import parse_options


class ScriptArgumentTest(unittest.TestCase):

    def test_directory_is_mandatory(self):
        with self.assertRaises(SystemExit) as e:
            parse_options([])

    def test_directory(self):
        opts = parse_options(["./foo"])
        self.assertEqual(opts.session_dir, "./foo")
