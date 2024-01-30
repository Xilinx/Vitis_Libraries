.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis, Solver, Vitis Solver Library, Alveo
   :description: Vitis Solver Library is an open-sourced Vitis library written in C++ for accelerating solver applications in a variety of use cases.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _brief:

================================
Vitis Solver Library Tutorial
================================


How Vitis Solver Library Works
==================================

AMD Vitis |trade| solver library targets to help Vitis kernel developers to accelerate.
It provides two layers of APIs, namely L1 and L2. Each tackles different calculation needs.

* L1 APIs are mainly for matrix decomposition, including Cholesky inverse, QR fraction, QR inverse, and SVD. They are designed as HLS components and have memory interfaces, which finally point to BRAM / URAM, do not support memory access to DDR / HBM and can not be called directly from the host side.
  L1 APIs support a variety of data types, including float / double / complex. For details of data types support, please take reference from documentation.
  Since L1 APIs need all input data ready in memory, they do have an upper limit to total size of input matrix. For details on how to setup these upper bounds, refer to API's documentation and test cases.
  Matrix in L1 API interface are stored as 2D array.

* L2 APIs are kernels running on FPGA cards, including linear solver, matrix inverse, QR factorization, LU decomposition of square matrix, SPD matrix solver, SPD matrix inverse, Cholesky decomposition, Eigen value decomposition, and dense triangular matrix solver.
  L2 APIs support double precision floating point type. They read DDR/HBM to get the input and write DDR/HBM to output result. They can be called from host side.
  2D Matrix in L2 API interface are row-based stored. 


L2 API
=======

Target Audience and Major Features
------------------------------------

Target audience of L2 API are users who have a certain understanding of HLS and programming on FPGA and want to modify kernels, including:

(1) Operator combinations in kernel, like number of operators or operator pipeline. Most L1 APIs and glue logic inside L2 kernels are connected with streams. You can add more operators into kernels to increase its performance as long as it is not bound by other factors like logic resource and memory bandwidth. 
(2) Building configs to improve frequency or migrate to other AMD FPGA device.
(3) Test correctness in sw-emu, hw-emu and on-board.

Command to Run L2 cases
-------------------------

.. code-block:: shell

    cd L2/tests/vitis_case_folder
    
    # build and run one of the following using your card's platform
    make run TARGET=sw_emu PLATFORM=/path/to/<your_card_platform>
    
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

Target audience of L1 API are users who are familiar with HLS programming and want to test / profile / modify L1 APIs.
With the HLS test project provided in L1 layer, you can get:

(1) Functional correctness tests, both in c-simulation and co-simulation
(2) Performance profiling from HLS synthesis report and co-simulation
(3) Resource and timing from AMD Vivado |trade| synthesis.


Command to Run L1 cases
-------------------------

.. code-block:: shell

    cd L1/tests/hls_case_folder
    
    make run CSIM=1 CSYNTH=0 COSIM=0 VIVADO_SYN=0 VIVADO_IMPL=0 \
        PLATFORM=/path/to/<your_card_platform>

Test control variables are:

* ``CSIM`` for high level simulation.
* ``CSYNTH`` for high level synthesis to RTL.
* ``COSIM`` for co-simulation between software test bench and generated RTL.
* ``VIVADO_SYN`` for synthesis by Vivado.
* ``VIVADO_IMPL`` for implementation by Vivado.

For all these variables, setting to ``1`` indicates execution while ``0`` for skipping.
The default value of all these control variables are ``0``, so they can be omitted from the command line
if the corresponding step is not wanted.

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
