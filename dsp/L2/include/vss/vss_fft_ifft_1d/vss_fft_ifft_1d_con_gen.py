#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
"""
VSS FFT/IFFT 1D Configuration Generator

This script generates connectivity configuration files for VSS FFT/IFFT 1D implementations
based on specified parameters including SSR, data type, and transpose options.
"""

import argparse

parser = argparse.ArgumentParser(
    description="Generate connectivity cfg file based on configuration parameters",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)
parser.add_argument(
    "-f", "--cfg_file_name", type=str, help="Name of the connectivity cfg file"
)
parser.add_argument("-s", "--ssr", type=int, help="Parallelization factor")
parser.add_argument(
    "-hz",
    "--freqhz",
    type=int,
    help="Frequency of PL components (set to 1/4th of AIE core frequency)",
)
parser.add_argument("-u", "--vss_unit", type=str, help="Name of VSS unit")
parser.add_argument("-v", "--version", type=int, help="Version of IP")
parser.add_argument(
    "-nf",
    "--add_front_transpose",
    type=int,
    help="Set to 1 to include front transpose, 0 otherwise",
)
parser.add_argument(
    "-nb",
    "--add_back_transpose",
    type=int,
    help="Set to 1 to include back transpose, 0 otherwise",
)
parser.add_argument(
    "-d",
    "--data_type",
    type=str,
    help="Data type (cint16, cint32, or cfloat)",
)
parser.add_argument(
    "-l",
    "--aie_obj_name",
    type=str,
    help="Name of the AIE object (found in input cfg file)",
)
parser.add_argument(
    "-aie_bd",
    "--use_aie_buffer_descriptors",
    type=int,
    help="Set to 1 if AIE buffer descriptors can do internal transposes",
)
parser.add_argument(
    "-m",
    "--vss_mode",
    type=int,
    help="VSS mode specification",
)
# optional argument dual_streams
parser.add_argument(
    "-ds",
    "--dual_streams",
    type=int,
    help="Set to 1 to generate config for dual stream AIE implementation (only applicable for AIE variant 1 with port API), 0 otherwise",
)
# Parse arguments
args = parser.parse_args()
SSR = args.ssr
fname = args.cfg_file_name
vss_name = args.vss_unit
freqhz = args.freqhz
data_type = args.data_type
ip_version = str(args.version)
aie_name = str(args.aie_obj_name)
add_front_transpose = int(args.add_front_transpose)
add_back_transpose = int(args.add_back_transpose)
use_aie_buffer_descriptors = int(args.use_aie_buffer_descriptors)
vss_mode = int(args.vss_mode)

# Determine sample size based on data type
if data_type == "cint16":
    sample_size = 32
else:
    sample_size = 64

# Calculate datawidth to match PLIO bandwidth with PL kernel bandwidth
# PL kernels run at 1/4th max PLIO frequency, so PL port width = 4x PLIO width
# Max PLIO frequency allows 32-bit datawidth, hence PL read ports are 128 bits
PL_READ_WIDTH = 128
samples_per_read = PL_READ_WIDTH // sample_size
ssr_int = int(SSR * samples_per_read)

f = open(fname, "w")

# Determine kernel configuration based on transpose settings
hls_kernels = []
rtl_kernels = []
# Build frequency, kernel list, and VSS component strings
freqhz_str = f"freqhz={freqhz}:"
kernel_list_str = ""
vss_comp_list = f"vss = amd:dsplib:{vss_name}:{ip_version}:ai_engine_0,"
rtl_kernel_instances = []

rtl_kernel_instances
if add_front_transpose == 0 and add_back_transpose == 0:
    hls_kernels.extend([
        "mid_transpose_wrapper",
        "splitter_wrapper",
        "joiner_wrapper"
    ])
    for kernel in hls_kernels:
        # Remove 'ifft_' prefix and '_wrapper' suffix for readability
        k_inst_name = kernel[:-8]
        freqhz_str += f"{k_inst_name}.ap_clk,"
        kernel_list_str += f"nk = {kernel}:1:{k_inst_name}\n"
        vss_comp_list += f"{k_inst_name},"
else:
    hls_kernels.extend([
        "ifft_front_transpose_wrapper",
        "ifft_transpose_wrapper",
        "ifft_back_transpose_wrapper"
    ])
    for kernel in hls_kernels:
        # Remove 'ifft_' prefix and '_wrapper' suffix for readability
        k_inst_name = kernel[5:-8]
        freqhz_str += f"{k_inst_name}.ap_clk,"
        kernel_list_str += f"nk = {kernel}:1:{k_inst_name}\n"
        vss_comp_list += f"{k_inst_name},"

if vss_mode == 2:
    rtl_kernels.append("xfft_bd")





freqhz_str = freqhz_str[:-1]

for kernel in rtl_kernels:
    # Create multiple instances for RTL kernels (e.g., xfft_bd_0, xfft_bd_1)
    for inst in range(SSR):
        inst_name = f"{kernel}_{inst}"
        rtl_kernel_instances.append(inst_name)
        vss_comp_list += f"{inst_name},"
        freqhz_str += f",{inst_name}.aclk_0"

    kernel_list_str += f"nk = {kernel}:{SSR}:"
    kernel_list_str += ",".join(rtl_kernel_instances)
    kernel_list_str += "\n"

vss_comp_list = vss_comp_list[:-1] + "\n"
freqhz_str += "\n"

