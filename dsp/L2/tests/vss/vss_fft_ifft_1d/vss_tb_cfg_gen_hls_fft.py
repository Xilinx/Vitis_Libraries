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
# this generates the VSS config file based on paramters
import argparse

parser = argparse.ArgumentParser(
    description="Python script that produces cfg file based on the configuration parameters",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)
parser.add_argument(
    "-f", "--cfg_file_name", type=str, help="name of the connectivity cfg file"
)
parser.add_argument("-s", "--ssr", type=int, help="parallelisation factor")
parser.add_argument("-u", "--vss_unit", type=str, help="name of VSS unit")
parser.add_argument("-q", "--freqhz", type=str, help="frequency of PL kernels")
parser.add_argument("-d", "--data_type", type=str, help="data type for the VSS")
parser.add_argument(
    "-l",
    "--aie_obj_name",
    type=str,
    help="Name of the AIE object. Found in the input cfg file.",
)
parser.add_argument(
    "-aie",
    "--aie_variant",
    type=int,
    help="AIE variant",
)
args = parser.parse_args()
SSR = args.ssr
fname = args.cfg_file_name
vssName = args.vss_unit
freq = args.freqhz
dataType = args.data_type
aieName = args.aie_obj_name
aieVariant = args.aie_variant

if aieVariant == 1:
    lpddrName = "LPDDR"
elif aieVariant == 2:
    lpddrName = "LPDDR2"
elif aieVariant == 22:
    lpddrName = "LPDDR01"

f = open(f"{fname}", "w")


if dataType == "cint16":
    common_begin_cfg = f"""
freqhz={freq}:{vssName}_back_transpose.ap_clk,{vssName}_ssr_fft_wrapper_1.ap_clk,{vssName}_ssr_fft_wrapper_2.ap_clk,{vssName}_ssr_fft_wrapper_3.ap_clk,{vssName}_ssr_fft_wrapper_4.ap_clk,mm2s.ap_clk,s2mm.ap_clk,{vssName}_joiner.ap_clk,{vssName}_splitter.ap_clk
"""
else:
    common_begin_cfg = f"""
freqhz={freq}:{vssName}_back_transpose.ap_clk,{vssName}_ssr_fft_wrapper_1.ap_clk,{vssName}_ssr_fft_wrapper_2.ap_clk,mm2s.ap_clk,s2mm.ap_clk,{vssName}_joiner.ap_clk,{vssName}_splitter.ap_clk
"""
common_begin_cfg += f"""
[connectivity]
# ------------------------------------------------------------
# HLS PL Kernels:
# ------------------------------------------------------------

# PL Transpose kernels:
nk = mm2s_wrapper:1:mm2s
nk = s2mm_wrapper:1:s2mm


sp=mm2s.mem:{lpddrName}

# ------------------------------------------------------------
# AXI Stream Connections (PL to AIE)
# ------------------------------------------------------------



"""

f.write(common_begin_cfg)


comment = "# connect mm2s\n"
f.write(comment)
if SSR == 1:
    text = f"sc = mm2s.sig_o:ai_engine_0.{aieName}_PLIO_front_in_0\n"
    f.write(text)
else:
    for i in range(SSR):
        text =(
            f"sc = mm2s.sig_o_{i}:ai_engine_0.{aieName}_PLIO_front_in_{i}\n"
        )
        f.write(text)


comment = "# connect s2mm\n"
f.write(comment)

if SSR == 1:
    text = f"sc = {vssName}_back_transpose.sig_o:s2mm.sig_i\n"
    f.write(text)
else:
    for i in range(SSR):
        text = (
            f"sc = {vssName}_back_transpose.sig_o_{i}:s2mm.sig_i_{i}\n"
        )
        f.write(text)

closing_text = f"""
sp=s2mm.mem:{lpddrName}

# ------------------------------------------------------------
# Vivado PAR
# ------------------------------------------------------------

[vivado]
prop=run.impl_1.steps.phys_opt_design.is_enabled=1
prop=run.impl_1.steps.post_route_phys_opt_design.is_enabled=1


# This enabled unified AIE flow to show AIE resource in Vivado:
param=project.enableUnifiedAIEFlow=true"""

f.write(closing_text)
f.close()
