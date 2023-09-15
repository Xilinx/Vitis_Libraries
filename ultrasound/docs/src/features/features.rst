.. 
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

.. _features:

Features for Vitis Ultrasound Library 
=====================================

.. toctree::
   :hidden:
   :maxdepth: 1

Before 23.2 release, the provied toolbox-style APIs focus on providing extreme performance but rely on many PL modules for sub-graph connection and parameter generation. In 23.2 release, the library provides an all-in-AIE scanline implementation. Thus, users can not only combinate algorithms in bottom-to-top way by using toolbox-style APIs, but also start from the all-in-AIE scanline implementation to obtain the final design by modifying some graphs.

A set of C-models of scanline are provided which can be seen as a step-by-step flow from algorithm-end to AIE-end. The C-model functions can also be used to generate input and verify output simultaneously and conveniently. 

Code structures enhancement
----------------------------

- L1: all L1 BLAS kernel APIs (toolbox) files locate in separated directories with their orinal name.
- L1: all L1 algorithm kernel APIs (all-in-AIE) files locate in separated directories beginning with "kernel_".
- L2: all L2 ultrasound toolbox graph APIs are shareing global settings from "L1/include/kernels.hpp".
- L2: all L2 ultrasound all-in-AIE graph APIs beginning with "graph_" are shareing global settings from "L1/include/kernel_*.hpp".
- L3: all 4 L3 example (Scanline_AllinAIE/PW/SA/Scanline) are located in separated directory under "L3/tests", each contain self contain data and source code.
- L3: the scanline algorithm C-models are located in separated directory under "L3/models".

For L2/L3 cases, source code for differenct device are well orgnized. The structure is showed as bellow:

.. code-block:: txt

   aie_graph:     aie graph define
   PL_kernels:    HLS C/C++ code for PL kernels 
   PS_host:       XRT based host code for VCK190
   data:          input data & output golden data
   system.cfg:    hw platform descriptions

Host code enhancement
---------------------

For L2/L3 cases, XRT based PS host code are provided in each directory of "<path_to_apis>/PS_host/". The host code read input simulation data and call on aie graph by 1 run and also evoke PL kernels of "mm2s" and "s2mm" for PL side data input/output. The result are compared to "data/golden" for verify the design.
 
Support for AIE full verification flow on VCK190 platform
-----------------------------------------------------------

This library provide the detailed example for full aie design/verification flow, which contains x86sim/aiesim/sw_emu(ps_on_x86)/hw_emu/hw. The testbench support "make run TARGET=*" from basic functional verfication to final hw performance test, and also contain godlen file check. Users could check performance or funcional correctness through each of the verification depth, in this toolbox library for vck190 based aie design.