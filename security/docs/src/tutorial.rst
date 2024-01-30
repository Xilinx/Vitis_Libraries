.. 
   .. Copyright ©2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

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

Crypto algorithms play an important role in transfer, verification, storage, and processing of data.
Crypto application includes encryption/decryption, checksum, hash, signature/verification, and so on.
Modern crypto algorithms are complicated and keep evolving, especially in new areas like block-chain application.
An FPGA is suitable for a wide variety of crypto algorithms and provides competitive or even better performance than CPU.
It is also convenient to construct a processing pipeline while crypto processing is one of the pipeline stages, which save bandwidth cost and improve performance.
Last but not the least, an FPGA can have multiple compute units working in parallel, which also improve the total performance.

.. image:: /images/pipeline_processing.png
   :alt: Execution Plan 
   :scale: 20%
   :align: center

How Vitis Security Library Works
==================================

AMD Vitis |trade| security library targets to help Vitis kernel developers to accelerate crypto algorithms.
It provides three layers of APIs, namely L1 / L2 / L3.
L1 APIs are the core for acceleration while L2 and L3 only contain a few specialized cases.

L1 API
=======

Target Audience and Major Features
------------------------------------

Target audience of L1 API are users who are familiar with HLS programming and want to test / profile / modify operators or add new APIs.
With the HLS test project provided in L1 layer, you get:

(1) Function correctness tests, both in c-simulation and co-simulation
(2) Performance profiling from HLS synthesis report and co-simulaiton
(3) Resource and timing from AMD Vivado |trade| synthesis.


Input / output interface
--------------------------

For easy connection with other HLS components, most of the security library L1 API takes hls::stream interface.
For APIs that could take variable length of input, an API includes either scalar parameter for input length or end flag stream working with data stream in a 1:1 fashion to tell if this is the last block of input data.
For details of interface definition and how padding happens in the stream interface, refer to the documentation and corresponding test cases.

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
The default value of all these control variables are ``0``, so they can be omitted from the command line
if the corresponding step is not required.


.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim: