#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SCRIPT_PATH="$SCRIPT_DIR/3rdparty/Pangolin"

echo 'export PATH='$SCRIPT_PATH'/bin:$PATH' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH='$SCRIPT_PATH'/lib:$LD_LIBRARY_PATH' >> ~/.bashrc
echo 'export CPP_INCLUDE_PATH='$SCRIPT_PATH'/include:$CPP_INCLUDE_PATH' >> ~/.bashrc