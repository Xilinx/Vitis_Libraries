#
# Copyright 2019 Xilinx, Inc.
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

set PROJ "decisiontree.prj"
set SOLN "solution_decisiontree_predict"
set CLKP 300MHz

open_project -reset $PROJ
config_debug

add_files test.cpp -cflags "-I${XF_PROJ_ROOT}/L1/include -I./"
add_files -tb test.cpp -cflags "-I${XF_PROJ_ROOT}/L1/include"
add_files -tb tree.data
add_files -tb test_ss.csv

set_top dut

open_solution -reset $SOLN

set_part $XPART
create_clock -period $CLKP -name default

#config_rtl -register_reset
#config_rtl -stall_sig_gen
#config_interface -m_axi_addr64
#config_compile -name_max_length 256

if {$CSIM == 1} {
  csim_design
}

if {$CSYNTH == 1} {
  csynth_design
}

if {$COSIM == 1} {
#  cosim_design -wave_debug -trace_level all
  cosim_design -trace_level all
}

if {$VIVADO_SYN == 1} {
  export_design -flow syn -rtl verilog
}

if {$VIVADO_IMPL == 1} {
  export_design -flow impl -rtl verilog
}

if {$QOR_CHECK == 1} {
  puts "QoR check not implemented yet"
}

exit
