# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# vitis hls makefile-generator v2.0.0

set CSIM 0
set CSYNTH 0
set COSIM 0
set VIVADO_SYN 0
set VIVADO_IMPL 0
set CUR_DIR [pwd]
set OPENCV_INCLUDE $::env(OPENCV_INCLUDE)
set OPENCV_LIB $::env(OPENCV_LIB)
set XF_PROJ_ROOT $CUR_DIR/../../../..
set XPART xcu200-fsgd2104-2-e

set PROJ "remap.prj"
set SOLN "sol1"

if {![info exists CLKP]} {
  set CLKP 3.3
}

open_project -reset $PROJ

add_files "${XF_PROJ_ROOT}/L1/examples/remap/xf_remap_accel.cpp" -cflags " -I ${XF_PROJ_ROOT}/L1/tests/remap/remap_NPPC2_8UC1_8UC1_URAM_INTERPOLATION_NN -I${XF_PROJ_ROOT}/L1/include -I ./ -D__SDSVHLS__ -std=c++0x" -csimflags " -I ${XF_PROJ_ROOT}/L1/tests/remap/remap_NPPC2_8UC1_8UC1_URAM_INTERPOLATION_NN -I${XF_PROJ_ROOT}/L1/include -I ./ -D__SDSVHLS__ -std=c++0x"
add_files -tb "${XF_PROJ_ROOT}/L1/examples/remap/xf_remap_tb.cpp" -cflags " -I ${XF_PROJ_ROOT}/L1/tests/remap/remap_NPPC2_8UC1_8UC1_URAM_INTERPOLATION_NN -I${OPENCV_INCLUDE} -I${XF_PROJ_ROOT}/L1/include -I ./ -D__SDSVHLS__ -std=c++0x" -csimflags " -I ${XF_PROJ_ROOT}/L1/tests/remap/remap_NPPC2_8UC1_8UC1_URAM_INTERPOLATION_NN -I${XF_PROJ_ROOT}/L1/include -I ./ -D__SDSVHLS__ -std=c++0x"
set_top remap_accel

open_solution -reset $SOLN



set_part $XPART
create_clock -period $CLKP

if {$CSIM == 1} {
  csim_design -ldflags "-L ${OPENCV_LIB} -lopencv_imgcodecs -lopencv_imgproc -lopencv_core -lopencv_highgui -lopencv_flann -lopencv_features2d" -argv "  ${XF_PROJ_ROOT}/data/128x128.png"
}

if {$CSYNTH == 1} {
  csynth_design
}

if {$COSIM == 1} {
  cosim_design -ldflags "-L ${OPENCV_LIB} -lopencv_imgcodecs -lopencv_imgproc -lopencv_core -lopencv_highgui -lopencv_flann -lopencv_features2d" -argv "  ${XF_PROJ_ROOT}/data/128x128.png"
}

if {$VIVADO_SYN == 1} {
  export_design -flow syn -rtl verilog
}

if {$VIVADO_IMPL == 1} {
  export_design -flow impl -rtl verilog
}

exit