import unittest

from psychopy_session_webserver.main import parse_options


class ScriptArgumentTest(unittest.TestCase):

    def test_directory_is_mandatory(self):
        with self.assertRaises(SystemExit) as e:
            parse_options([])

    def test_default(self):
        opts = parse_options(["./foo"])
        self.assertDictEqual(
            {
                "session_dir": "./foo",
                "listen": ["192.168.0.0/16", "10.0.0.0/8", "100.64.0.0/10"],
                "port": 5000,
                "data_dir": None,
            },
            opts,
        )

    def test_listen_overrrides_default(self):
        opts = parse_options(["-l", "0.0.0.0", "-l", "::", "./foo"])
        self.assertDictEqual(
            {
                "session_dir": "./foo",
                "listen": ["0.0.0.0", "::"],
                "port": 5000,
                "data_dir": None,
            },
            opts,
        )
