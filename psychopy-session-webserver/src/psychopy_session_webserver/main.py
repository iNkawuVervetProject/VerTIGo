import asyncio
import ipaddress
import logging
import os
import time

import structlog
from fastapi import FastAPI, Request, Response
from fastapi.exceptions import HTTPException
from fastapi.responses import JSONResponse, StreamingResponse
from hypercorn.config import Config
from psychopy.session import asyncio
from pydantic import BaseModel

from psychopy_session_webserver.options import parse_options
from psychopy_session_webserver.server import BackgroundServer
from psychopy_session_webserver.session import Session
from psychopy_session_webserver.types import Catalog, Parameter
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
        userAgent=request.headers["user-agent"] or "<missing>",
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


@app.exception_handler(RuntimeError)
async def handle_session_error(request: Request, exc: RuntimeError):
    end = time.time_ns()
    await request.state.slog.awarn(
        "BAD", exc_info=exc, time=format_ns(end - request.state.start)
    )
    return JSONResponse(status_code=500, content={"detail": str(exc)})


@app.get("/experiments")
async def get_experiments() -> Catalog:
    return session.experiments


async def send_server_side_event(agen):
    async for event in agen:
        if isinstance(event.data, BaseModel):
            dump = event.data.model_dump_json()
        else:
            dump = event.__pydantic_serializer__.to_json(event.data, warnings=False)
        yield f"event:{event.type}\ndata:{dump.decode(encoding='utf-8')}\n\n"


@app.get("/events")
async def get_events():
    return StreamingResponse(
        send_server_side_event(session.updates()), media_type="text/event-stream"
    )


@app.delete("/window")
async def close_window(request: Request):
    session.closeWindow(logger=request.state.slog)


class RunExperimentRequest(BaseModel):
    key: str
    parameter: Parameter


@app.post("/experiment/")
async def run_experiment(body: RunExperimentRequest, request: Request):
    session.runExperiment(body.key, logger=request.state.slog, **body.parameter)


@app.delete("/experiment")
async def stop_experiment(request: Request):
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
        session.sessionLoop()
    except KeyboardInterrupt:
        session.close()
    finally:
        server.stop()
        pass

    server.join()
