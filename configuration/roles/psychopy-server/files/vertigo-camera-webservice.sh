#!/bin/bash

set -e

PSYCHOPY_ROOT=$HOME/psychopy
LIBCAMERA_ROOT=/usr/local/src/libcamera

source $LIBCAMERA_ROOT/venv/bin/activate

mkdir -p $PSYCHOPY_ROOT/movies

cd $PSYCHOPY_ROOT/movies

GST_PLUGIN_PATH=$LIBCAMERA_ROOT/build/src/gstreamer python -m inkawuvp_vertigo_camera serve --host 127.0.0.1 --port 5042
