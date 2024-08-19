# iNkawu-vertigo-camera

This package holds utilities to manipulate the camera on VerTIGo. It provides:

* a package to record live stream of camera via a simple webapi. It is based on
  a modified libcamera gstreamer plugin that allows to record a UNIX timestamp
  for each frame.
* Utilities to access each frame according to its timestamp (i.e. for matching
  with a psychopy session).
