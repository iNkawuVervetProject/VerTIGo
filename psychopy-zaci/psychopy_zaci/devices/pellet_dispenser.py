from functools import partial
from queue import Empty, SimpleQueue

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
    def __init__(self, logger):
        self.logger = logger
        self.device = Device()

        self.inqueue = SimpleQueue()
        self.outqueue = SimpleQueue()

        self.logger.info("[PelletDispenser] starting thread")
        self.thread = threading.Thread(target=partial(self.loop))
        self.thread.start()

    def close(self):
        self.logger.info("[PelletDispenser] closing device")
        self.inqueue.put(None)
        self.logger.info("[PelletDispenser] joining threads")
        self.thread.join()
        del self.inqueue
        del self.outqueue
        del self.thread

    def dispense(self, count: int):
        if self.outqueue.empty() is False:
            raise RuntimeError("cannot dispense twice, query result first")
        self.logger.info(f"[PelletDispenser] dispensing {count}")
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
        self.logger.debug("[PelletDispenser] enter main loop")
        while True:
            try:
                self.logger.debug(
                    "[PelletDispenser] waiting for dispense count"
                )
                count = self.inqueue.get(timeout=1)
            except Empty:
                continue

            if count is None:
                return

            try:
                self.logger.debug("[PelletDispenser] dispensing")
                dispensed = self.device.dispense(count)
                self.logger.debug("[PelletDispenser] reporting")
                self.outqueue.put(dispensed)

            except PelletDispenserError as e:
                self.logger.error(f"[PelletDispenser] error: {e}")
                self.outqueue.put(e)


class PelletDispenserDevice:
    def __init__(self, win: Window, logger):
        try:
            self.device = _AsyncDevice(logger)
        except Exception as e:
            logger.error(
                "[PelletDispenser] could not find a pellet dispenser"
                f" device: {e}"
            )
            logger.warn(
                "[PelletDispenser] no pellet will be dispensed in this"
                " experiment"
            )
            self.device = _NoDevice(win)

        # The Builder BaseComponent needs such a field for start/stop
        # conditions
        self.status = None
        self._count = None

    def close(self):
        self.device.close()

    def dispense(self, count: int | None = None):
        if count is None:
            if self._count is not None:
                self.device.dispense(self._count)
            else:
                raise RuntimeError(
                    "count is not define through parameter or .setCount()"
                )
        else:
            self.device.dispense(count)

    def setCount(self, count: int | None):
        self._count = count

    @property
    def dispensed(self) -> None | int | PelletDispenserError:
        return self.device.dispensed()
