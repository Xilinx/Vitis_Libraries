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
   :keywords: Vitis, Solver, Vitis Solver Library, Alveo
   :description: Vitis Solver Library is an open-sourced Vitis library written in C++ for accelerating solver applications in a variety of use cases.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _brief:

================================
Vitis Solver Library Tutorial
================================


Linear Solver
==============

Linear solver is application to solve linear system. Systems of linear equations are a common and applicable subset of systems of equations. Solving systems of equations is a very general and important idea, and one that is fundamental in many areas of mathematics, engineering and science.


How Vitis Database Library Works
==================================

Vitis database library targets to help Vitis kernel developers to accelerate.
It provides two layers of APIs, namely L1 and L2. Each tackles different calculation needs.

* L1 APIs are maily for matrix decomposition, including Cholesky inverse, QR fraction, QR inverse and SVD. They are designed as HLS component and have memory interface which finally point to BRAM / URAM, does not support memory access to DDR / HBM and can not be called directly from host side.
  L1 APIs support a variety data type, including float / double / complex. For details of data type support, please take reference from documentation.
  Since L1 APIs need all input data ready in memory, they do have upper limit to total size of input matrix. For details of how to setup these upper bound please take reference to API's documentation and test cases.
  Matrix in L1 API interface are stored as 2D array.

* L2 APIs are kernels running on FPGA cards, including general lineare solver, general matrix inverse, general QR factorization, LU decomposition of square matrix, SPD matrix solver, SPD matrix inverse, Cholesky decomposition, Eigen value decomposition and dense triangular matrix solver.
  L2 APIs support double precision data type. They read DDR/HBM to get input and write DDR/HBM to output result. They could be called from host side.
  2D Matrix in L2 API interface are rows based stored.


L2 API -- GQE kernels
======================

Target Audience and Major Features
------------------------------------

Target audience of L2 API are users who has certian understanding of HLS and programming on FPGA and want to make modification on kernels, including:

(1) Operator combinitions in kernel, like number of operators or operator pipeline. Most L1 APIs and glue logics inside L2 kernels are connect with streams. Users could add more operators into kernels to increase its performance as long as it is not bounded by other factors like logic resource and memory bandwidth. 
(2) Building configs to improve frequency or migrate to other Xilinx FPGA device.
(3) Test correctness in sw-emu, hw-emu and on-board.

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


L1 API
=======

Target Audience and Major Features
------------------------------------

Target audience of L1 API are users who is familiar with HLS programming and want to tests / profile / modify L1 APIs.
With the HLS test project provided in L1 layer, user could get:

(1) Function correctness tests, both in c-simulation and co-simulation
(2) Performance profiling from HLS synthesis report and co-simulaiton
(3) Resource and timing from Vivado synthesis.


Command to Run L1 cases
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
