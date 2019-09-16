#!/bin/env bash

if [ "$RDI_REGR_SCRIPTS" != "" ]; then
	WRAPPER=$RDI_ROOT/hierdesign/fisInfra/sprite/bin/rdiWrapper
else
	export RDI_REGR_SCRIPTS=/proj/rdi/env/stable/rdi/util/regression
	WRAPPER=/proj/rdi/env/stable/hierdesign/fisInfra/sprite/bin/rdiWrapper
fi
. $RDI_REGR_SCRIPTS/init.sh
pragma_normal

export XLNX_SRC_PATH=${XLNX_SRC_PATH:=/proj/fisdata/fisusr/no_backup/RDI_fisusr_auviz/xfcv} 
echo "XLNX_SRC_PATH=$XLNX_SRC_PATH"

export OPENCV_INCLUDE=${OPENCV_INCLUDE:=/proj/fisdata/fisusr/no_backup/RDI_fisusr_auviz/lib/zcu102_arm_opencv/include} 
export OPENCV_LIB=${OPENCV_LIB:=/proj/fisdata/fisusr/no_backup/RDI_fisusr_auviz/lib/zcu102_arm_opencv/lib} 
export XILINX_SDX=${XILINX_SDX:=/proj/xbuilds/2017.1_sdx_daily_latest/installs/lin64/SDx/2017.1}

$XILINX_SDX/bin/sdx -version
$XILINX_SDX/bin/xocc -version
. ${XILINX_SDX}/settings64.sh

sds++ -version

make ultraclean

make copy

make | tee make.log

mstatus=${PIPESTATUS[0]}

if [ $mstatus -eq 0 ]; then
	echo "MAKE PASSED. STATUS = $mstatus"

	timing_error=`grep -m1 -P "^CRITICAL WARNING: \[Timing.* The design failed to meet the timing requirements" make.log | sed 's#CRITICAL WARNING: ##' `

	if [ ! -z "$timing_error" ] ; then
	    echo "ERROR: $timing_error"
	    exit 1
	fi

else
	echo "MAKE FAILED. STATUS = $mstatus"

	if [ $mstatus == 2 ]; then
		exit 1
	else
		exit $mstatus
	fi
fi
