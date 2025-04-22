.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis, Analytics, Vitis Data Analytics Library, Alveo
   :description: Vitis Data Analytics Library is an open-sourced Vitis library written in C++ for accelerating data analytics applications in a variety of use cases.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _brief:

=====================================
Vitis Data Analytics Library Tutorial
=====================================

Data Analytics and Hardware Acceleration
=====================================

Data analytics aims to accelerate a variety applications including data mining, text processing, formatting from raw data to dataframe, and querying operations on spatial data.

As an typical use case of data analysis, CSV Scanner is used to convert csv data into formatted data storage for subsequent analysis. An AMD Alveo™ card can help improve the CSV Scanner's performance in the following way: 

(1) Instruction parallelism by creating a customized and long pipeline.
(2) Data parallelism by processing multiple rows/files at the same time.
(3) Customizable memory hierarchy of BRAM/URAM/DDR, providing high bandwidth of memory access to help operators.

How Vitis Data Analytics Library Works
======================================

The Vitis data analytics library targets to help data analysis developers to accelerate analysis execution. It provides three layers of APIs, namely L1/L2/L3. Each tackles different parts of the whole processing.

* L3 provides pure software APIs for:

(1) Define data structures to describe input/output/ operation types and parameters.
(2) Provide combination of operations for acceleration. These combinations are commonly used and easy to fit into the whole execution plan (decompress + parser + filter + format output).
(3) Schedule jobs, distributing sub jobs among all FPGA cards, pipeline data transfers, and kernel executions.

* L2 APIs are kernels running on FPGA cards. Each time called, they will finish certain processing according to input configs. L2 APIs are combinations of multiple processing units with each unit consisting of multiple processing stages. In this way, kernels could both processing multiple data at the same time and apply multiple operations to the same data. L2 API design subject to resource constraints and will differ according to FPGA cards.

* L1 APIs are basic operators in processing, like csv parser. They're all highly optimize HLS design, providing optimal performance. They're all template design, make them easier to scale and fit into different resource constraint.


L3 API -- CSV Scanner Engine
============================

Target Audience and Major Features
----------------------------------

Target audience of L3 API are users who just want to link a shared library and call the API to accelerate part of an execution plan on FPGA cards.

The major feature of L3 API are:

(1) Generalized execution. L3 API pre-defined operator combinations like gunzip_csv; its flow is "decompress + csv parser + filter + format output". More generalized execution will be provided in the future.
(2) Automatic card management. As soon as program created a instance, it will scan the machine and find all qualified AMD FPGA cards by their shell name. It will load the cards with the xclbins, management buffers, and schedule tasks.
(3) Parallel Accelerate. Reasonably schedule Send Thread, Kernel Run, Receive Thread, and improve end-to-end performance.

.. image:: /images/csv_scanner_card.png
   :alt: CSV Scanner Block Diagram
   :scale: 50%
   :align: center

Command to Run L3 Cases
-------------------------

.. code-block:: shell

    cd L3/tests/vitis_case_folder
    
    # build and run one of the following using u200 platform
    make run TARGET=hw_emu PLATFORM=/path/to/xilinx_u200_gen3x16_xdma_2_202110_1.xpfm
    
    # delete generated files
    make cleanall

Here, ``TARGET`` decides the FPGA binary type:

* ``hw_emu`` is for hardware emulation.
* ``hw`` is for deployment on the physical card (compilation to hardware binary often takes hours).

Besides ``run``, the Vitis case makefile also allows ``host`` and ``xclbin`` as a build target.

L2 API -- CSV Scanner Kernels
=============================

Target Audience and Major Features
----------------------------------

The target audience of L2 API are users who have a certain understanding of HLS and programming on FPGAs and want to make modifications on kernels, including:

(1) Operator combinations in kernel, like number of operators or operator pipeline. Most L1 APIs and glue logics inside L2 kernels are connect with streams. Users could add more operators into kernels to increase its performance as long as it is not bounded by other factors like logic resource and memory bandwidth. 
(2) Add more pre-processing and post-processing to kernel, like compression/decompression. Certain files might store its data in a compressed format to save the memory space and bandwidth to transfer. Adding a decompression module to build a longer processing pipeline will save the time to decompress the data and increase system performance.
(3) Test correctness in hw-emu, and on-board.
(4) Get accurate kernel resources and clock.
(5) Analyze kernel's timing performance and throughput.

Command to Run L2 Cases
-------------------------

.. code-block:: shell

    cd L2/tests/vitis_case_folder
    
    # build and run one of the following using u200 platform
    make run TARGET=hw_emu PLATFORM=/path/to/xilinx_u200_gen3x16_xdma_2_202110_1.xpfm
    
    # delete generated files
    make cleanall

Here, ``TARGET`` decides the FPGA binary type:

* ``hw_emu`` is for hardware emulation.
* ``hw`` is for deployment on the physical card (compilation to hardware binary often takes hours).

Besides ``run``, the Vitis case makefile also allows ``host`` and ``xclbin`` as build targets.


L1 API
=======

Target Audience and Major Features
------------------------------------

The target audience of L1 API are users who are familiar with HLS programming and want to tests/profile/modify operators or add a new operator. With the HLS test project provided in the L1 layer, you could get:

(1) Function correctness tests, both in c-simulation and co-simulation.
(2) Performance profiling from HLS synthesis report and co-simulation.
(3) Resource and timing from AMD Vivado™ synthesis.


Command to Run L1 Cases
-------------------------

.. code-block:: shell

    cd L1/tests/hls_case_folder
    
    make run CSIM=1 CSYNTH=0 COSIM=0 VIVADO_SYN=0 VIVADO_IMPL=0 \
        PLATFORM=/path/to/xilinx_u200_gen3x16_xdma_2_202110_1.xpfm

Test control variables are:

* ``CSIM`` for high level simulation.
* ``CSYNTH`` for high level synthesis to RTL.
* ``COSIM`` for co-simulation between software test bench and generated RTL.
* ``VIVADO_SYN`` for synthesis by Vivado.
* ``VIVADO_IMPL`` for implementation by Vivado.

For all these variables, setting to ``1`` indicates execution while ``0`` for skipping. The default value of all these control variables are ``0``, so they can be omitted from command line if the corresponding step is not wanted.
