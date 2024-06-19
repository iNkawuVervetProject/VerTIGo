import argparse


def parse_options(args=None):

    class OverrideDefaultAppend(argparse.Action):
        def __init__(self, option_strings, dest, **kwargs):
            self._set = False
            super().__init__(option_strings, dest, **kwargs)

        def __call__(self, parser, namespace, values, option_string=None):
            if self._set is False:
                self._set = True
                setattr(namespace, self.dest, [])
            getattr(namespace, self.dest).append(values)

    parser = argparse.ArgumentParser(
        prog="psychopy-session-webserver",
        description="Allows PsychoPy session to be run via a REST API",
    )

    parser.add_argument(
        "-l",
        "--listen",
        help="address to listen to, defaults to a set of private subnet address",
        default=["192.168.0.0/16", "10.0.0.0/8", "100.64.0.0/10"],
        action=OverrideDefaultAppend,
    )

    parser.add_argument(
        "-P", "--port", default=5000, help="port to listen to", type=int
    )

    parser.add_argument(
        "-d",
        "--data-dir",
        help="directory to store the session data, default to <session_dir>/data",
    )
    parser.add_argument(
        "session_dir",
        help="directory containing all .psyexp file available in the session",
    )

    return vars(parser.parse_args(args))
