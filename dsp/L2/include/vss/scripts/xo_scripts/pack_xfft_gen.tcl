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
set part [lindex $argv 0]
set dtype [lindex $argv 1]
set ssr [lindex $argv 2]
set freq [lindex $argv 3]
set point_size [lindex $argv 4]
set data_width [lindex $argv 5]
create_project project_1 project_1 -part ${part}
create_bd_design "xfft_bd"
create_bd_cell -type ip -vlnv xilinx.com:ip:xfft:* xfft_1
set_property -dict [list \
  CONFIG.aclken {false} \
  CONFIG.data_format [list $dtype] \
  CONFIG.output_ordering {natural_order} \
  CONFIG.super_sample_rates [list $ssr] \
  CONFIG.target_clock_frequency [list $freq] \
  CONFIG.transform_length [list $point_size] \
  CONFIG.input_width [list $data_width] \
] [get_bd_cells xfft_1]
make_bd_intf_pins_external  [get_bd_intf_pins xfft_1/M_AXIS_DATA]
make_bd_intf_pins_external  [get_bd_intf_pins xfft_1/S_AXIS_DATA]
make_bd_pins_external  [get_bd_pins xfft_1/aclk]
generate_target all [get_files project_1/project_1.srcs/sources_1/bd/xfft_bd/xfft_bd.bd]
ipx::package_project -root_dir ip_repo -vendor user.org -library user -taxonomy /UserIP -module xfft_bd -import_files
set_property ipi_drc {ignore_freq_hz false} [ipx::current_core]
set_property sdx_kernel true [ipx::current_core]
set_property sdx_kernel_type rtl [ipx::current_core]
set_property vitis_drc {ctrl_protocol ap_ctrl_hs} [ipx::current_core]
set_property vitis_drc {ctrl_protocol ap_ctrl_none} [ipx::current_core]
set_property ipi_drc {ignore_freq_hz true} [ipx::current_core]
ipx::associate_bus_interfaces -busif M_AXIS_DATA_0 -clock CLK.ACLK_0 [ipx::current_core]
ipx::associate_bus_interfaces -busif S_AXIS_DATA_0 -clock CLK.ACLK_0 [ipx::current_core]
ipx::associate_bus_interfaces -clock CLK.ACLK_0 -reset S_AXIS_aresetn_0 [ipx::current_core]
set_property core_revision 2 [ipx::current_core]
ipx::create_xgui_files [ipx::current_core]
ipx::update_checksums [ipx::current_core]
ipx::check_integrity -kernel -xrt [ipx::current_core]
ipx::save_core [ipx::current_core]
package_xo  -xo_path xo/xfft_bd.xo -kernel_name xfft_bd -ip_directory ip_repo -ctrl_protocol ap_ctrl_none