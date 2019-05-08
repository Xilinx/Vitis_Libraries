#!/usr/bin/env bash

unset SDSOC_SDK
unset SDSOC_VIVADO
unset SDSOC_VIVADO_HLS
unset SDX_VIVADO
unset SDX_VIVADO_HLS
unset XILINX_VIVADO_HLS
unset XILINX_SDK

export TA_PATH=/proj/xbuilds/2019.1_daily_latest/installs/lin64
export XILINX_SDX=${TA_PATH}/SDx/2019.1
export XILINX_VIVADO=${TA_PATH}/Vivado/2019.1
export LD_LIBRARY_PATH=`$XILINX_SDX/bin/ldlibpath.sh $XILINX_SDX/lib/lnx64.o`:${XILINX_SDX}/lnx64/tools/opencv
source ${TA_PATH}/SDx/2019.1/settings64.sh
