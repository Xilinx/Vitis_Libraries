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
import argparse

parser = argparse.ArgumentParser(
    description="Python script that produces cfg file based on the configuration parameters",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)
parser.add_argument(
    "-f", "--cfg_file_name", type=str, help="name of the connectivity cfg file"
)
parser.add_argument("-s", "--ssr", type=int, help="parallelisation factor")
parser.add_argument(
    "-hz",
    "--freqhz",
    type=int,
    help="frequency of PL components. Set this to be 1/4th of the core frequency of the AIE device",
)
parser.add_argument("-u", "--vss_unit", type=str, help="name of vss unit")
parser.add_argument("-v", "--version", type=int, help="Version of IP")
parser.add_argument(
    "-nf",
    "--add_front_transpose",
    type=int,
    help="Set flag to 1 to return a packaged vss including the back transpose. Else set to 0.",
)
parser.add_argument(
    "-nb",
    "--add_back_transpose",
    type=int,
    help="Set flag to 1 to return a packaged vss including the front transpose. Else set to 0.",
)
parser.add_argument(
    "-d",
    "--data_type",
    type=str,
    help="Data type. Set to cint16, cint32, cfloat",
)
parser.add_argument(
    "-l",
    "--aie_obj_name",
    type=str,
    help="Name of the AIE object. Found in the input cfg file.",
)
parser.add_argument(
    "-aie_bd",
    "--use_aie_buffer_descriptors",
    type=int,
    help="Specifies whether AIE buffer descriptors have the ability to do transposes internally.",
)
args = parser.parse_args()
SSR = args.ssr
fname = args.cfg_file_name
vssName = args.vss_unit
freqhz = args.freqhz
dataType = args.data_type
ipVersion = str(args.version)
aieName = str(args.aie_obj_name)
addFrontTpose = int(args.add_front_transpose)
addBackTpose = int(args.add_back_transpose)
use_aie_buffer_descriptors = int(args.use_aie_buffer_descriptors)

if dataType == "cint16":
    sample_size = 32
else:
    sample_size = 64
    
plReadWidth = 128
ssrInt = (int)(SSR*(plReadWidth/sample_size))
f = open(f"{fname}", "w")

if addFrontTpose == 0 and addBackTpose == 0:
    common_begin_cfg = f"""

freqhz={freqhz}:transpose.ap_clk,splitter.ap_clk,joiner.ap_clk
#freqhz={freqhz}:transpose.ap_clk

[connectivity]
# ------------------------------------------------------------
# HLS PL Kernels:
# ------------------------------------------------------------

# PL Transpose kernels:
nk = mid_transpose_wrapper:1:transpose
nk = splitter_wrapper:1:splitter
nk = joiner_wrapper:1:joiner


vss = amd:dsplib:{vssName}:{ipVersion}:splitter,joiner,transpose,ai_engine_0
#vss = amd:dsplib:{vssName}:{ipVersion}:transpose,ai_engine_0


# ------------------------------------------------------------
# AXI Stream Connections (PL to AIE)
# ------------------------------------------------------------

"""
elif addBackTpose == 1 and addFrontTpose == 0:
    common_begin_cfg = f"""

freqhz={freqhz}:transpose.ap_clk,back_transpose.ap_clk

[connectivity]
# ------------------------------------------------------------
# HLS PL Kernels:
# ------------------------------------------------------------

# PL Transpose kernels:
nk = ifft_transpose_wrapper:1:transpose
nk = ifft_back_transpose_wrapper:1:back_transpose

vss = amd:dsplib:{vssName}:{ipVersion}:back_transpose,transpose,ai_engine_0


# ------------------------------------------------------------
# AXI Stream Connections (PL to AIE)
# ------------------------------------------------------------

"""
elif addFrontTpose == 1 and addBackTpose == 0:
    common_begin_cfg = f"""

freqhz={freqhz}:front_transpose.ap_clk,transpose.ap_clk

[connectivity]
# ------------------------------------------------------------
# HLS PL Kernels:
# ------------------------------------------------------------

# PL Transpose kernels:
nk = ifft_front_transpose_wrapper:1:front_transpose
nk = ifft_transpose_wrapper:1:transpose

vss = amd:dsplib:{vssName}:{ipVersion}:front_transpose,transpose,ai_engine_0


# ------------------------------------------------------------
# AXI Stream Connections (PL to AIE)
# ------------------------------------------------------------

"""
else:
    common_begin_cfg = f"""

freqhz={freqhz}:front_transpose.ap_clk,transpose.ap_clk,back_transpose.ap_clk

[connectivity]
# ------------------------------------------------------------
# HLS PL Kernels:
# ------------------------------------------------------------

# PL Transpose kernels:
nk = ifft_front_transpose_wrapper:1:front_transpose
nk = ifft_back_transpose_wrapper:1:back_transpose
nk = ifft_transpose_wrapper:1:transpose
#nk = mid_transpose_wrapper:1:transpose
#nk = splitter_wrapper:1:splitter
#nk = joiner_wrapper:1:joiner

vss = amd:dsplib:{vssName}:{ipVersion}:front_transpose,back_transpose,transpose,ai_engine_0
#vss = amd:dsplib:{vssName}:{ipVersion}:splitter,joiner,front_transpose,back_transpose,transpose,ai_engine_0


# ------------------------------------------------------------
# AXI Stream Connections (PL to AIE)
# ------------------------------------------------------------



"""

