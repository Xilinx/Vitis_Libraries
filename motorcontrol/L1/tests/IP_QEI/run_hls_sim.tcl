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

set PROJ "hls_qei.prj"
set SOLN "sol1"

if {![info exists CLKP]} {
  set CLKP 10
}

open_project -reset $PROJ

add_files "${CUR_DIR}/src/ip_qei.cpp" -cflags "-DSIM_FINITE -I${XF_PROJ_ROOT}/L1/include/hw -I${CUR_DIR}/src"
add_files -tb "${CUR_DIR}/src/test_ip_qei.cpp" -cflags "-I${XF_PROJ_ROOT}/L1/include/hw -I${CUR_DIR}/src"
set_top hls_qei

open_solution -reset $SOLN



set_part $XPART
create_clock -period $CLKP
set_clock_uncertainty 1.25

#config_interface -s_axilite_auto_restart_counter 1
#config_interface -s_axilite_sw_reset

if {$CSIM == 1} {
  csim_design
}

if {$CSYNTH == 1} {
  csynth_design
}

if {$COSIM == 1} {
  cosim_design
}

if {$VIVADO_SYN == 1} {
  config_export -ipname hls-qei-axi
  export_design -flow syn -rtl verilog
}

if {$VIVADO_IMPL == 1} {
  config_export -ipname hls-qei-axi
  export_design -flow impl -rtl verilog
}

exit
