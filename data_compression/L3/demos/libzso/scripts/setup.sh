#!/bin/bash

if [ -z $1 ]; then
    echo "Input Argument Missing"
    echo "Usage: source setup.sh <absolute path to xclbin>"
    exit 1
fi

source /opt/xilinx/xrt/setup.sh
export XILINX_LIBZ_XCLBIN=$1
export XRT_INI_PATH=$PWD/xrt.ini
export LD_LIBRARY_PATH=${PWD}:$LD_LIBRARY_PATH