f.write(common_begin_cfg)


if addFrontTpose == 1:
    comment = "# FRONT TRANSPOSE TO AIE:\n"
    f.write(comment)

    if SSR == 1:
        text = f'''sc = front_transpose.sig_o:ai_engine_0.{aieName}_PLIO_front_in_0\n'''
        f.write(text)
    else:
        for i in range(SSR):
            text = f'''sc = front_transpose.sig_o_{i}:ai_engine_0.{aieName}_PLIO_front_in_{i}\n'''
            f.write(text)


comment = "# AIE TO PL TRANSPOSE1:\n"
f.write(comment)

if use_aie_buffer_descriptors:
    if SSR == 1:
        text = f"sc = ai_engine_0.{aieName}_PLIO_front_out_0:transpose.sig_i\n"
        f.write(text)
    else:
        for i in range(SSR):
            text = f'''sc = ai_engine_0.{aieName}_PLIO_front_out_{i}:splitter.sig_i_{i}\n'''
            f.write(text)

    for i in range(ssrInt):
        text = f'''sc = splitter.sig_i_int_{i}:transpose.sig_i_{i}\n'''
        f.write(text)
    
    for i in range(ssrInt):
        text = f'''sc = transpose.sig_o_{i}:joiner.sig_o_int_{i}\n'''
        f.write(text)

else:
    if SSR == 1:
        text = f"sc = ai_engine_0.{aieName}_PLIO_front_out_0:transpose.sig_i\n"
        f.write(text)
    else:
        for i in range(SSR):            
            text = f'''sc = ai_engine_0.{aieName}_PLIO_front_out_{i}:transpose.sig_i_{i}\n'''
            f.write(text)

comment = "# PL TRANSPOSE1 to AIE:\n"
f.write(comment)

if use_aie_buffer_descriptors:
    if SSR == 1:
        text = f'''sc = transpose.sig_o:ai_engine_0.{aieName}_PLIO_back_in_0\n'''
        f.write(text)
    else:
        for i in range(SSR):
            text = (
                f'''sc = joiner.sig_o_{i}:ai_engine_0.{aieName}_PLIO_back_in_{i}\n'''
            )
            f.write(text)
else:
    if SSR == 1:
        text = f'''sc = transpose.sig_o:ai_engine_0.{aieName}_PLIO_back_in_0\n'''
        f.write(text)
    else:
        for i in range(SSR):
            text = f'''sc = transpose.sig_o_{i}:ai_engine_0.{aieName}_PLIO_back_in_{i}\n'''
            f.write(text)

if addBackTpose == 1:
    comment = "# AIE TO PL BACK TRANSPOSE:\n"
    f.write(comment)

    if SSR == 1:
        text = f'''sc = ai_engine_0.{aieName}_PLIO_back_out_0:back_transpose.sig_i\n'''
        f.write(text)
    else:
        for i in range(SSR):
            text = f'''sc = ai_engine_0.{aieName}_PLIO_back_out_{i}:back_transpose.sig_i_{i}\n'''
            f.write(text)

closing_text = f"""

# ------------------------------------------------------------
# Vivado PAR
# ------------------------------------------------------------

[vivado]
#impl.strategies=Performance_Explore,Performance_ExplorePostRoutePhysOpt,Performance_ExtraTimingOpt
#impl.strategies=Congestion_SpreadLogic_high
#impl.jobs=8
prop=run.impl_1.steps.phys_opt_design.is_enabled=1
prop=run.impl_1.steps.post_route_phys_opt_design.is_enabled=1

#prop=run.impl_1.steps.opt_design.args.directive=SpreadLogic_high
#prop=run.impl_1.steps.place_design.args.directive=SpreadLogic_high
#prop=run.impl_1.steps.phys_opt_design.args.directive=SpreadLogic_high
#prop=run.impl_1.steps.route_design.args.directive=SpreadLogic_high

# This enabled unified AIE flow to show AIE resource in Vivado:
param=project.enableUnifiedAIEFlow=true"""

f.write(closing_text)
f.close()
