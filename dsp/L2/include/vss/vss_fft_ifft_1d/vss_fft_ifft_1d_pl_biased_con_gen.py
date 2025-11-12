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
    help="Flag that returns a packages vss including the back transpose",
)
parser.add_argument(
    "-d",
    "--data_type",
    type=str,
    help="Data type for the VSS",
)
parser.add_argument(
    "-l",
    "--aie_obj_name",
    type=str,
    help="Name of the AIE object. Found in the input cfg file.",
)

args = parser.parse_args()
SSR = args.ssr
fname = args.cfg_file_name
vssName = args.vss_unit
freqhz = args.freqhz
ipVersion = str(args.version)
addBackTranspose = int(args.add_back_transpose)
aieName = str(args.aie_obj_name)
dataType = str(args.data_type)

if dataType == "cint16":
    aie_buff_descriptors_works = True
    sample_size = 32
else:
    aie_buff_descriptors_works = False
    sample_size = 64
plReadWidth = 128
samplesPerRead = (int)(plReadWidth / sample_size)
ssrInt = (int)(SSR*(plReadWidth/sample_size))

f = open(f"{fname}", "w")

if addBackTranspose == 0:
    common_begin_cfg = f"""

freqhz={freqhz}:ssr_fft.ap_clk

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
    if dataType == "cint16":
        common_begin_cfg = f"""
freqhz={freqhz}:ssr_fft_wrapper_1.ap_clk,ssr_fft_wrapper_2.ap_clk,ssr_fft_wrapper_3.ap_clk,ssr_fft_wrapper_4.ap_clk,back_transpose.ap_clk,splitter.ap_clk,joiner.ap_clk"""
    else:
        common_begin_cfg = f"""
freqhz={freqhz}:ssr_fft_wrapper_1.ap_clk,ssr_fft_wrapper_2.ap_clk,back_transpose.ap_clk,splitter.ap_clk,joiner.ap_clk"""

    common_begin_cfg += f"""
[connectivity]
# ------------------------------------------------------------
# HLS PL Kernels:
# ------------------------------------------------------------

# PL Transpose kernels:
nk= back_transpose_simple_wrapper:1:back_transpose
nk = joiner_wrapper:1:joiner
nk = splitter_wrapper:1:splitter
nk = ssr_fft_wrapper:{samplesPerRead}
"""

    if dataType == "cint16":
        common_begin_cfg += f"""
vss = amd:dsplib:{vssName}:{ipVersion}:back_transpose,ssr_fft_wrapper_1,ssr_fft_wrapper_2,ssr_fft_wrapper_3,ssr_fft_wrapper_4,ai_engine_0,splitter,joiner
"""
    else:
        common_begin_cfg += f"""
vss = amd:dsplib:{vssName}:{ipVersion}:back_transpose,ssr_fft_wrapper_1,ssr_fft_wrapper_2,ai_engine_0,splitter,joiner
"""

common_begin_cfg += f"""
# ------------------------------------------------------------
# AXI Stream Connections (PL to AIE)
# ------------------------------------------------------------



"""

f.write(common_begin_cfg)


comment = "# AIE FFT TO PL FFT:\n"
f.write(comment)

if SSR == 1:
    text = "sc = ai_engine_0.{aieName}_PLIO_front_out_0" + ":ssr_fft.inData" + "\n"
    f.write(text)
else:
    for i in range(SSR):
        text = f'''sc = ai_engine_0.{aieName}_PLIO_front_out_{i}:splitter.sig_i_{i}\n'''
        f.write(text)
    for i in range(SSR):
        for samp in range(samplesPerRead):
            curStream = samp*SSR+i
            text = f'''sc = splitter.sig_i_int_{curStream}:ssr_fft_wrapper_{samp+1}.inData_{i}\n'''
            f.write(text)
    for i in range(SSR):
        for samp in range(samplesPerRead):
            curStream = samp*SSR+i
            text = f'''sc = ssr_fft_wrapper_{samp+1}.outData_{i}:joiner.sig_o_int_{curStream}\n'''
            f.write(text)



if addBackTranspose == 1:
    comment = "# PL FFT TO PL TRANSPOSE:\n"
    f.write(comment)

    if SSR == 1:
        text = "sc = ssr_fft.outData" + ":back_transpose.sig_i" + "\n"
        f.write(text)
    else:
        for i in range(SSR):
            text = f'''sc = joiner.sig_o_{i}:back_transpose.sig_i_{i}\n'''
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
