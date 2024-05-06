.. 
Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
SPDX-License-Identifier: X11

.. _l1_manual_jpeg_huffman_decoder:

============
JPEG Huffman Decoder
============

Jpeg Huffman Decoder example resides in ``L1/tests/jpegDec`` directory. The tutorial provides a step-by-step guide that covers commands for building and running IP.

Executable Usage
===============

* **Work Directory(Step 1)**

The steps for library download and environment setup can be found in :ref:`l1_vitis_codec`. For getting the design,

.. code-block:: bash

   cd L1/tests/jpegDec

* **Run and Build IP(Step 2)**

Run the following make command to build your IP targeting a specific device. Note that this process takes a long time, maybe couple of hours.

.. code-block:: bash

   make run PLATFORM=xilinx_u50_gen3x16_xdma_201920_3 VIVADO_IMPL=1

   Usage: host.exe -[-xclbin -JPEGFile]
          -JPEGFile:  the path point to input *.jpg

Note: Default arguments are set in Makefile, you can use other :ref:`pictures` listed in the table.

* **Example output(Step 2)** 

.. code-block:: bash

   ...
   Time resolution is 1 ps
   ...
   RTL Simulation : 0 / 1 [n/a] @ "106000"
   RTL Simulation : 1 / 1 [n/a] @ "345801000"
   ...
   INFO: [COSIM 212-316] Starting C post checking ...
   INFO: [COSIM 212-1000] *** C/RTL co-simulation finished: PASS ***
   ...
   Implementation tool: Xilinx Vivado v.2022.1
   Project:             test.prj
   Solution:            solution1
   ...
   #=== Post-Implementation Resource usage ===
   SLICE:            0
   LUT:           7939
   FF:            8043
   DSP:             12
   BRAM:             5
   URAM:             0
   LATCH:            0
   SRL:            717
   CLB:           1748

   #=== Final timing ===
   CP required:                     2.500
   CP achieved post-synthesis:      4.564
   CP achieved post-implementation: 3.703
   
Profiling
=========

The hardware resource utilizations are listed in the following table.
Different tool versions may result slightly different resource.


.. table:: Table 1 IP resources for jpeg Huffman Decoder with jfif parser and huffman decoder
    :align: center

    +-----------------------+----------+----------+----------+----------+---------+-----------------+
    |           IP          |   BRAM   |   URAM   |    DSP   |    FF    |   LUT   | Frequency(MHz)  |
    +-----------------------+----------+----------+----------+----------+---------+-----------------+
    | kernel_parser_decoder |     5    |     0    |    12    |    7939  |   8043  |       270       |
    +-----------------------+----------+----------+----------+----------+---------+-----------------+

Table 2 : Jpeg Huffman Decoder IP profiling

.. image:: /images/jpegDecoderpofile.png
   :alt: Jpeg huffman Decoder IP profiling
   :width: 70%
   :align: center

.. note::
    | 1. MAX_DEC_PIX is for benchmark. If testcase image is larger than 20M, the value of MAX_DEC_PIX should be enlarged following the size of image.   
    | 2. MAXCMP_BC is for benchmark. If testcase image is larger than 20M, the value of MAXCMP_BC should be enlarged following the size of image.   

.. toctree::
   :maxdepth: 1

.. Copyright © 2020–2023 Advanced Micro Devices, Inc
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.