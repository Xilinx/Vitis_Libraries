#
# Copyright 2019-2021 Xilinx, Inc.
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

set CSIM 1
set CSYNTH 1
set COSIM 1
set VIVADO_SYN 1
set VIVADO_IMPL 1
set CUR_DIR [pwd]
set XF_PROJ_ROOT $CUR_DIR/../../..
set XPART 

set PROJ "compaction_test.prj"
set SOLN "sol1"

if {![info exists CLKP]} {
  set CLKP 3.33333333333333
}

open_project -reset $PROJ

add_files "${XF_PROJ_ROOT}/L2/src/hw/compaction_core.cc" -cflags "-I ${XF_PROJ_ROOT}/L2/include/hw"
add_files -tb "host/main.cc" -cflags "-I ${XF_PROJ_ROOT}/L2/include/hw"
set_top CUCoreLoopTop

open_solution -reset $SOLN




set_part $XPART
create_clock -period $CLKP

if {$CSIM == 1} {
  csim_design -argv "${CUR_DIR}"
}

if {$CSYNTH == 1} {
  csynth_design
}

if {$COSIM == 1} {
  cosim_design -argv "${CUR_DIR}"
}

if {$VIVADO_SYN == 1} {
  export_design -flow syn -rtl verilog
}

if {$VIVADO_IMPL == 1} {
  export_design -flow impl -rtl verilog
}

exit