#!/usr/bin/env bash

unset XILINX_VIVADO
unset XILINX_VITIS
export PATH=/proj/rdi-xco/staff/lingl/anaconda3/bin:$PATH
. /proj/rdi-xco/staff/lingl/anaconda3/etc/profile.d/conda.sh
conda activate xf_blas_3_6_7

export TA_PATH=/proj/xbuilds/2019.2_daily_latest/installs/lin64
export XILINX_VITIS=${TA_PATH}/Vitis/2019.2
export XILINX_VIVADO=${TA_PATH}/Vivado/2019.2
export LD_LIBRARY_PATH=`$XILINX_VITIS/bin/ldlibpath.sh $XILINX_VITIS/lib/lnx64.o`:${XILINX_VITIS}/lnx64/tools/opencv
source ${XILINX_VIVADO}/settings64.sh

