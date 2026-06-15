# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2023, Advanced Micro Devices, Inc.

set CSIM 0
set CSYNTH 0
set COSIM 0
set VIVADO_SYN 0
set VIVADO_IMPL 0
set CUR_DIR [pwd]
set OPENCV_INCLUDE $::env(OPENCV_INCLUDE)
set OPENCV_LIB $::env(OPENCV_LIB)
set XF_PROJ_ROOT $CUR_DIR/../../../..
set XPART xcve2802-vsvh1760-2MP-e-S
set TEST_DIR $CUR_DIR
set CONFIG_DIR "${XF_PROJ_ROOT}/L1/examples/layout_formatter/config"
set EXAMPLE_DIR "${XF_PROJ_ROOT}/L1/examples/layout_formatter"

set PROJ "layout_formatter.prj"
set SOLN "sol1"

if {![info exists CLKP]} {
  set CLKP 7.5
}

open_project -reset $PROJ

add_files "${XF_PROJ_ROOT}/L1/examples/layout_formatter/xf_layout_formatter_accel.cpp" -cflags " -I $TEST_DIR -I $CONFIG_DIR -I${XF_PROJ_ROOT}/L1/include -I ./ -D__SDSVHLS__ -std=c++0x" -csimflags " -I $TEST_DIR -I $CONFIG_DIR -I${XF_PROJ_ROOT}/L1/include -I ./ -D__SDSVHLS__ -std=c++0x"
add_files -tb "${XF_PROJ_ROOT}/L1/examples/layout_formatter/xf_layout_formatter_tb.cpp" -cflags " -I $TEST_DIR -I $CONFIG_DIR -I $EXAMPLE_DIR -I${OPENCV_INCLUDE} -I${XF_PROJ_ROOT}/L1/include -I ./ -D__SDSVHLS__ -std=c++0x"
set_top layout_formatter_accel

open_solution -reset $SOLN
set_part $XPART
create_clock -period $CLKP

if {$CSIM == 1} {
  csim_design -ldflags "-L ${OPENCV_LIB} -lopencv_imgcodecs -lopencv_imgproc -lopencv_core -lopencv_highgui -lopencv_flann -lopencv_features2d" -argv "${XF_PROJ_ROOT}/data/128x128.png"
}

if {$CSYNTH == 1} { csynth_design }
if {$COSIM == 1} {
  cosim_design -ldflags "-L ${OPENCV_LIB} -lopencv_imgcodecs -lopencv_imgproc -lopencv_core -lopencv_highgui -lopencv_flann -lopencv_features2d" -argv "${XF_PROJ_ROOT}/data/128x128.png"
}
if {$VIVADO_SYN == 1} { export_design -flow syn -rtl verilog }
if {$VIVADO_IMPL == 1} { export_design -flow impl -rtl verilog }

exit
