from datetime import datetime, timezone
from enum import Enum
import time
from typing import Optional
import pickle

import gi

gi.require_version("Gst", "1.0")

from gi.repository import GLib, Gst

from pydantic import BaseModel

Gst.init(None)


def _print_message(msg: Gst.Message):
    if msg.type == Gst.MessageType.ERROR:
        print(msg.src.name, msg.parse_error())
    elif msg.type == Gst.MessageType.EOS:
        print(msg.src.name, "EOS")
    elif msg.type == Gst.MessageType.STATE_CHANGED:
        print(msg.src.name, msg.parse_state_changed())
    elif msg.type == Gst.MessageType.STREAM_STATUS:
        print(msg.src.name, msg.parse_stream_status())
    elif msg.type == Gst.MessageType.PROGRESS:
        print(msg.src.name, msg.parse_progress())
    elif msg.type == Gst.MessageType.LATENCY:
        print(msg.src.name, msg.new_latency)
    else:
        print(msg.src.name, msg.type)
    print("")


class Resolution(BaseModel):
    Width: int
    Height: int


class AwbModeEnum(str, Enum):
    Auto = "awb-auto"
    Incandescent = "awb-incandescent"
    Tungsten = "awb-tungsten"
    Fluorescent = "awb-fluorescent"
    Indoor = "awb-indoor"
    Daylight = "awb-daylight"
    Cloudy = "awb-cloudy"
    Custom = "awb-custom"


class AutoFocusModeEnum(str, Enum):
    Manual = "manual-focus"
    Auto = "automatic-auto-focus"
    Continuous = "continuous-auto-focus"


class AfRangeEnum(str, Enum):
    Normal = "af-range-normal"
    Macro = "af-range-macro"
    Full = "af-range-full"


class CameraParameter(BaseModel):

    Framerate: int = 30

    FileResolution: Resolution = Resolution(Width=1920, Height=1080)
    FileBitrate: int = 1500
    FileSpeedPreset: str = "fast"

    StreamResolution: Resolution = Resolution(Width=854, Height=480)
    StreamBitrate: int = 400

    RtspServerPath: str = "rtsp://localhost:8554/camera-live"

    AwbMode: AwbModeEnum = AwbModeEnum.Auto
    AutoFocusMode: AutoFocusModeEnum = AutoFocusModeEnum.Auto
    AfRange: AfRangeEnum = AfRangeEnum.Normal

    LensPosition: float = 0.0


class CameraStream:
    _timestamp_unix = Gst.Caps.from_string("timestamp/x-unix")

    def __init__(self, params: CameraParameter = CameraParameter(), debug=False):
        self.now = datetime.now(timezone.utc).astimezone()
        self._file = open(self.basename() + ".pickle", "wb")
        self._debug = debug
        self._params = params
        self._create_pipeline()

    def basename(self):
        return self.now.isoformat()

    def _on_sample(self, appsink: Gst.Element, *args, **kwargs) -> Gst.FlowReturn:
        sample = appsink.emit("pull-sample")

        if not sample:
            return Gst.FlowReturn.OK

        buffer: Optional[Gst.Buffer] = sample.get_buffer()
        if not buffer:
            return Gst.FlowReturn.OK

        if self._debug is False:
            ts = buffer.get_reference_timestamp_meta(CameraStream._timestamp_unix)
            if not ts:
                print("no timestamp")
                return Gst.FlowReturn.OK
            ts = ts.timestamp
        else:
            ts = time.time_ns()

        pickle.dump([buffer.offset, buffer.pts, ts], self._file)

        return Gst.FlowReturn.OK

    def _create_pipeline(self):
        if self._debug is False:
            src = (
                "libcamerasrc name=source unix-timestamp=true"
                f" lens-position={self._params.LensPosition:.3f} "
                f"auto-focus-range={self._params.AfRange} "
                f"awb-mode={self._params.AwbMode} "
                f"auto-focus-mode={self._params.AutoFocusMode}"
            )

        else:
            src = "videotestsrc is-live=true name=source"

        self.pipeline = Gst.parse_launch(
            f"{src} !"
            f" video/x-raw,width={self._params.FileResolution.Width},height={self._params.FileResolution.Height},framerate={self._params.Framerate}/1"
            " ! tee name=t ! queue ! x264enc"
            f" bitrate={self._params.FileBitrate} speed-preset={self._params.FileSpeedPreset} tune=zerolatency"
            f" ! mp4mux ! filesink name=mp4sink location={self.basename()}.mp4 t. !"
            " queue leaky=downstream flush-on-eos=true ! videoscale !"
            f" video/x-raw,width={self._params.StreamResolution.Width},height={self._params.StreamResolution.Height} !"
            f" openh264enc bitrate={self._params.StreamBitrate*1000} ! video/x-h264 !"
            " rtspclientsink"
            f" location={self._params.RtspServerPath} async-handling=true"
            " timeout=200000 tcp-timeout=200000 debug=true t. ! queue ! appsink"
            " name=ts_sink"
        )

        self.pipeline.set_name("pipeline_" + self.now.isoformat())

        self.ts_sink: Gst.Element = self.pipeline.get_by_name("ts_sink")

        self.ts_sink.set_property("emit-signals", True)
        self.ts_sink.connect("new-sample", self._on_sample)

    def run(self):
        if self.pipeline is None:
            raise RuntimeError("pipeline is already done")

        bus = self.pipeline.get_bus()
        if bus is None:
            raise RuntimeError("no bus available? this is annoying")

        loop = GLib.MainLoop()

        def on_message(_, msg: Gst.Message):
            if self._debug:
                print(msg)
            if msg.src.name != self.pipeline.name:
                return
            if msg.type == Gst.MessageType.ERROR:
                err, dbg = msg.parse_error()
                loop.quit()
                raise RuntimeError(f"Gst Error ({msg.src.name}): {err}\n{dbg}")
            if msg.type == Gst.MessageType.EOS:
                loop.quit()

        bus.add_signal_watch()
        bus.connect("message", on_message)

        print(f"starting pipeline {self.pipeline.name}")
        self.pipeline.set_state(Gst.State.PLAYING)

        try:
            loop.run()
        finally:
            self.pipeline.set_state(Gst.State.NULL)
            del self.pipeline
            self._file.close()
            del self._file

    def stop(self):

        if self.pipeline is None:
            raise RuntimeError("pipeline is not set")
        print(f"stopping pipeline {self.pipeline.name}")
        self.pipeline.send_event(Gst.Event.new_eos())
