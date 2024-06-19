import argparse


def parse_options(args=None):
    parser = argparse.ArgumentParser(
        prog="psychopy-session-webserver",
        description="Allows PsychoPy session to be run via a REST API",
    )

    parser.add_argument(
        "-l",
        "--listen",
        nargs="*",
        help="address to listen to, defaults to a set of private subnet address",
        default=["192.168.0.0/16", "10.0.0.0/8", "100.64.0.0/10"],
    )

    parser.add_argument(
        "-P", "--port", default=5000, help="port to listen to", type=int
    )

    parser.add_argument(
        "-d",
        "--data-dir",
        help="directory to store the session data, default to <session-dir>/data",
    )
    parser.add_argument(
        "session-dir",
        nargs=1,
        help="directory containing all .psyexp file available in the session",
    )

    return parser.parse_args(args)