# Write configuration file header
common_begin_cfg = f"""{freqhz_str}
# ------------------------------------------------------------
# PL Kernels:
# ------------------------------------------------------------
[connectivity]
{kernel_list_str}
# ------------------------------------------------------------
# VSS Definition:
# ------------------------------------------------------------
{vss_comp_list}
# ------------------------------------------------------------
# Connections:
# ------------------------------------------------------------
"""

f.write(common_begin_cfg)

# Front transpose connections
if add_front_transpose == 1:
    f.write("# FRONT TRANSPOSE TO AIE:\n")
    if SSR == 1:
        f.write(f"sc = front_transpose.sig_o:ai_engine_0.{aie_name}_PLIO_front_in_0\n")
    else:
        for i in range(SSR):
            f.write(f"sc = front_transpose.sig_o_{i}:ai_engine_0.{aie_name}_PLIO_front_in_{i}\n")

# AIE to PL transpose connections
f.write("# AIE TO PL TRANSPOSE1:\n")

if use_aie_buffer_descriptors:
    if SSR == 1:
        f.write(f"sc = ai_engine_0.{aie_name}_PLIO_front_out_0:mid_transpose.sig_i\n")
    else:
        for i in range(SSR):
            f.write(f"sc = ai_engine_0.{aie_name}_PLIO_front_out_{i}:splitter.sig_i_{i}\n")

    for i in range(ssr_int):
        f.write(f"sc = splitter.sig_i_int_{i}:mid_transpose.sig_i_{i}\n")

    for i in range(ssr_int):
        f.write(f"sc = mid_transpose.sig_o_{i}:joiner.sig_o_int_{i}\n")
else:
    if SSR == 1:
        f.write(f"sc = ai_engine_0.{aie_name}_PLIO_front_out_0:transpose.sig_i\n")
    else:
        for i in range(SSR):
            f.write(f"sc = ai_engine_0.{aie_name}_PLIO_front_out_{i}:transpose.sig_i_{i}\n")

# PL transpose to AIE connections
f.write("# PL TRANSPOSE1 to AIE:\n")

if vss_mode == 1:
    if use_aie_buffer_descriptors:
        if SSR == 1:
            f.write(f"sc = mid_transpose.sig_o:ai_engine_0.{aie_name}_PLIO_back_in_0\n")
        else:
            for i in range(SSR):
                f.write(f"sc = joiner.sig_o_{i}:ai_engine_0.{aie_name}_PLIO_back_in_{i}\n")
    else:
        if SSR == 1:
            f.write(f"sc = transpose.sig_o:ai_engine_0.{aie_name}_PLIO_back_in_0\n")
        else:
            for i in range(SSR):
                f.write(f"sc = transpose.sig_o_{i}:ai_engine_0.{aie_name}_PLIO_back_in_{i}\n")
elif vss_mode == 2:
    if use_aie_buffer_descriptors:
        if SSR == 1:
            f.write("sc = mid_transpose.sig_o:xfft_bd_0.S_AXIS_DATA_0\n")
        else:
            for i in range(SSR):
                f.write(f"sc = joiner.sig_o_{i}:xfft_bd_{i}.S_AXIS_DATA_0\n")
    else:
        if SSR == 1:
            f.write("sc = transpose.sig_o:xfft_bd_0.S_AXIS_DATA_0\n")
        else:
            for i in range(SSR):
                f.write(f"sc = transpose.sig_o_{i}:xfft_bd_{i}.S_AXIS_DATA_0\n")

# Back transpose connections
if add_back_transpose == 1:
    f.write("# AIE TO PL BACK TRANSPOSE:\n")
    if vss_mode == 1:
        if SSR == 1:
            f.write(f"sc = ai_engine_0.{aie_name}_PLIO_back_out_0:back_transpose.sig_i\n")
        else:
            for i in range(SSR):
                f.write(f"sc = ai_engine_0.{aie_name}_PLIO_back_out_{i}:back_transpose.sig_i_{i}\n")
    elif vss_mode == 2:
        if SSR == 1:
            f.write("sc = xfft_bd_0.M_AXIS_DATA_0:back_transpose.sig_i\n")
        else:
            for i in range(SSR):
                f.write(f"sc = xfft_bd_{i}.M_AXIS_DATA_0:back_transpose.sig_i_{i}\n")

# Write Vivado configuration
closing_text = """
# ------------------------------------------------------------
# Vivado PAR
# ------------------------------------------------------------

[vivado]
# Implementation strategies (commented out by default)
#impl.strategies=Performance_Explore,Performance_ExplorePostRoutePhysOpt,Performance_ExtraTimingOpt
#impl.strategies=Congestion_SpreadLogic_high
#impl.jobs=8

# Enable physical optimization steps
prop=run.impl_1.steps.phys_opt_design.is_enabled=1
prop=run.impl_1.steps.post_route_phys_opt_design.is_enabled=1

# Optimization directives (commented out by default)
#prop=run.impl_1.steps.opt_design.args.directive=SpreadLogic_high
#prop=run.impl_1.steps.place_design.args.directive=SpreadLogic_high
#prop=run.impl_1.steps.phys_opt_design.args.directive=SpreadLogic_high
#prop=run.impl_1.steps.route_design.args.directive=SpreadLogic_high

# Enable unified AIE flow to show AIE resources in Vivado
param=project.enableUnifiedAIEFlow=true
"""

f.write(closing_text)
f.close()
