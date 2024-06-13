from functools import partial
from queue import Empty, Queue

from psychopy import core
from psychopy.core import threading
from psychopy.visual import Window
from unil_pellet_dispenser import Device
from unil_pellet_dispenser import DispenserError as PelletDispenserError


class _NoDevice:

    _Delay = 1.0

    def __init__(self, win: Window):
        self.win = win
        self.clock = core.Clock()
        self.start = self.clock.getTime() - _NoDevice._Delay
        self.count = None

    def close(self):
        pass

    def dispense(self, count: int):
        self.win.showMessage("NOT dispensing %d pellet(s)" % (count,))
        self.start = self.clock.getTime()
        self.count = 0

    def dispensed(self) -> None | int | PelletDispenserError:
        t = self.clock.getTime()
        if (t - self.start) < _NoDevice._Delay:
            return None
        self.win.showMessage(None)
        count = self.count
        self.count = None
        return PelletDispenserError(
            f"could not dispense {count} pellet(s): no dispenser connected", 0
        )


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
        if self.outqueue.empty() is False:
            raise RuntimeError("cannot dispense twice, query result first")
        self._dispensed = None
        self.inqueue.put(count)

    def dispensed(self) -> None | int | PelletDispenserError:
        # return last result if any
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
    def __init__(self, win: Window, logger):
        try:
            self.device = _AsyncDevice()
        except Exception as e:
            logger.error(f"could not find a pellet dispenser device: {e}")
            logger.warn("no pellet will be dispensed in this experiment")
            self.device = _NoDevice(win)

        # The Builder BaseComponent needs such a field for start/stop
        # conditions
        self.status = None

    def close(self):
        self.device.close()

    def dispense(self, count: int):
        self.device.dispense(count)

    @property
    def dispensed(self) -> None | int | PelletDispenserError:
        return self.device.dispensed()
