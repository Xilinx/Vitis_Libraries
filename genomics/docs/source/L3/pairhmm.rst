====================
PairHMM Application
====================

This section presents brief introduction about PairHMM application and step by step
guidelines to build and deployment.

Overview
--------

PairHMM is an Open Source Genomics library* which provides
high throughput and low latency. The Xilinx PairHMM application
 is developed and tested on Xilinx Alveo U200 board.

.. code-block:: bash

   Tested Tool: 2021.2
   Tested XRT:  2021.2
   Tested XSA:  xilinx_u200_xdma_201830_2

Executable Usage
----------------

This application is present under ``L3/demos/pairhmm/`` directory. Follow build instructions to generate executable and binary.

The host executable generated is named as "**xil_pairhmm**" and it is generated in ``./build`` directory.

Following is the usage of the executable:

To execute Single PairHMM kernel :      ``./build/xil_pairhmm ./build/xclbin_<xsa_name>_<TARGET mode>/pairhmm.xclbin --syn <number of tests>``


===========================================================
   
