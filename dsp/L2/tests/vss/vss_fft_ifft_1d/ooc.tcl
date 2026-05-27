#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
set build_dir [lindex $::argv 0]
set constr    [lindex $::argv 1]

open_project ${build_dir}/_x/link/vivado/vpl/prj/prj.xpr
open_bd_design ${build_dir}/_x/link/vivado/vpl/prj/prj.srcs/sources_1/bd/vss_fft_ifft_1d/vss_fft_ifft_1d.bd
upgrade_ip [get_ips]
make_wrapper -files [get_files ${build_dir}/_x/link/vivado/vpl/prj/prj.srcs/sources_1/bd/vss_fft_ifft_1d/vss_fft_ifft_1d.bd] -top
add_files -norecurse ${build_dir}/_x/link/vivado/vpl/prj/prj.gen/sources_1/bd/vss_fft_ifft_1d/hdl/vss_fft_ifft_1d_wrapper.v
add_files -fileset constrs_1 -norecurse ${constr}
update_compile_order -fileset sources_1
set_property top vss_fft_ifft_1d_wrapper [current_fileset]
#synth_design -mode out_of_context
set_property -name {STEPS.SYNTH_DESIGN.ARGS.MORE OPTIONS} -value {-mode out_of_context} -objects [get_runs synth_1]
reset_run synth_1
launch_runs synth_1 -jobs 4
wait_on_runs synth_1
launch_runs impl_1 -jobs 4
wait_on_runs impl_1
open_run impl_1
report_utilization -name utilization_1
close_project