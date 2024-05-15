#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
 
parser = argparse.ArgumentParser(description="Python script that produces cfg file based on the configuration parameters",
                                 formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument("-f", "--cfg-file-name", action="store_true", help="name of the connectivity cfg file")
parser.add_argument("-s", "--ssr", type=int, help="parallelisation factor")
args = parser.parse_args()
SSR = args.ssr
f = open("system.cfg", "w")

common_begin_cfg = (f"""
                    
freqhz=312500000:dma_src.ap_clk,transpose.ap_clk,dma_snk.ap_clk

[connectivity]

# ------------------------------------------------------------
# HLS PL Kernels:
# ------------------------------------------------------------

# PL Transpose kernels:
nk = ifft_transpose_wrapper:1:transpose

# Sources/Sinks Data Movers to/from LPDDR/PL:
nk = ifft_dma_src_wrapper:1:dma_src
nk = ifft_dma_snk_wrapper:1:dma_snk

# ------------------------------------------------------------
# AXI Stream Connections (PL to AIE)
# ------------------------------------------------------------

# LPDDR to PL DMA SOURCE:
sp=dma_src.mem:LPDDR

# PL DMA Source to AIE: 
""")

f.write(common_begin_cfg)

for i in range(SSR):
    text = "sc = dma_src.sig_o_" + str(i) + ":ai_engine_0.PLIO_front_in_" + str(i) + "\n"
    f.write(text)

comment = "# AIE TO PL TRANSPOSE1:\n"
f.write(comment)

for i in range(SSR):
    text = "sc = ai_engine_0.PLIO_front_out_" + str(i) + ":transpose.sig_i_" + str(i) + "\n"
    f.write(text)

comment = "# PL TRANSPOSE1 to AIE:\n"
f.write(comment)

for i in range(SSR):
    text = "sc = transpose.sig_o_" + str(i) + ":ai_engine_0.PLIO_back_in_" + str(i) + "\n"
    f.write(text)

comment = "# AIE TO PL DMA SINK:\n"
f.write(comment)

for i in range(SSR):
    text = "sc = ai_engine_0.PLIO_back_out_" + str(i) + ":dma_snk.sig_i_" + str(i) + "\n"
    f.write(text)

closing_text=(f"""
# PL DMA SINK to LPDDR
sp=dma_snk.mem:LPDDR

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
param=project.enableUnifiedAIEFlow=true""")

f.write(closing_text)
f.close()
