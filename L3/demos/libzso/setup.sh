#!/bin/bash
source /opt/xilinx/xrt/setup.sh
source /opt/xilinx/xrm/setup.sh
export XILINX_LIBZ_XCLBIN=$1
export XRT_INI_PATH=$PWD/xrt.ini
export LD_LIBRARY_PATH=${PWD}:$LD_LIBRARY_PATH
systemctl status xrmd
echo "Run ./xrmxclbin.sh"
