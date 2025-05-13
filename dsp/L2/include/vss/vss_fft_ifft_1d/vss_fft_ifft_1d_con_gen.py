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
args = parser.parse_args()
SSR = args.ssr
fname = args.cfg_file_name
vssName = args.vss_unit
freqhz = args.freqhz
ipVersion = str(args.version)
addFrontTpose = int(args.add_front_transpose)
addBackTpose = int(args.add_back_transpose)

f = open(f"{fname}", "w")

if addFrontTpose == 0 and addBackTpose == 0:
    common_begin_cfg = f"""

freqhz={freqhz}:transpose.ap_clk

[connectivity]
# ------------------------------------------------------------
# HLS PL Kernels:
# ------------------------------------------------------------

# PL Transpose kernels:
nk = ifft_transpose_wrapper:1:transpose


vss = amd:dsplib:{vssName}:{ipVersion}:transpose,ai_engine_0


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


vss = amd:dsplib:{vssName}:{ipVersion}:front_transpose,back_transpose,transpose,ai_engine_0


# ------------------------------------------------------------
# AXI Stream Connections (PL to AIE)
# ------------------------------------------------------------



"""

f.write(common_begin_cfg)


if addFrontTpose == 1:
    comment = "# FRONT TRANSPOSE TO AIE:\n"
    f.write(comment)

    if SSR == 1:
        text = (
            "sc = front_transpose.sig_o" + ":ai_engine_0.fft_aie_PLIO_front_in_0" + "\n"
        )
        f.write(text)
    else:
        for i in range(SSR):
            text = (
                "sc = front_transpose.sig_o_"
                + str(i)
                + ":ai_engine_0.fft_aie_PLIO_front_in_"
                + str(i)
                + "\n"
            )
            f.write(text)


comment = "# AIE TO PL TRANSPOSE1:\n"
f.write(comment)

if SSR == 1:
    text = "sc = ai_engine_0.fft_aie_PLIO_front_out_0" + ":transpose.sig_i" + "\n"
    f.write(text)
else:
    for i in range(SSR):
        text = (
            "sc = ai_engine_0.fft_aie_PLIO_front_out_"
            + str(i)
            + ":transpose.sig_i_"
            + str(i)
            + "\n"
        )
        f.write(text)

comment = "# PL TRANSPOSE1 to AIE:\n"
f.write(comment)

if SSR == 1:
    text = "sc = transpose.sig_o" + ":ai_engine_0.fft_aie_PLIO_back_in_0" + "\n"
    f.write(text)
else:
    for i in range(SSR):
        text = (
            "sc = transpose.sig_o_"
            + str(i)
            + ":ai_engine_0.fft_aie_PLIO_back_in_"
            + str(i)
            + "\n"
        )
        f.write(text)

if addBackTpose == 1:
    comment = "# AIE TO PL BACK TRANSPOSE:\n"
    f.write(comment)

    if SSR == 1:
        text = (
            "sc = ai_engine_0.fft_aie_PLIO_back_out_0" + ":back_transpose.sig_i" + "\n"
        )
        f.write(text)
    else:
        for i in range(SSR):
            text = (
                "sc = ai_engine_0.fft_aie_PLIO_back_out_"
                + str(i)
                + ":back_transpose.sig_i_"
                + str(i)
                + "\n"
            )
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
