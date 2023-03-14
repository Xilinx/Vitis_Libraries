#
# Copyright 2022 Xilinx, Inc.
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
#

source settings.tcl


set PROJ "ip_foc_periodic_sensorless_ap_fixed.prj"
set SOLN "sol1"

if {![info exists CLKP]} {
  set CLKP 10
}

open_project -reset $PROJ

add_files "${CUR_DIR}/src/ip_foc.cpp" -cflags "-DSIM_FINITE -O3 -I${XF_PROJ_ROOT}/L1/include/hw -I${CUR_DIR}/src"
add_files -tb "${CUR_DIR}/src/test_ip_foc.cpp" -cflags "-O3 -I${XF_PROJ_ROOT}/L1/include/hw -I${CUR_DIR}/src -I${XF_PROJ_ROOT}/L1/tests/Model -Wno-write-strings"
set_top hls_foc_periodic_ap_fixed

# add_files "${XF_PROJ_ROOT}/L1/tests/IP_FOC_sensorless/src/foc_demo.cpp" -cflags "-I${XF_PROJ_ROOT}/L1/include/hw -I${XF_PROJ_ROOT}/L1/tests/IP_FOC_sensorless/src"
# add_files -tb "${XF_PROJ_ROOT}/L1/tests/IP_FOC_sensorless/src/test_foc.cpp" -cflags "-I${XF_PROJ_ROOT}/L1/include/hw -I${XF_PROJ_ROOT}/L1/tests/IP_FOC_sensorless/src -I${XF_PROJ_ROOT}/L1/tests/Model -Wno-write-strings"
# set_top foc_demo

open_solution -reset $SOLN

set_part $XPART
create_clock -period $CLKP
set_clock_uncertainty 1.25

if {$CSIM == 1} {
  csim_design -ldflags -lncurses
}

if {$CSYNTH == 1} {
  csynth_design
}

if {$COSIM == 1} {
  cosim_design -ldflags -lncurses
}

if {$VIVADO_SYN == 1} {
  export_design -flow syn -rtl verilog
}

if {$VIVADO_IMPL == 1} {
  export_design -flow impl -rtl verilog
}

exit
