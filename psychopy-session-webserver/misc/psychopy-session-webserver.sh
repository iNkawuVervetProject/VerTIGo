#!/bin/bash

set -e

PSYCHOPY_ROOT=$HOME/psychopy

source $PSYCHOPY_ROOT/venv/bin/activate

SITE_PACKAGE=$PSYCHOPY_ROOT/venv/lib/python3.10/site-packages

export LD_LIBRARY_PATH=$SITE_PACKAGE/wx:$SITE_PACKAGE/freetype:$LD_LIBRARY_PATH

export DISPLAY=:0

mkdir -p $PSYCHOPY_ROOT/data
mkdir -p $PSYCHOPY_ROOT/experiments

python -m psychopy_session_webserver -l 127.0.0.1 -l 172.16.0.0/12 -l 100.64.0.0/10 -P 5000 --data-dir $PSYCHOPY_ROOT/data $PSYCHOPY_ROOT/experiments
