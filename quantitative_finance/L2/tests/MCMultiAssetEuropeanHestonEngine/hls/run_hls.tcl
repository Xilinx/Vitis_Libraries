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
open_project mc_euro_prj -reset
set_top kernel_mc_0
config_debug 

set WORKDIR "$::env(PWD)/.."


add_files "${WORKDIR}/kernel/kernel_mceuropeanengine.cpp" -cflags " -D KN_0  -D DPRAGMA -g -D VIVADO_HLS_SIM -I ${WORKDIR}/kernel -I ${WORKDIR}/src -I ${WORKDIR}/../../include -D HLS_TEST"
add_files -tb "${WORKDIR}/src/test.cpp" -cflags " -g -D VIVADO_HLS_SIM -I ${WORKDIR}/kernel -I ${WORKDIR}/src -I ${WORKDIR}/../../include -D HLS_TEST"


open_solution solution -reset
set_part xcvu9p-fsgd2104-2-i
create_clock -period 300MHz -name default
set_clock_uncertainty 27.000000%
config_rtl -register_reset
config_rtl -stall_sig_gen
config_interface -m_axi_addr64
config_compile -name_max_length 256

set host_argv "-mode fpga"

csim_design -argv "$host_argv" -compiler clang


csynth_design

cosim_design -trace_level all -argv "$host_argv"
#export_design -flow impl -rtl verilog -format ip_catalog
exit
