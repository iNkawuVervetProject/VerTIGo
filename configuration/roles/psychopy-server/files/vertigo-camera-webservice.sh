#!/bin/bash

set -e

PSYCHOPY_ROOT=$HOME/psychopy
LIBCAMERA_ROOT=/usr/local/src/libcamera
VERTIGO_CAMERA_ROOT=/usr/local/src/inkawuvp-vertigo-camera

source $VERTIGO_CAMERA_ROOT/venv/bin/activate

mkdir -p $PSYCHOPY_ROOT/movies

cd $PSYCHOPY_ROOT/movies

GST_PLUGIN_PATH=$LIBCAMERA_ROOT/build/src/gstreamer python -m inkawuvp_vertigo_camera serve --host 127.0.0.1 --port 5042
