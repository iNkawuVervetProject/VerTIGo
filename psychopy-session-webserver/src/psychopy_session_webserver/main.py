import asyncio
import ipaddress

import orjson
from fastapi import FastAPI, HTTPException
from fastapi.responses import StreamingResponse
from hypercorn.config import Config
from psychopy.session import asyncio
from pydantic import BaseModel

from psychopy_session_webserver.options import parse_options
from psychopy_session_webserver.server import BackgroundServer
from psychopy_session_webserver.session import Session
from psychopy_session_webserver.types import Catalog, Parameter

app = FastAPI()

session: Session = None


@app.get("/experiments")
async def get_experiments() -> Catalog:
    return session.experiments


async def send_server_side_event(agen):
    while True:
        event = await anext(agen)
        if isinstance(event.data, BaseModel):
            dump = event.data.model_dump_json()
        else:
            dump = event.__pydantic_serializer__.to_json(event.data, warnings=False)
        yield f"event:{event.type}\ndata:{str(dump)}\n\n"


@app.get("/events")
async def get_events():
    return StreamingResponse(
        send_server_side_event(session.updates()), media_type="text/event-stream"
    )


@app.put("/window")
async def open_window():
    try:
        session.openWindow()
    except Exception as e:
        raise HTTPException(status_code=409, detail=str(e))


@app.delete("/window")
async def close_window():
    try:
        session.closeWindow()
    except Exception as e:
        raise HTTPException(status_code=409, detail=str(e))


class RunExperimentRequest(BaseModel):
    key: str
    parameter: Parameter


@app.post("/experiment/")
async def run_experiment(request: RunExperimentRequest):
    try:
        session.runExperiment(request.key, **request.parameter)
    except Exception as e:
        raise HTTPException(status_code=400, detail=str(e))


@app.delete("/experiment")
async def stop_experiment():
    try:
        session.stopExperiment()
    except Exception as e:
        raise HTTPException(status_code=409, detail=str(e))


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

    server.join()
