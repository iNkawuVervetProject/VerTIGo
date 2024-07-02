import unittest

from psychopy_session_webserver.utils import format_ns

MICROSECONDS = 1_000
MILLISECONDS = 1_000_000
SECONDS = 1_000_000_000
MINUTES = 60 * SECONDS
HOURS = 60 * MINUTES


class DurationTest(unittest.TestCase):

    def test_format(self):
        data = [
            ("0s", 0),
            ("1ns", 1),
            ("1.1µs", 1_100),
            ("2.2ms", 2_200 * MICROSECONDS),
            ("3.3s", 3_300 * MILLISECONDS),
            ("-4.4s", -4_400 * MILLISECONDS),
            ("4m5s", 4 * MINUTES + 5 * SECONDS),
            ("4m5.001s", 4 * MINUTES + 5001 * MILLISECONDS),
            ("5h6m7.001s", 5 * HOURS + 6 * MINUTES + 7 * SECONDS + 1 * MILLISECONDS),
            ("8m1e-09s", 8 * MINUTES + 1),
        ]
        for expected, d in data:
            with self.subTest(expected):
                self.assertEqual(expected, format_ns(d))
