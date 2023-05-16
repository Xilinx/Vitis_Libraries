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
   :keywords: Vitis, Security, Vitis Security Library, Alveo
   :description: Vitis Security Library is an open-sourced Vitis library written in C++ for accelerating security applications in a variety of use cases.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _brief:

================================
Vitis Security Library Tutorial
================================


Crypto Algorithm Hardware Acceleration
=======================================

Crypto algorithms play an important role in transfer, verification, storage and processing of data.
Crypto application includes encryption/decryption, checksum, hash, signature/verification, etc.
Modern crypto algorithms are complicated and keep evolving, especially in new areas like block-chain application.
FPGA is suitable for a wide variety of crypto algorithms and able to provide competitive or even better performance than CPU.
It is also convenient to construct a processing pipeline while crypto processing is one of the pipeline stages, which will save bandwidth cost and improve performance.
Last but not the least, FPGA could have multiple compute units working in parallel, which will also improve total performance.

.. image:: /images/pipeline_processing.png
   :alt: Execution Plan 
   :scale: 20%
   :align: center

How Vitis Security Library Works
==================================

Vitis security library targets to help Vitis kernel developers to accelerate crypto algorithms.
It provides three layers of APIs, namely L1 / L2 / L3.
L1 APIs are the core for acceleration while L2 and L3 only contains a few specialized cases.

L1 API
=======

Target Audience and Major Features
------------------------------------

Target audience of L1 API are users who is familiar with HLS programming and want to test / profile / modify operators or add new APIs.
With the HLS test project provided in L1 layer, user could get:

(1) Function correctness tests, both in c-simulation and co-simulation
(2) Performance profiling from HLS synthesis report and co-simulaiton
(3) Resource and timing from Vivado synthesis.


Input / output interface
--------------------------

For easy connection with other HLS components, most of security library L1 API takes hls::stream interface.
For APIs which could take variable length of input, API will include either scalar parameter for input length or "end flag" stream working with data stream in a 1:1 fashion to tell if this is the last block of input data.
For details of interface definition and how padding will happen in stream interface, please take reference to documentation and corresponding test cases.

Command to Run L1 cases
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
The default value of all these control variables are ``0``, so they can be omitted from command line
if the corresponding step is not wanted.
