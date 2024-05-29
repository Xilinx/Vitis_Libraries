.. 
   Copyright 2024 Advanced Micro Devices, Inc
  
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.


.. _l2_fast:


FAST Corner Detection
######################

The FAST Corner Detection example resides in the ``L2/examples/fast`` directory.

This benchmark tests the performance of the `fast` function. Features from accelerated segment test (FAST) is a corner detection algorithm, that is faster than most of the other feature detectors.

The tutorial provides a step-by-step guide that covers commands for building and running a kernel.

Executable Usage
=================

* **Work Directory (Step 1)**

The steps for library download and environment setup can be found in the README of the L2 folder. For getting the design,

.. code-block:: bash

   cd L2/examples/fast

* **Build Kernel (Step 2)**

Run the following make command to build your XCLBIN and host binary targeting a specific device. Be aware that this process can take up to a couple of hours.

.. code-block:: bash

   export OPENCV_INCLUDE=< path-to-opencv-include-folder >
   export OPENCV_LIB=< path-to-opencv-lib-folder >
   export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:< path-to-opencv-lib-folder >
   export PLATFORM=< path-to-platform-directory >/< platform >.xpfm
   make host xclbin TARGET=hw

* **Run Kernel (Step 3)**

To get the benchmark results, run the following command.

.. code-block:: bash

   make run TARGET=hw

* **Example output(Step 4)** 

.. code-block:: bash
   
   -----------FAST Design---------------
   Found Platform
   Platform Name: Xilinx
   INFO: Device found - xilinx_u200_xdma_201830_2
   XCLBIN File Name: krnl_fast
   INFO: Importing vision/L2/examples/fast/Xilinx_Fast_L2_Test_vitis_hw_u200/build_dir.hw.xilinx_u200_xdma_201830_2/krnl_fast.xclbin
   Loading: 'vision/L2/examples/fast/Xilinx_Fast_L2_Test_vitis_hw_u200/build_dir.hw.xilinx_u200_xdma_201830_2/krnl_fast.xclbin'
   ocvpoints:511=
   INFO: Verification results:
       Common = 511
       Success = 100
       Loss = 0
       Gain = 0
   Test Passed
   ------------------------------------------------------------

Profiling 
=========

The fast corner detection design is validated on an Alveo U200 board at 300 MHz frequency. 
Hardware resource utilization is listed in the following table.

.. table:: Table 1: Hardware Resources for FAST Corner Detection
    :align: center

    +------------------------------------------+-----------------+------------+------------+------------+
    |            Dataset                       |      LUT        |    BRAM    |     FF     |    DSP     |
    +------------+------+----------------------+                 |            |            |            |
    | Resolution | NPPC | other params         |                 |            |            |            |
    +============+======+======================+=================+============+============+============+
    |     4K     |   8  |      NA              |     21171       |     10     |    13396   |     0      |
    +------------+------+----------------------+-----------------+------------+------------+------------+
    |     FHD    |   8  |      NA              |     20437       |     10     |    14322   |     0      |
    +------------+------+----------------------+-----------------+------------+------------+------------+


The performance is shown in the following table.

.. table:: Table 2: Performance Numbers in terms of FPS (Frames Per Second) for FAST Corner Detection
    :align: center
	
    +----------------------+--------------+--------------+
    |       Dataset        |   FPS(CPU)   |   FPS(FPGA)  |
    +======================+==============+==============+
    |     4k (3840x2160)   |     79       |     289      |
    +----------------------+--------------+--------------+
    |   Full HD(1920x1080) |     186      |     1100     |
    +----------------------+--------------+--------------+



.. toctree::
    :maxdepth: 1
