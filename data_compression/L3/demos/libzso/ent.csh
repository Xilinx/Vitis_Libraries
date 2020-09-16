#!/bin/csh
source /opt/xilinx/xrt/setup.csh
source /opt/xilinx/xrm/setup.csh
setenv XILINX_LIBZ_XCLBIN $1
setenv XRT_INI_PATH $PWD/xrt.ini
setenv LD_LIBRARY_PATH ${PWD}:$LD_LIBRARY_PATH
