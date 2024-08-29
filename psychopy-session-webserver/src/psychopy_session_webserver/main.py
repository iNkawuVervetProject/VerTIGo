import asyncio
import ipaddress
import logging
import os
import time
from typing import Dict

from pydantic_core import ValidationError, to_json
import structlog
from fastapi import FastAPI, Request
from fastapi.responses import JSONResponse, StreamingResponse
from hypercorn.config import Config
from psychopy.session import asyncio
from pydantic import BaseModel

from psychopy_session_webserver.options import parse_options
from psychopy_session_webserver.server import BackgroundServer
from psychopy_session_webserver.session import Session
from psychopy_session_webserver.types import Catalog, Experiment, Parameter, Participant
from psychopy_session_webserver.utils import format_ns

app = FastAPI()

session: Session = None

windowParams = {}

logLevel = logging.INFO


if "PSYSW_DEBUG" in os.environ:
    windowParams = {"fullscr": False}
    # logLevel = logging.DEBUG

structlog.configure(
    processors=[
        structlog.contextvars.merge_contextvars,
        structlog.processors.add_log_level,
        structlog.processors.StackInfoRenderer(),
        structlog.processors.ExceptionRenderer(),
        structlog.dev.set_exc_info,
        structlog.processors.TimeStamper("iso"),
        structlog.dev.ConsoleRenderer(),
    ],
    wrapper_class=structlog.make_filtering_bound_logger(logLevel),
    context_class=dict,
    logger_factory=structlog.PrintLoggerFactory(),
    cache_logger_on_first_use=False,
)

logger = structlog.get_logger()


@app.middleware("http")
async def log_access(request: Request, call_next):
    slog = logger.bind(
        method=request.method,
        URI=request.url.path,
        client=request.client.host,
        userAgent=request.headers.get("user-agent", "<missing>"),
    )
    request.state.slog = slog
    request.state.start = time.time_ns()

    response = await call_next(request)
    end = time.time_ns()
    if response.status_code < 400:
        await slog.ainfo(
            "OK", status=response.status_code, time=format_ns(end - request.state.start)
        )

    return response


@app.exception_handler(KeyError)
@app.exception_handler(RuntimeError)
@app.exception_handler(ValidationError)
async def handle_session_error(request: Request, exc: Exception):
    end = time.time_ns()
    await request.state.slog.awarn(
        "BAD", exc_info=exc, time=format_ns(end - request.state.start)
    )
    return JSONResponse(status_code=500, content={"detail": str(exc)})


@app.get(
    "/experiments",
    responses={
        200: {
            "description": "List of available experiments.",
            "content": {
                "application/json": {
                    "example": {
                        "blue.psyexp": {
                            "key": "blue.psyexp",
                            "resources": {},
                            "parameters": ["participant", "session"],
                        },
                        "green.psyexp": {
                            "key": "green.psyexp",
                            "resources": {},
                            "parameters": ["participant", "session"],
                        },
                        "complex.psyexp": {
                            "key": "complex.psyexp",
                            "resources": {"image.png": True, "missing.png": False},
                            "parameters": ["participant", "session", "reward"],
                        },
                    }
                }
            },
        },
    },
)
async def get_experiments() -> Catalog:
    return session.experiments


async def send_server_side_event(agen):
    async for event in agen:
        dump = to_json(event.data, indent=None)
        yield f"event:{event.type}\ndata:{dump.decode(encoding='utf-8')}\n\n"


@app.get("/events")
async def get_events():
    return StreamingResponse(
        send_server_side_event(session.updates()), media_type="text/event-stream"
    )


@app.delete("/window")
async def close_window(request: Request) -> None:
    await session.asyncCloseWindow(logger=request.state.slog)


@app.get(
    "/participants",
    responses={
        200: {
            "description": "List of known participants",
            "content": {
                "application/json": {
                    "example": {
                        "asari": Participant(name="asari", nextSession=12345),
                        "turian": Participant(name="turian", nextSession=1),
                    }
                }
            },
        }
    },
)
async def get_participants() -> Dict[str, Participant]:
    return session.participants


class RunExperimentRequest(BaseModel):
    key: str
    parameters: Parameter


@app.post("/experiment")
async def run_experiment(body: RunExperimentRequest, request: Request) -> None:
    await session.asyncRunExperiment(
        body.key, logger=request.state.slog, **body.parameters
    )


@app.delete("/experiment")
async def stop_experiment(request: Request) -> None:
    session.stopExperiment(logger=request.state.slog)


def list_ip_address():
    from netifaces import AF_INET, AF_INET6, ifaddresses, interfaces

    res = []
    for itf in interfaces():
        addresses = ifaddresses(itf)
        inet = addresses.get(AF_INET)
        if inet is not None:
            res.extend([addr["addr"] for addr in inet])

        inet6 = addresses.get(AF_INET6)
        if inet6 is not None:
            res.extend([addr["addr"] for addr in inet6])
    return [ipaddress.ip_address(addr) for addr in res]


host_ip_addresses = list_ip_address()


def validate_address(address):
    if "/" not in address:
        if ipaddress.ip_address(address) not in host_ip_addresses:
            raise RuntimeError(
                f"invalid IP '{address}' : not in host address {host_ip_addresses}"
            )
        return address
    network = ipaddress.ip_network(address)
    validAddress = [ip for ip in host_ip_addresses if ip in network]
    if len(validAddress) > 0:
        return validAddress[0]
    return None


def main():
    global session

    from psychopy import plugins

    plugins.activatePlugins()

    opts = parse_options()

    loop = asyncio.new_event_loop()

    session = Session(root=opts["session_dir"], loop=loop, dataDir=opts["data_dir"])

    config = Config()
    port = opts["port"]

    addresses = [validate_address(l) for l in opts["listen"]]

    config.bind = [f"{addr}:{port}" for addr in addresses if addr is not None]
    config.workers = 1

    server = BackgroundServer(app=app, config=config, loop=loop)

    server.start()
    try:
        session.run()
    except KeyboardInterrupt:
        structlog.get_logger().info("ending gracefully")
    finally:
        session.close()
        server.stop()

    server.join()
