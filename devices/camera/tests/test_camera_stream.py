import unittest

from inkawuvp_vertigo_camera.camera_stream import (
    CameraParameter,
    format_gstreamer_libcamerasrc,
)


class TestCameraStream(unittest.TestCase):

    def test_format_gstreamer_libcamerasrc(self):
        self.assertEqual(
            format_gstreamer_libcamerasrc(CameraParameter()),
            "libcamerasrc name=source unix-timestamp=true lens-position=0.000"
            " auto-focus-range=af-range-normal awb-mode=awb-auto"
            " auto-focus-mode=automatic-auto-focus",
        )
