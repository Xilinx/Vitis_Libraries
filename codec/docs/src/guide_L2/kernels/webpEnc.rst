.. 
Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
SPDX-License-Identifier: X11

.. _l2_kernel_webp_encoder:

=============
 WebP Encoder
=============

WebP is a new image format developed by Google and supported in Chrome, Opera and Android that is optimized to enable faster and smaller images on the Web. WebP images are about 30% smaller in size compared to PNG and JPEG images at equivalent visual quality. In addition, the WebP image format has feature parity with other formats as well.

Implementation
==============

This accelerated WebP encoder project is based on `libwebp` open source project. For one input picutre (.png), the output picutre (.webp) is achieved after following six steps:

.. image:: /images/webp_steps.png
   :alt: Webp Encoder Structure
   :scale: 60%
   :align: center

Time-consuming functions are accelerated by 2 FPGA kernels including:

.. code-block:: bash

  Kernel-1: intra-prediction and probability counting
  Kernel-2: arithmetic coding

Performance
===========

* One instance achieves about 6~14 times acceleration. Here are some examples:
  
.. table:: Table 1  Acceleration process on CPU comparison with FPGA
    :align: center

    +-----------------+--------------------+-------------+--------------+----+---------------------+----------------------+------------+-----------------------------+-----------------------+----------+
    |    Pictures     | Texture complexity | Width (pix) | Height (pix) | -q | kernel1 latency(ms) | kernel2 latency (ms) | Freq (MHz) | Throughput FPGA U200 (MB/s) | Throughput CPU (MB/s) | Speed up |
    +-----------------+--------------------+-------------+--------------+----+---------------------+----------------------+------------+-----------------------------+-----------------------+----------+
    | 3840-city.png   |       complex      |    3840     |     2160     | 80 |        95.30        |         87.93        |    250     |            129.82           |         18.46         |   7.03   |
    +-----------------+--------------------+-------------+--------------+----+---------------------+----------------------+------------+-----------------------------+-----------------------+----------+
    | 1920x1080x4.png |       simple       |    3840     |     2160     | 80 |        83.90        |         74.96        |    250     |            159.74           |         16.17         |   9.88   |
    +-----------------+--------------------+-------------+--------------+----+---------------------+----------------------+------------+-----------------------------+-----------------------+----------+
    | 1920x1080.png   |       simple       |    1920     |     1080     | 80 |        21.51        |         18.60        |    250     |            172.54           |         11.85         |   14.56  |
    +-----------------+--------------------+-------------+--------------+----+---------------------+----------------------+------------+-----------------------------+-----------------------+----------+
    | 853x640.png     |       simple       |    853      |     640      | 80 |         4.13        |         74.96        |    250     |            156.45           |         20.97         |   7.46   |
    +-----------------+--------------------+-------------+--------------+----+---------------------+----------------------+------------+-----------------------------+-----------------------+----------+
    | lena_c_512.png  |       middle       |    512      |     512      | 80 |         2.90        |         2.84         |    250     |            127.17           |         21.32         |   5.96   |
    +-----------------+--------------------+-------------+--------------+----+---------------------+----------------------+------------+-----------------------------+-----------------------+----------+
  
Platform: CPU: Intel(R) Xeon(R) Gold 6142 CPU @ 2.60GHz (single thread)

* One instance takes about 6% resource of U200 acceleraction card, following is the detail:

.. table:: Table 2  Resource using for FPGA
    :align: center
    
    +---------------+--------------+--------------+---------------------+
    | Utilizations  |   Kernel-1   |   Kernel-2   | Kernel-1 + Kernel-2 |
    +---------------+--------------+--------------+---------------------+
    |     LUT       |    52889     |    15866     |        5.37%        |
    +---------------+--------------+--------------+---------------------+
    |     FF        |    68991     |    23039     |        3.30%        |
    +---------------+--------------+--------------+---------------------+
    |     DSP       |     410      |      4       |        6.00%        |
    +---------------+--------------+--------------+---------------------+
    |     BRAM      |      72      |     157      |        4.00%        |
    +---------------+--------------+--------------+---------------------+
    |     URAM      |      10      |      0       |        2.08%        |
    +---------------+--------------+--------------+---------------------+

* Multi-pictures process. Host code supports multi-pictures process with asynchronous behaviors, which allows to overlap host-device communiations, prediction kernel computation and arithmetic coding kernel computation. This is shown by following demonstration picture and profiling result.

.. image:: /images/webp_overlap.png
   :alt: Webp Encoder OverLap
   :scale: 60%
   :align: center
 
.. image:: /images/webp_profiling.png
   :alt: Webp Encoder Profiling
   :scale: 60%
   :align: center

Software and system requirements
================================

The following packages are required to run this application:
* Xilinx Vitis 2022.1
* GCC 8.x
* make
* PLATFORM: xilinx_u200_gen3x16_xdma_2_202110_1


Building the accelerated WebP encoder
=====================================

* In a terminal window, execute the following commands to set-up the Vitis environment

.. code-block:: bash

    cd L2/demos/webpEnc
    source $XILINX_VITIS/settings64.sh 

* Build the accelerated WebP encoder and run software emulation with the following command:

.. code-block:: bash

    make run TARGET=sw_emu

* Build the accelerated WebP encoder and run hardware emulation with the following command:

.. code-block:: bash

    make run TARGET=hw_emu

* Build the accelerated WebP encoder for on board execution with the following command:

.. code-block:: bash

    make run TARGET=hw

* kernel.xclbin and cwebp will generated in directory build_dir.hw.xilinx_u200_gen3x16_xdma_2_202110_1/


Running the accelerated WebP encoder
====================================

* The `cwebp` application takes the following arguments:

.. code-block:: bash

    list.rst is text file lists input pictures, should be equal to "NPicPool" defined in src_syn/vp8_AsyncConfig.h
    -use_ocl: should be kept
    -q: compression quality
    -o: output directory

* Run the accelerated WebP encoder with the following commands:

.. code-block:: bash

    source /opt/xilinx/xrt/setup.sh
    ./cwebp -xclbin kernel.xclbin list.rst -use_ocl -q 80 -o ./images/

.. Copyright © 2020–2023 Advanced Micro Devices, Inc
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.