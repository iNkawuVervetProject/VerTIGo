from contextlib import contextmanager
import time
from threading import Thread
from inkawuvp_vertigo_camera import app

import uvicorn
from argparse import ArgumentParser

from inkawuvp_vertigo_camera.camera_stream import CameraParameter, CameraStream


def stream(args):
    stream = CameraStream(CameraParameter(), debug=args.testvideo)
    thread = Thread(target=stream.run)
    thread.start()
    try:
        while True:
            time.sleep(3)
    except KeyboardInterrupt:
        stream.stop()
    finally:
        thread.join()


def serve(args):
    try:
        uvicorn.run(app, port=args.port, log_level="info", host=args.host)
    except KeyboardInterrupt:
        pass


parser = ArgumentParser(
    prog="rpi_camera_service",
    description="exposes rpi camera recording as a webservice",
)

# parser.set_defaults(func=serve)
subparsers = parser.add_subparsers(help="command help")

args_serve = subparsers.add_parser(
    "serve",
    help="starts a webservice exposing a camera stream on demand",
)
args_serve.set_defaults(func=serve)
args_serve.add_argument(
    "--host", default="127.0.0.1", help="address to listen to (default: 127.0.0.1)"
)
args_serve.add_argument(
    "--port", default=5040, help="port to listen to (default: 5040)"
)
args_stream = subparsers.add_parser(
    "stream",
    help="directly start a stream from commandline without exposing a webserver",
)
args_stream.set_defaults(func=stream)
args_stream.add_argument(
    "--testvideo", action="store_true", default=False, help="Use videotestsrc"
)


args = parser.parse_args()
if "func" in args:
    args.func(args)
else:
    parser.print_help()
