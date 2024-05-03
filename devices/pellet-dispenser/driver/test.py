import hid
import ctypes
from enum import IntEnum


class LoggedError(ctypes.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [("time", ctypes.c_longlong), ("error", ctypes.c_byte)]

    @staticmethod
    def from_bytes(bytes):
        if len(bytes) != 9:
            raise RuntimeError(f"Invalid buffer length {len(bytes)}")
        return ctypes.cast(bytes, ctypes.POINTER(LoggedError)).contents

    def __str__(self):
        return f"{self.time/1_000_000:011.6f}s: error({self.error})"


class ErrorReport(ctypes.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [("count", ctypes.c_byte), ("error", LoggedError)]


class TimeReport(ctypes.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [("time", ctypes.c_longlong)]


class Command(ctypes.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [("code", ctypes.c_byte), ("parameter", ctypes.c_short)]


class CommandReport(ctypes.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("code", ctypes.c_byte),
        ("value", ctypes.c_short),
        ("error", ctypes.c_byte),
    ]


class PelletDispenser:
    class ReportType(IntEnum):
        ERROR_LOG = 0x61
        TIME = 0x62

    class Command(IntEnum):
        DISPENSE = 0x71
        CALIBRATE = 0x72

    def __init__(self, device=None):
        self._dev = device or hid.Device(0xCAFE, 0x4004)
        self._errors = []

    def error_log(self):
        while True:
            bytes = self._dev.get_feature_report(
                PelletDispenser.ReportType.ERROR_LOG, 11
            )
            try:
                report = ErrorReport.from_buffer_copy(bytes[1:])
            except ValueError:
                break
            self._errors.append(report.error)
        return self._errors

    def current_time(self):
        report = TimeReport.from_buffer_copy(
            self._dev.get_feature_report(PelletDispenser.ReportType.TIME, 9)[1:]
        )

        return report.time

    def _execute_cmd(self, cmd):
        self._dev.write(bytes(bytearray([0]) + bytearray(cmd)))
        report = CommandReport.from_buffer_copy(self._dev.read(4))
        if report.code != cmd.code:
            raise RuntimeError(
                f"Unexpected command report code 0x{report.code:x} (expected:0x{cmd.code:x})"
            )
        return report

    def dispense(self, count):
        cmd = Command(code=PelletDispenser.Command.DISPENSE, parameter=count)
        report = self._execute_cmd(cmd)
        if report.error != 0:
            raise RuntimeError(
                f"could not dispense {count}: got error:{report.error}, dispensed:{report.value}"
            )
        return report.value

    def calibrate(self, speed):
        cmd = Command(code=PelletDispenser.Command.CALIBRATE, parameter=speed)
        report = self._execute_cmd(cmd)
        if report.error != 0:
            raise RuntimeError(f"could not calibrate: got error: {report.error}")


dev = PelletDispenser()
print(f"now is {dev.current_time()/1_000_000:011.6f}s")
for e in dev.error_log():
    print(e)
print(dev.dispense(1))
