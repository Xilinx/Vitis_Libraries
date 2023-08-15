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

.. meta::
   :keywords: BLAS, Library, Vitis BLAS Library, primitives, L1 test
   :description: Vitis BLAS library L1 primitive implementations have been tested in vitis tools.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _user_guide_test_l1:

*******************************
L1 Test
*******************************

.. code-block:: bash

    source <install path>/Vitis/2023.2/settings64.sh
    source /opt/xilinx/xrt/setup.sh
    cd L1/tests/case_folder/
    make run TARGET=<cosim/csim/csynth/vivado_syn/vivado_impl> PLATFORM=/path/to/xilinx_u250_gen3x16_xdma_3_1_202020_1.xpfm

- `csim` (high level simulation),
- `csynth` (high level synthesis to RTL),
- `cosim` (cosimulation between software testbench and generated RTL),
- `vivado_syn` (synthesis by Vivado) and
- `vivado_impl` (implementation by Vivado).

