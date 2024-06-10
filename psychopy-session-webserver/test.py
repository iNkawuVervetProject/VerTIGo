import contextlib
import logging
from fastapi import FastAPI
import threading
import uvicorn
import time
import sys


class Server(uvicorn.Server):
    def install_signal_handlers(self):
        pass

    @contextlib.contextmanager
    def run_in_thread(self):
        thread = threading.Thread(target=self.run)
        thread.start()
        try:
            while not self.started:
                time.sleep(1e-3)
            yield
        finally:
            self.should_exit = True
            thread.join()


if __name__ == "__main__":
    config = uvicorn.Config("test:app", host="127.0.0.1", port=5000, log_level="info")
    server = Server(config=config)

    with server.run_in_thread():
        time.sleep(60)


app = FastAPI()

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
stream_handler = logging.StreamHandler(sys.stderr)
log_formatter = logging.Formatter(
    "%(asctime)s [%(processName)s: %(process)d] [%(threadName)s: %(thread)d] [%(levelname)s] %(name)s: %(message)s"
)
stream_handler.setFormatter(log_formatter)
logger.addHandler(stream_handler)

logger.info("API is starting up")


@app.get("/")
async def home():
    logger.info(
        f"Current: {threading.current_thread()} Main: {threading.main_thread()}"
    )
    return "Hello World"
