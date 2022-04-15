.. 
   Copyright 2019-2020 Xilinx, Inc.
  
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

Vitis utils library does not contain any acceleration applications, but utility functions that help Vitis design. It comes in two parts: HLS hardware utilities, Software utilities and Datamover.
* HLS hardware utilities are most commonly used HLS design pattern, like Memory Access by AXI, Low latency URAM, Stream combine and merge. Utils library provide standard and optimized design to help avoid re-invent the wheels. They're in L1/include/xf_utils_hw.
* Software utilities are commonly used functions in Vitis host design. They are pure C++ design and contains log and error printing functions which help unify testing. They're in L1/include/xf_utils_sw.
* Datamover are kernels load data from DDR/HBM to streams and write data from streams to DDR/HBM. They help AIE to access data in better bandwidth. They're in L2/src and need run python script to generate final source code. Please take reference test cases from L2/tests/datamover.

HLS hardware utiliy API
=========================

Target Audience and Major Features
------------------------------------

Target audience of L1 API are users who is familiar with HLS programming and want to tests / profile / modify utility functions.
With the HLS test project provided in L1 layer, user could get:

(1) Function correctness tests, both in c-simulation and co-simulation
(2) Performance profiling from HLS synthesis report and co-simulaiton
(3) Resource and timing from Vivado synthesis.

Command to Run cases
-------------------------

.. code-block:: shell

    cd L1/tests/hls_case_folder
    
    make run CSIM=1 CSYNTH=0 COSIM=0 VIVADO_SYN=0 VIVADO_IMPL=0 \
        DEVICE=/path/to/xilinx_u280_xdma_201920_3.xpfm

Test control variables are:

* ``CSIM`` for high level simulation.
* ``CSYNTH`` for high level synthesis to RTL.
* ``COSIM`` for co-simulation between software test bench and generated RTL.
* ``VIVADO_SYN`` for synthesis by Vivado.
* ``VIVADO_IMPL`` for implementation by Vivado.

For all these variables, setting to ``1`` indicates execution while ``0`` for skipping.
The default value of all these control variables are ``0``, so they can be omitted from command line
if the corresponding step is not wanted.


Datamover
==========

Target Audience and Major Features
------------------------------------

Target audience of datamover are AIE application designer who want to move data between AIE and DDR/HBM.
Current datamover is a template design and need pre-build to generate final source code. Please take reference of Makefile of test cases in L2/datamover to check how to run pre-build. Current datamover design support datamovement: (1) AXI master to stream (2) Stream to AXI master (3) RAM to stream (4) Stream to RAM.
Although design for AIE application, test cases don't directly include an AIE program. They use another dummy kernel to consume/generate data, mimicing an AIE program that read/write streams. Dummy kernels connect with datamover kernel by AXI streams, just like datamover connection with AIE array.


Command to Run L2 cases
-------------------------

.. code-block:: shell

    cd L2/tests/vitis_case_folder
    
    # build and run one of the following using U280 platform
    make run TARGET=sw_emu DEVICE=/path/to/xilinx_u280_xdma_201920_3.xpfm
    
    # delete generated files
    make cleanall

Here, ``TARGET`` decides the FPGA binary type

* ``sw_emu`` is for software emulation
* ``hw_emu`` is for hardware emulation
* ``hw`` is for deployment on physical card. (Compilation to hardware binary often takes hours.)

Besides ``run``, the Vitis case makefile also allows ``host`` and ``xclbin`` as build target.

