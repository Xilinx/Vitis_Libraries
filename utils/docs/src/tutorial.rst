.. 
   Copyright 2019-2023 Advanced Micro Devices, Inc
  
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
   :keywords: Vitis, Utils, Vitis Utils Library, Alveo
   :description: Vitis Utils Library is an open-sourced Vitis library written in C++ for commonly used procesing pattern in HLS design.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _brief:

================================
Vitis Utils Library Tutorial
================================


How Vitis Utils Library Works
==================================

AMD Vitis |trade| utils library does not contain any acceleration applications, but consists of utility functions that help the Vitis design. It comes in two parts, HLS hardware utilities and Software utilities.

* HLS hardware utilities are most commonly used HLS design pattern, like Memory Access by AXI, Low latency URAM, Stream combine, and merge. Utils library provides a standard and optimized design to help avoid re-invent the wheels. They are in L1/include/xf_utils_hw.
* Software utilities are commonly used functions in Vitis host design. They are pure C++ design and contains log and error printing functions that help unify testing. They are in L1/include/xf_utils_sw.

HLS Hardware Utiliy API
=========================

Target Audience and Major Features
------------------------------------

Target audience of L1 API are users who are familiar with HLS programming and want to test / profile / modify utility functions.
With the HLS test project provided in L1 layer, you could get:

(1) Function correctness tests, both in c-simulation and co-simulation
(2) Performance profiling from HLS synthesis report and co-simulation
(3) Resource and timing from AMD Vivado |trade| synthesis.

Command to Run cases
-------------------------

.. code-block:: shell

    cd L1/tests/hls_case_folder
    
    make run CSIM=1 CSYNTH=0 COSIM=0 VIVADO_SYN=0 VIVADO_IMPL=0 \
        PLATFORM=/path/to/xilinx_u280_xdma_201920_3.xpfm

Test control variables are:

* ``CSIM`` for high level simulation.
* ``CSYNTH`` for high level synthesis to RTL.
* ``COSIM`` for co-simulation between software test bench and generated RTL.
* ``VIVADO_SYN`` for synthesis by Vivado.
* ``VIVADO_IMPL`` for implementation by Vivado.

For all these variables, setting to ``1`` indicates execution while ``0`` for skipping.
The default value of all these control variables is ``0``, so it can be omitted from command line
if the corresponding step is not wanted.


.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim: