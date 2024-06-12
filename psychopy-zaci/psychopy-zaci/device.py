import threading
import time
from functools import partial
from queue import Empty, Queue

from psychopy.visual import Window
from unil_pellet_dispenser import Device
from unil_pellet_dispenser import DispenserError as PelletDispenserError


class _NoDevice:
    def __init__(self, win: Window):
        self.win = win

    def close(self):
        pass

    def dispense(self, count: int):
        self.win.showMessage("NOT dispensing %d pellet(s)" % (count,))
        self.start = time.process_time_ns()

    def dispensed(self) -> None | int | PelletDispenserError:
        if time.process_time_ns() - self.start < 1_000_000_000:
            return None
        return PelletDispenserError("No dispenser connected", 0)


class _AsyncDevice:
    def __init__(self):
        self.device = Device()
        self.thread = threading.Thread(target=partial(self.loop))
        self.thread.start()
        self.inqueue = Queue()
        self.outqueue = Queue()

    def close(self):
        self.inqueue.put(None)
        self.thread.join()
        del self.inqueue
        del self.outqueue
        del self.thread

    def dispense(self, count: int):
        if self.outqueue.empty is False:
            raise RuntimeError("cannot dispense twice, query result first")
        self._dispensed = None
        self.inqueue.put(count)

    def dispensed(self) -> None | int | PelletDispenserError:
        if self._dispensed is not None:
            return self._dispensed
        try:
            self._dispensed = self.outqueue.get_nowait()
        except Empty:
            return None

        return self._dispensed

    def loop(self):
        while True:
            try:
                count = self.inqueue.get(timeout=1)
            except Empty:
                continue
            if count is None:
                return

            try:
                dispensed = self.device.dispense(count)
                self.outqueue.put(dispensed)
            except PelletDispenserError as e:
                self.outqueue.put(e)


class PelletDispenserDevice:
    def __init__(self, win: Window):
        try:
            self.device = _AsyncDevice()
        except any:
            self.device = _NoDevice(win)

    def close(self):
        self.device.close()

    def dispense(self, count: int):
        self.device.dispense(count)

    @property
    def dispensed(self) -> None | int | PelletDispenserError:
        return self.device.dispensed()
