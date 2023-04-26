#!/bin/bash

mkdir -p logs

source /proj/xbuilds/2023.1_daily_latest/installs/lin64/Vitis/2023.1/settings64.sh #|| { exitcode=$?; echo $exitcode > $rstlog; exit $exitcode; }

source /proj/xbuilds/2023.1_daily_latest/xbb/xrt/packages/setenv.sh #|| { exitcode=$?; echo $exitcode > $rstlog; exit $exitcode; }

ulimit -S -s 16384 || true 


ulimit -a || true 
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$OPENCV_LIB 
export PATH=$PATH:/usr/sbin 

export SYSROOT=/proj/picasso/2023.1_stable_latest/internal_platforms/sw/versal/xilinx-versal/sysroots/aarch64-xilinx-linux
export XILINX_VITIS_AIETOOLS=$XILINX_VITIS/aietools
export XILINXD_LICENSE_FILE=2100@aiengine-eng
export LM_LICENSE_FILE=1757@xsjlicsrvip

export JENKINS_INTERNAL_BUILD=1

#make run -f Makefile.old
make run -f Makefile PLATFORM_REPO_PATHS=/proj/picasso/2023.1_stable_latest/installs/lin64/Vitis/2023.1/base_platforms PLATFORM=/proj/picasso/2023.1_stable_latest/installs/lin64/Vitis/2023.1/base_platforms/xilinx_vck190_base_202310_1/xilinx_vck190_base_202310_1.xpfm TARGET=hw_emu
