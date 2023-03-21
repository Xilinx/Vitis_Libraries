.. 
   Copyright 2019 Xilinx, Inc.
  
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

Vitis Ultrasound library provides implementation of different L1/L2/L3 APIs Toolbox for ultrasound image processing. For library users, these APIs contains source code for AIE & PL kernels. The testbench is also provided for simulation and verification for each stand alone cases.

Code structures enhancement
----------------------------

- L1: all L1 BLAS apis could be referenced by including "L1/include/", different apis sources files (AIE kernel code) locate in separated directory.
- L2: all L2 ultrasound component apis could be reference by including "L1/include/", different apis shareing golbal settings from "L1/include/kernels.hpp".
- L3, all 3 example of connected beamformer (PW/SA/Scanline) are located in separated directory under "L3/demos", each contain self contain data and source code.
- Design code structure enahancement: for L2/L3 cases, source code for differenct device are well orgnized. The structure is showed as bellow:

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