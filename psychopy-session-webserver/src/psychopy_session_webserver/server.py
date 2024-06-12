from contextlib import contextmanager
import threading
import uvicorn


class Server(uvicorn.Server):
    def install_signal_handlers(self):
        pass

    @contextmanager
    def run_in_thread(self):
        thread = threading.Thread(target=self.run)
        thread.start()
        try:
            yield
        finally:
            self.should_exit = True
            thread.join()
