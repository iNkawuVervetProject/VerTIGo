import ctypes
from enum import IntEnum

import hid


class Error:
    NO_ERROR = 0
    IR_SENSOR_READOUT_ERROR = 1
    WHEEL_CONTROLLER_MOTOR_FAULT = 2
    WHEEL_CONTROLLER_SENSOR_ISSUE = 3
    WHEEL_CONTROLLER_SENSOR_IRRESPONSIVE = 4
    PELLET_COUNTER_SENSOR_ISSUE = 5
    DISPENSER_EMPTY = 6
    DISPENSER_JAMMED = 7
    DISPENSER_QUEUE_FULL = 8
    DISPENSER_SELF_CHECK_FAIL = 9
    DISPENSER_CALIBRATION_ERROR = 10
    DISPENSER_OPERATION_WATCHDOG = 11

    description = {
        NO_ERROR: "No error",
        IR_SENSOR_READOUT_ERROR: "IRSensor: readout error",
        WHEEL_CONTROLLER_MOTOR_FAULT: "WheelController: motor is stalled",
        WHEEL_CONTROLLER_SENSOR_ISSUE: "WheelController: sensor issue",
        WHEEL_CONTROLLER_SENSOR_IRRESPONSIVE: "WheelController: sensor seems irresponsive",
        PELLET_COUNTER_SENSOR_ISSUE: "PelletCounter: sensor issue",
        DISPENSER_EMPTY: "Dispenser: empty",
        DISPENSER_JAMMED: "Dispenser: jammed",
        DISPENSER_QUEUE_FULL: "Dispenser: too many commands",
        DISPENSER_SELF_CHECK_FAIL: "Dispenser: self-test failed",
        DISPENSER_CALIBRATION_ERROR: "Dispenser: calibration error",
        DISPENSER_OPERATION_WATCHDOG: "Dispenser: operation watchdog reached",
    }


class LoggedError(ctypes.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [("time", ctypes.c_longlong), ("error", ctypes.c_byte)]

    @staticmethod
    def from_bytes(bytes):
        if len(bytes) != 9:
            raise RuntimeError(f"Invalid buffer length {len(bytes)}")
        return ctypes.cast(bytes, ctypes.POINTER(LoggedError)).contents

    def __str__(self):
        return f"{self.time/1_000_000:011.6f}s: {Error.description.get(self.error,'Unknown error')}"


class ErrorReport(ctypes.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [("count", ctypes.c_byte), ("error", LoggedError)]

    @property
    def error_description(self):
        return Error.description.get(self.error, "Unknown Error")


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

    @property
    def has_error(self):
        return self.error != Error.NO_ERROR

    @property
    def error_description(self):
        return Error.description.get(self.error, "Unknown Error")


class ReportType(IntEnum):
    ERROR_LOG = 0x61
    TIME = 0x62


class CommandCode(IntEnum):
    DISPENSE = 0x71
    CALIBRATE = 0x72
    BOOTSEL = 0x7F


class DispenserError(RuntimeError):
    def __init__(self, description, dispensed):
        super(DispenserError, self).__init__(description)
        self.dispensed = dispensed


class Device:
    def __init__(self, device=None):
        self._dev = device or hid.Device(0xCAFE, 0x4004)
        self._errors = []

    def error_log(self):
        while True:
            bytes = self._dev.get_feature_report(ReportType.ERROR_LOG, 11)
            try:
                report = ErrorReport.from_buffer_copy(bytes[1:])
            except ValueError:
                break
            self._errors.append(report.error)
        return self._errors

    def current_time(self):
        report = TimeReport.from_buffer_copy(
            self._dev.get_feature_report(ReportType.TIME, 9)[1:]
        )

        return report.time

    def _execute_cmd(self, cmd):
        self._dev.write(bytes(bytearray([0]) + bytearray(cmd)))
        report = CommandReport.from_buffer_copy(self._dev.read(4))
        if report.code != cmd.code:
            raise RuntimeError(
                f"unexpected command report code 0x{report.code:x} (expected:0x{cmd.code:x})"
            )
        return report

    def dispense(self, count):
        cmd = Command(code=CommandCode.DISPENSE, parameter=count)
        report = self._execute_cmd(cmd)
        if report.error != 0:
            raise DispenserError(
                f"could not dispense {count}: {report.error_description}, dispensed:{report.value}",
                report.value,
            )
        return report.value

    def calibrate(self, speed):
        cmd = Command(code=CommandCode.CALIBRATE, parameter=speed)
        report = self._execute_cmd(cmd)
        if report.error != 0:
            raise DispenserError(
                f"could not calibrate: got error: {report.error_description}", 0
            )

    def reboot_to_bootsel(self):
        cmd = Command(code=CommandCode.BOOTSEL, parameter=0)
        report = self._execute_cmd(cmd)
        if report.error != 0:
            raise DispenserError(
                f"could not reboot to bootsel: {report.error_description}", 0
            )
        del self._dev
