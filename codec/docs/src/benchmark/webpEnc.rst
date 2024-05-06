.. 
Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
SPDX-License-Identifier: X11

.. _l2_webp:

============
Webp Encoder
============

Webp encoder demo resides in ``L2/demo/webpEnc`` directory. The tutorial provides a step-by-step guide that covers commands for building and running kernel.

Executable Usage
================

* **Work Directory(Step 1)**

The steps for library download and environment setup can be found in :ref:`l2_vitis_codec`. For getting the design,

.. code-block:: bash

   cd L2/demo/webpEnc

* **Build kernel(Step 2)**

Run the following make command to build your XCLBIN and host binary targeting a specific device. Note that this process takes a long time, maybe couple of hours.

.. code-block:: bash

   make run TARGET=hw PLATFORM=xilinx_u200_xdma_201830_2

* **Run kernel(Step 3)**

To get the benchmark results, please run the following command.

.. code-block:: bash

   ./build_dir.hw.xilinx_u200_xdma_201830_2/cwebp list.rst -use_ocl -q 80 -o output

Webp Input Arguments:

.. code-block:: bash

   Usage: cwebp -[-use_ocl -q -o]
          list.rst:     the input list
          -use_ocl:     should be kept
          -q:           compression quality
          -o:           output directory

Note: Default arguments are set in Makefile, you can use other :ref:`pictures` listed in the table.

* **Example output(Step 4)** 

.. code-block:: bash

   INFO: CreateKernel start.
   INFO: Number of Platforms: 1
   INFO: Selected Platform: Xilinx
   INFO: Number of devices for platform 0: 1
   INFO: target_device found:   xilinx_u200_xdma_201830_2
   INFO: target_device chosen:  xilinx_u200_xdma_201830_2
   NFO: OpenCL Version: 1.-48
   INFO: Loading kernel.xclbin
   INFO: Loading kernel.xclbin Finished

   ...

   *** Picture: 1 - 1,  Buffer: 0, Instance: 0, Event: 0 ***
   INFO: Host2Device finished. Computation time is 0.480000 (ms)
   INFO: PredKernel Finished. Computation time is 0.042000 (ms)
   INFO: ACKernel Finished. Computation time is 0.012000 (ms)
   INFO: Device2Host finished. Computation time is 0.005000 (ms)
   INFO: Loop of Pictures Finished. Computation time is 16.500000 (ms)
   INFO: VP8EncTokenLoopAsync Finished. Computation time is 22.676000 (ms)
   INFO: WebPEncodeAsync Finished. Computation time is 47.519000 (ms)
   INFO: Release Kernel.

Profiling
=========

The hardware resource utilizations are listed in the following table.
Different tool versions may result slightly different resource.


.. table:: Table 1 Hardware resources for webp kernels
    :align: center

    +-----------+----------+----------+----------+----------+---------+-----------------+
    |   Kernel  |   BRAM   |   URAM   |    DSP   |    FF    |   LUT   | Frequency(MHz)  |
    +-----------+----------+----------+----------+----------+---------+-----------------+
    |  kernel1  |    72    |    10    |   410    |   56498  |  48301  |       250       |
    +-----------+----------+----------+----------+----------+---------+-----------------+
    |  kernel2  |    11    |    0     |    5     |   23073  |  16375  |       250       |
    +-----------+----------+----------+----------+----------+---------+-----------------+


* One instance achieves about 6~14 times acceleration. Here are some examples:


.. table:: Table 2 Performance of Webp Encoder for FPGA 
    :align: center

    +--------------+-------------+--------------+----+----------------+--------------------------+--------------------------+------------+
    |    Kernel    | Width (pix) | Height (pix) | -q |  latency (ms)  | Throughput FPGA B (Mb/s) | Throughput FPGA P (Mp/s) | FPs (fps)  |
    +--------------+-------------+--------------+----+----------------+--------------------------+--------------------------+------------+
    |    Kernel1   |    1920     |     1080     | 80 |     21.18      |          146.83          |          97.88           |   47.20    |
    +--------------+-------------+--------------+----+----------------+--------------------------+--------------------------+------------+
    |    Kernel2   |    1920     |     1080     | 80 |     14.57      |          213.54          |         142.36           |   68.65    |
    +--------------+-------------+--------------+----+----------------+--------------------------+--------------------------+------------+
    |    Kernel1   |    512      |     512      | 80 |     3.22       |          122.03          |          81.35           |   310.33   |
    +--------------+-------------+--------------+----+----------------+--------------------------+--------------------------+------------+
    |    Kernel2   |    512      |     512      | 80 |     2.92       |          134.65          |          89.77           |   342.43   |
    +--------------+-------------+--------------+----+----------------+--------------------------+--------------------------+------------+
    |    Kernel1   |    1920     |     1080     | 90 |     21.03      |          147.87          |          98.58           |   47.54    |
    +--------------+-------------+--------------+----+----------------+--------------------------+--------------------------+------------+
    |    Kernel2   |    1920     |     1080     | 90 |     15.92      |          195.43          |          130.29          |   62.83    |
    +--------------+-------------+--------------+----+----------------+--------------------------+--------------------------+------------+
    |    Kernel1   |    512      |     512      | 90 |     4.73       |          83.12           |          55.41           |   211.39   |
    +--------------+-------------+--------------+----+----------------+--------------------------+--------------------------+------------+
    |    Kernel2   |    512      |     512      | 90 |     4.93       |          79.73           |          53.16           |   202.78   |
    +--------------+-------------+--------------+----+----------------+--------------------------+--------------------------+------------+


* Platform: FPGA U200, CPU details are listd belowd (single thread)

.. note::
    | 1. Kernels running on platform with Intel(R) Xeon(R) CPU E5-2603 v3 @ 1.60GHz, 48 Threads.
    | 2. time unit: ms.
    | 3. "-" Indicates that the result could not be obtained due to insufficient memory.
    | 4. FPGA time is the kernel runtime by adding data transfer and executed with webp encoder. 

.. toctree::
   :maxdepth: 1

.. Copyright © 2020–2023 Advanced Micro Devices, Inc
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.