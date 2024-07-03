#!/bin/bash

set -x

source $HOME/psychopy/venv/bin/activate

mkdir -p $HOME/psychopy/data $HOME/psychopy/experiments

python -m psychopy_session_webserver -L 127.0.0.1 --data-dir $HOME/psychopy/data $HOME/psychopy/experiments
