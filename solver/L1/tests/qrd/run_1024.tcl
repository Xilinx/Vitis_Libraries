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

source settings.tcl

set PROJ "qrd_cfloat1024_test.prj"
set SOLN "sol1"
set TESTPATH "qrd"
set QRD_A_ROWS "1024"
set QRD_A_COLS "256"

if {![info exists CLKP]} {
  set CLKP 500MHz
}

open_project -reset $PROJ

add_files "${XF_PROJ_ROOT}/L1/include/hw/qrd.hpp" -cflags "-D_DATA_PATH=${XF_PROJ_ROOT}/L1/tests/${TESTPATH}/data/ -I./ -I${XF_PROJ_ROOT}/L1/tests/ -I${XF_PROJ_ROOT}/L1/include/ -I${XF_PROJ_ROOT}/L1/include/hw -I${XF_PROJ_ROOT}/L2/include -I${XF_PROJ_ROOT}/../utils/L1/include/"
add_files "${XF_PROJ_ROOT}/L1/tests/${TESTPATH}/src/qrd.cpp" -cflags "-DQRF_A_ROWS=${QRD_A_ROWS} -DQRF_A_COLS=${QRD_A_COLS} -D_DATA_PATH=${XF_PROJ_ROOT}/L1/tests/${TESTPATH}/data/ -I./ -I${XF_PROJ_ROOT}/L1/tests/qrf/host/ -I${XF_PROJ_ROOT}/L1/tests/qrf/kernel/ -I${XF_PROJ_ROOT}/L1/tests/qrf/ -I${XF_PROJ_ROOT}/L1/tests/ -I${XF_PROJ_ROOT}/L1/include/ -I${XF_PROJ_ROOT}/L1/include/hw -I${XF_PROJ_ROOT}/../utils/L1/include/"
add_files -tb "${XF_PROJ_ROOT}/L1/tests/${TESTPATH}/test_qrd.cpp" -cflags "-O0 -g -DQRF_A_ROWS=${QRD_A_ROWS} -DQRF_A_COLS=${QRD_A_COLS} -D_DATA_PATH=${XF_PROJ_ROOT}/L1/tests/${TESTPATH}/data/ -I./ -I${XF_PROJ_ROOT}/L1/tests/qrf/host/ -I${XF_PROJ_ROOT}/L1/tests/qrf/kernel/ -I${XF_PROJ_ROOT}/L1/tests/qrf/ -I${XF_PROJ_ROOT}/L1/tests/ -I${XF_PROJ_ROOT}/L1/include/ -I${XF_PROJ_ROOT}/L1/include/hw -I${XF_PROJ_ROOT}/../utils/L1/include/"
#-DDEBUG_QRD

#IP to estimate performance
set_top qrd_cfloat_top

#IP + load buffer + output buffer to estimate resource
#set_top qrd_ip_cfloat_top

open_solution -reset $SOLN

set_part $XPART
create_clock -period $CLKP
set_clock_uncertainty 0.1

if {$CSIM == 1} {
  csim_design -argv "-data ${XF_PROJ_ROOT}/L1/tests/qrd/data"
}

if {$CSYNTH == 1} {
  csynth_design
}

if {$COSIM == 1} {
  cosim_design -argv "-data ${XF_PROJ_ROOT}/L1/tests/qrd/data" 
}

if {$VIVADO_SYN == 1} {
  export_design -flow syn -rtl verilog
}

if {$VIVADO_IMPL == 1} {
  export_design -flow impl -rtl verilog
}

exit
