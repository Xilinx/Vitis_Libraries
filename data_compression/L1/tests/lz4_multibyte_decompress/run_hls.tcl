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
set PROJ "lz4_decompress_test.prj"
set SOLN "sol1"
set CLKP 3.3
set DIR_NAME "lz4_multibyte_decompress"
set DESIGN_PATH "${XF_PROJ_ROOT}/L1/tests/${DIR_NAME}"

# Create a project
open_project -reset $PROJ

# Add design and testbench files
add_files lz4_decompress_test.cpp -cflags "-I${XF_PROJ_ROOT}/L1/include/hw"
add_files -tb lz4_decompress_test.cpp -cflags "-I${XF_PROJ_ROOT}/L1/include/hw"

# Set the top-level function
set_top lz4DecompressEngineRun

# Create a solution
open_solution -reset $SOLN

# Define technology and clock rate
set_part {xcu200}
create_clock -period $CLKP

if {$CSIM == 1} {
  csim_design -O -argv "${DESIGN_PATH}/sample.txt.encoded ${DESIGN_PATH}/sample.txt.encoded.out ${DESIGN_PATH}/sample.txt"
}

if {$CSYNTH == 1} {
  csynth_design  
}

if {$COSIM == 1} {
  cosim_design -O -argv "${DESIGN_PATH}/sample.txt.encoded ${DESIGN_PATH}/sample.txt.encoded.out ${DESIGN_PATH}/sample.txt"
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
