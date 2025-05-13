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
args = parser.parse_args()
SSR = args.ssr
fname = args.cfg_file_name
vssName = args.vss_unit
freq = args.freqhz

f = open(f"{fname}", "w")

common_begin_cfg = f"""
freqhz={freq}:{vssName}_back_transpose.ap_clk,{vssName}_ssr_fft.aclk,mm2s.ap_clk,s2mm.ap_clk

[connectivity]
# ------------------------------------------------------------
# HLS PL Kernels:
# ------------------------------------------------------------

# PL Transpose kernels:
nk = mm2s_wrapper:1:mm2s
nk = s2mm_wrapper:1:s2mm


sp=mm2s.mem:LPDDR

# ------------------------------------------------------------
# AXI Stream Connections (PL to AIE)
# ------------------------------------------------------------



"""

f.write(common_begin_cfg)


comment = "# connect mm2s\n"
f.write(comment)
if SSR == 1:
    text = "sc = mm2s.sig_o:" + "ai_engine_0.fft_aie_PLIO_front_in_0" + "\n"
    f.write(text)
else:
    for i in range(SSR):
        text = (
            "sc = mm2s.sig_o_"
            + str(i)
            + ":"
            + "ai_engine_0.fft_aie_PLIO_front_in_"
            + str(i)
            + "\n"
        )
        f.write(text)


comment = "# connect s2mm\n"
f.write(comment)

if SSR == 1:
    text = "sc = " + str(vssName) + "_back_transpose.sig_o" + ":s2mm.sig_i" + "\n"
    f.write(text)
else:
    for i in range(SSR):
        text = (
            "sc = "
            + str(vssName)
            + "_back_transpose.sig_o_"
            + str(i)
            + ":s2mm.sig_i_"
            + str(i)
            + "\n"
        )
        f.write(text)


closing_text = f"""
sp=s2mm.mem:LPDDR

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
