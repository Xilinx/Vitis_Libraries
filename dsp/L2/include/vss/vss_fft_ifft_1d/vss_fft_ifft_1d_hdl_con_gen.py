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
    "-nb",
    "--add_back_transpose",
    type=int,
    help="Flag that returns a packages vss with the back transpose. If set to 0, the vss will not include the back transpose block.",
)
args = parser.parse_args()
SSR = args.ssr
fname = args.cfg_file_name
vssName = args.vss_unit
freqhz = args.freqhz
ipVersion = str(args.version)
addBackTranspose = int(args.add_back_transpose)

f = open(f"{fname}", "w")

if addBackTranspose == 0:
    common_begin_cfg = f"""

freqhz={freqhz}:ssr_fft.aclk

[connectivity]
# ------------------------------------------------------------
# HLS PL Kernels:
# ------------------------------------------------------------

# PL Transpose kernels:
nk = parallel_fft:1:ssr_fft


vss = amd:dsplib:{vssName}:{ipVersion}:ssr_fft,ai_engine_0


# ------------------------------------------------------------
# AXI Stream Connections (PL to AIE)
# ------------------------------------------------------------

"""
else:
    common_begin_cfg = f"""

freqhz={freqhz}:ssr_fft.aclk,back_transpose.ap_clk

[connectivity]
# ------------------------------------------------------------
# HLS PL Kernels:
# ------------------------------------------------------------

# PL Transpose kernels:
nk= back_transpose_simple_wrapper:1:back_transpose
nk = parallel_fft:1:ssr_fft


vss = amd:dsplib:{vssName}:{ipVersion}:back_transpose,ssr_fft,ai_engine_0


# ------------------------------------------------------------
# AXI Stream Connections (PL to AIE)
# ------------------------------------------------------------



"""

f.write(common_begin_cfg)


comment = "# AIE FFT TO PL FFT:\n"
f.write(comment)

if SSR == 1:
    text = "sc = ai_engine_0.fft_aie_PLIO_front_out_0" + ":ssr_fft.s_axis0_din" + "\n"
    f.write(text)
else:
    for i in range(SSR):
        text = (
            "sc = ai_engine_0.fft_aie_PLIO_front_out_"
            + str(i)
            + ":ssr_fft.s_axis"
            + str(i)
            + "_din\n"
        )
        f.write(text)

if addBackTranspose == 1:
    comment = "# PL FFT TO PL TRANSPOSE:\n"
    f.write(comment)

    if SSR == 1:
        text = "sc = ssr_fft.m_axis1_dout" + ":back_transpose.sig_i" + "\n"
        f.write(text)
    else:
        for i in range(SSR):
            text = (
                "sc = ssr_fft.m_axis"
                + str(i)
                + "_dout:back_transpose.sig_i_"
                + str(i)
                + "\n"
            )
            f.write(text)


closing_text = f"""

# ------------------------------------------------------------
# Vivado PAR
# ------------------------------------------------------------

[vivado]


# This enabled unified AIE flow to show AIE resource in Vivado:
param=project.enableUnifiedAIEFlow=true"""

f.write(closing_text)
f.close()
