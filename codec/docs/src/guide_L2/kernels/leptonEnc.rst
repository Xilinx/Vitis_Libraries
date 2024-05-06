.. 
Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
SPDX-License-Identifier: X11

.. _l2_kernel_lepton_encoder:

===========
Lepton Encoder
===========

Lepton Encoder example resides in ``L2/demos/leptonEnc`` directory. The tutorial provides a step-by-step guide that covers commands for building and running kernel.

Internal Designs
================

* **Overall Designs**

The design of the Lepton Encoder is as follows:

.. image:: /images/leptonEncoder.png
   :alt: Block Design of Lepton Encoder
   :align: center

The Lepton Encoder is composed of the following components:

AXI-to-Stream is responsible to load jpeg image from external memory into the FPGA.

Jpeg decode is responsible to decode the the jpeg data format into DCT coefficients.

IDCT is responsible to convert the DCT coefficients into pixels.

DC Predict is responsible to make prediction of DC coefficients. Pixels will be used in the dc prediction.

Line buffer implement a line buffer for the prediction of AC coefficients.

The Serialize and predict modules is responsible to do the prediction of AC coefficients.

The AC and DC coefficients and their predictions will be collected and then used to build up the probability tables.

Then Arithmetic Encode is used to generate the compressed bitstream.

The bitstream will be written to the external memory and moved back to the host to generate lepton file.

Software and system requirements
================================

The following packages are required to run this application:
* Xilinx Vitis 2022.1
* GCC 8.x
* make
* PLATFORM: xilinx_u200_gen3x16_xdma_2_202110_1

Building the accelerated Lepton encoder
=====================================

* In a terminal window, execute the following commands to set-up the Vitis environment

.. code-block:: bash

    cd L2/demos/leptonEnc
    source $XILINX_VITIS/settings64.sh 

* Build the accelerated Lepton encoder and run software emulation with the following command:

.. code-block:: bash

    make run TARGET=sw_emu

* Build the accelerated Lepton encoder and run hardware emulation with the following command:

.. code-block:: bash

    make run TARGET=hw_emu

* Build the accelerated Lepton encoder for on board execution with the following command:

.. code-block:: bash

    make run TARGET=hw

* lepEnc.xclbin and host.exe will generated in directory build_dir.hw.xilinx_u200_gen3x16_xdma_2_202110_1/


Running the accelerated Lepton encoder
===============

To get the benchmark results, please run the following command.

.. code-block:: bash
   
   source /opt/xilinx/xrt/setup.sh
   ./build_dir.hw.xilinx_u200_gen3x16_xdma_2_202110_1/host.exe --xclbin build_dir.hw.xilinx_u200_gen3x16_xdma_2_202110_1/lepEnc.xclbin images

Input Arguments:

.. code-block:: bash

   Usage: host.exe [--xclbin] [JPGDATAPATH]
          --xclbin:         the kernel name
          JPGDATAPATH:      a list of jpeg file to be encoded

Note: Default arguments are set in Makefile.

Performance
=========

The hardware resource utilizations are listed in the following table.
Different tool versions may result slightly different resource.


.. table:: Table 1 Acceleration performance on FPGA
    :align: center

    +-----------------+-------------+--------------+--------+----------+---------------+------------+-------------+-----------------------------+
    |    Pictures     | Width (pix) | Height (pix) | Format | Size(MB) | Comprs. Ratio | Freq (MHz) | Latency(ms) | Throughput FPGA U200 (MB/s) |
    +-----------------+-------------+--------------+--------+----------+---------------+------------+-------------+-----------------------------+
    | android.jpg     |    960      |     1280     |  420   |   0.13   |      1.33     |    202     |     6.63    |            159.74           |
    +-----------------+-------------+--------------+--------+----------+---------------+------------+-------------+-----------------------------+
    | iphone.jpg      |    3264     |     2448     |  420   |   2.1    |      1.32     |    202     |    94.69    |            172.54           |
    +-----------------+-------------+--------------+--------+----------+---------------+------------+-------------+-----------------------------+
    | offset.jpg      |    5184     |     3456     |  422   |   7.4    |      1.30     |    202     |   332.35    |            156.45           |
    +-----------------+-------------+--------------+--------+----------+---------------+------------+-------------+-----------------------------+
    | hq.jpg          |    5760     |     3840     |  444   |  22.60   |      1.22     |    202     |  1056.62    |            127.17           |
    +-----------------+-------------+--------------+--------+----------+---------------+------------+-------------+-----------------------------+

.. table:: Table 2  Resource using for FPGA
    :align: center
    
    +---------------+--------------+------------+
    | Utilizations  |    Lepton    | Percentage |
    +---------------+--------------+------------+
    |     LUT       |     80699    |    8.11%   |
    +---------------+--------------+------------+
    |     FF        |     72706    |    3.46%   |
    +---------------+--------------+------------+
    |     DSP       |      64      |    0.94%   |
    +---------------+--------------+------------+
    |     BRAM      |      58      |    3.08%   |
    +---------------+--------------+------------+
    |     URAM      |      86      |   29.15%   |
    +---------------+--------------+------------+


.. Copyright © 2020–2023 Advanced Micro Devices, Inc
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.