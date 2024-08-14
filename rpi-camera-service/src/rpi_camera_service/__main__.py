from contextlib import contextmanager
import time
from threading import Thread
from rpi_camera_service import app

import uvicorn
from argparse import ArgumentParser

from rpi_camera_service.camera_stream import CameraParameter, CameraStream


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
        uvicorn.run(app, port=5041, log_level="info", host="127.0.0.1")
    except KeyboardInterrupt:
        pass


parser = ArgumentParser(
    prog="rpi_camera_service",
    description="exposes rpi camera recording as a webservice",
)

parser.set_defaults(func=serve)
subparsers = parser.add_subparsers()

parser_serve = subparsers.add_parser("serve")
parser_serve.set_defaults(func=serve)

parser_stream = subparsers.add_parser("stream")
parser_stream.set_defaults(func=stream)
parser_stream.add_argument(
    "--testvideo", action="store_true", default=False, help="Use videotestsrc"
)

args = parser.parse_args()

args.func(args)
