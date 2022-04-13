====================
SMEM Application
====================

This section presents brief introduction about SMEM application and step by step
guidelines to build and deployment.

Overview
--------

SMEM is an Open Source Genomics library* which provides
high throughput with low latency. The Xilinx SMEM application
 is developed and tested on Xilinx Alveo U250 board.

.. code-block:: bash

   Tested Tool: 2021.2
   Tested XRT:  2021.2
   Tested XSA:  xilinx_u250_gen3x16_xdma_3_1_202020_1

Executable Usage
----------------

This application is present under ``L3/demos/smem/`` directory. Follow build instructions to generate executable and binary.

The host executable generated is named as "**xil_smem**" and it is generated in ``./build`` directory.

Following is the usage of the executable:

To execute file	: ``./build/xil_smem  ./build/xclbin_<xsa_name>_<TARGET mode>/smem.xclbin  <file_name>``

===========================================================


