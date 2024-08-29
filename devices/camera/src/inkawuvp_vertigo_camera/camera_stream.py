from datetime import datetime, timezone
from enum import StrEnum
import time
from typing import List, Optional
import pickle
import random

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


class AwbModeEnum(StrEnum):
    Auto = "awb-auto"
    Incandescent = "awb-incandescent"
    Tungsten = "awb-tungsten"
    Fluorescent = "awb-fluorescent"
    Indoor = "awb-indoor"
    Daylight = "awb-daylight"
    Cloudy = "awb-cloudy"
    Custom = "awb-custom"


class AutoFocusModeEnum(StrEnum):
    Manual = "manual-focus"
    Auto = "automatic-auto-focus"
    Continuous = "continuous-auto-focus"


class AfRangeEnum(StrEnum):
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


def format_gstreamer_libcamerasrc(params: CameraParameter) -> str:
    return (
        "libcamerasrc name=source unix-timestamp=true"
        f" lens-position={params.LensPosition:.3f} "
        f"auto-focus-range={params.AfRange} "
        f"awb-mode={params.AwbMode} "
        f"auto-focus-mode={params.AutoFocusMode}"
    )


def format_gstreamer_pipeline(src: str, params: CameraParameter, *, basename) -> str:
    return (
        f"{src} !"
        f" video/x-raw,width={params.FileResolution.Width},height={params.FileResolution.Height},framerate={params.Framerate}/1"
        " ! tee name=t t.src_0 ! queue ! x264enc"
        f" bitrate={params.FileBitrate} speed-preset={params.FileSpeedPreset} tune=zerolatency"
        f" ! mp4mux ! filesink name=mp4sink location={basename}.mp4 t.src_1 ! queue !"
        " appsink name=ts_sink t.src_2 ! queue name=streamqueue ! videoscale"
        " name=streamscale !"
        f" video/x-raw,width={params.StreamResolution.Width},height={params.StreamResolution.Height} !"
        f" openh264enc bitrate={params.StreamBitrate*1000} name=streamenc ! capsfilter"
        " caps=video/x-h264 name=streamcaps ! rtspclientsink name=streamsink"
        f" location={params.RtspServerPath}"
    )


class CameraStream:
    _timestamp_unix = Gst.Caps.from_string("timestamp/x-unix")

    def __init__(self, params: CameraParameter = CameraParameter(), debug=False):
        self.now = datetime.now(timezone.utc).astimezone()
        self._file = open(self.basename() + ".pickle", "wb")
        self._debug = debug
        self._params = params
        self._create_pipeline()
        self._reconnection_attempt = 0

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

    def _reconnection_timeout(self):
        """Return an exponential timeout starting at 2s. also adds a random +/- 1s
        jitter to avoid reconnection storms."""
        if self._debug:
            return 2000
        return (
            2 ** min(6, max(1, self._reconnection_attempt))
        ) * 1000 + random.randrange(-1000, 1000)

    def _disconnect_stream(self):
        def disconnect_probe(pad, info, *args):
            Gst.Pad.remove_probe(pad, info.id)
            self._tee.unlink(self._streamelements[0])
            self._streamelements[-2].unlink(self._streamelements[-1])
            self.pipeline.remove(self._streamelements[-1])

            for e in reversed(self._streamelements):
                e.set_state(Gst.State.NULL)
            self._streamelements = self._streamelements[:-1]

            self.pipeline.set_state(Gst.State.PLAYING)
            self._reconnection_attempt += 1
            GLib.timeout_add(self._reconnection_timeout(), self._reconnect_stream)
            return Gst.PadProbeReturn.OK

        pad = self._tee.get_static_pad("src_2")
        if pad:
            pad.add_probe(
                Gst.PadProbeType.BLOCK_DOWNSTREAM,
                disconnect_probe,
                None,
            )

    def _reconnect_stream(self):
        def connect_probe(pad: Gst.Pad, info, *args):
            Gst.Pad.remove_probe(pad, info.id)
            self._streamelements.append(
                Gst.ElementFactory.make_with_properties(
                    "rtspclientsink",
                    names=["name", "location"],
                    values=["streamsink", self._params.RtspServerPath],
                )
            )

            self.pipeline.add(self._streamelements[-1])
            self._streamelements[-2].link(self._streamelements[-1])
            self._tee.link_pads(srcpadname="src_2", dest=self._streamelements[0])
            for e in self._streamelements:
                e.set_state(Gst.State.PLAYING)

            return Gst.PadProbeReturn.OK

        pad = self._tee.get_static_pad("src_2")
        if pad:
            pad.add_probe(
                Gst.PadProbeType.BLOCK_DOWNSTREAM,
                connect_probe,
                None,
            )
        return False

    def _create_pipeline(self):
        if self._debug is False:
            src = format_gstreamer_libcamerasrc(self._params)
        else:
            src = "videotestsrc is-live=true do-timestamp=1 name=source ! timeoverlay"

        self.pipeline = Gst.parse_launch(
            format_gstreamer_pipeline(src, self._params, basename=self.basename())
        )

        self.pipeline.set_name("pipeline_" + self.now.isoformat())

        self.ts_sink: Gst.Element = self.pipeline.get_by_name("ts_sink")

        self._streamelements: List[Gst.Element] = list([
            self.pipeline.get_by_name(n)
            for n in [
                "streamqueue",
                "streamscale",
                "streamenc",
                "streamcaps",
                "streamsink",
            ]
        ])
        self._streamsink = None

        self._tee: Gst.Element = self.pipeline.get_by_name("t")

        self.ts_sink.set_property("emit-signals", True)
        self.ts_sink.connect("new-sample", self._on_sample)

    def run(self):
        if self.pipeline is None:
            raise RuntimeError("pipeline is already done")

        bus = self.pipeline.get_bus()
        if bus is None:
            raise RuntimeError("no bus available? this is annoying")

        loop = GLib.MainLoop()

        def on_pipeline_message(msg: Gst.Message):
            if msg.type == Gst.MessageType.ERROR:
                err, dbg = msg.parse_error()
                loop.quit()
                raise RuntimeError(f"Gst Error ({msg.src.name}): {err}\n{dbg}")
            if msg.type == Gst.MessageType.EOS:
                loop.quit()

        def on_streamsink_message(msg: Gst.Message):
            if msg.type == Gst.MessageType.ERROR:
                self._disconnect_stream()

        def on_message(_, msg: Gst.Message):
            if msg.src.name == self._streamelements[-1].name:
                on_streamsink_message(msg)
                return
            if msg.src.name == self.pipeline.name:
                on_pipeline_message(msg)
                return

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
