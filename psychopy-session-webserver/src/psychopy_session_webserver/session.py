from functools import partial
import queue
from os import path
from psychopy import session
from multiprocessing import Process


class Session:
    def __init__(self, root):
        pass

    def start():
        pass

    def stop():
        pass

    def addExperiment(file, key=None):
        pass

    def removeExperiment(key):
        pass

    def runExperiment(key):
        pass

    def stopExperiment():
        pass

    def close():
        pass


class ThreadedSession:
    @staticmethod
    def Loop(q: queue.Queue):
        while True:
            try:
                it = q.get(timeout=2)
                it.method(**it.params)
                q.task_done()
            except queue.Empty:
                pass

    def __init__(self, *args, **kwargs):
        self._session = Session(*args, **kwargs)
        self._queue = queue.Queue()
        self._process = Process(target=ThreadedSession.Loop, args=(self._queue,))
        self._process.start()

    def close(self):
        self._queue.put(partial(self._session.close))
        self._queue.join()
        self._process.close()
