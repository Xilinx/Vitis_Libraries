.. 
   Copyright 2024 Advanced Micro Devices, Inc
  
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.


.. _l3_isppipeline:

=======================================
Image Sensor Processing (ISP) Pipeline
=======================================

The ISP Pipeline example resides in the ``L3/examples/isppipeline`` directory.

This benchmark tests the performance of the `isppipeline` function. ISP is a pipeline of functions that enhance the overall visual quality of the raw image from the sensor.

The tutorial provides a step-by-step guide that covers commands for building and running a kernel.

Executable Usage
================

* **Work Directory (Step 1)**

The steps for library download and environment setup can be found in the README of the L3 folder. To get the design:

.. code-block:: bash

   cd L3/examples/isppipeline

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

* **Example Output (Step 4)** 

.. code-block:: bash
   
   -----------ISP Pipeline Design--------------------------------------------------------------------------------
   Found Platform
   Platform Name: Xilinx
   XCLBIN File Name: krnl_ISPPipeline
   INFO: Importing vision/L3/examples/isppipeline/build_dir.hw.xilinx_u200_xdma_201830_2/krnl_ISPPipeline.xclbin
   Loading: 'vision/L3/examples/isppipeline/build_dir.hw.xilinx_u200_xdma_201830_2/krnl_ISPPipeline.xclbin'
   The maximum depth reached by any of the 8 hls::stream() instances in the design is 196608
   ---------------------------------------------------------------------------------------------------------------

Profiling 
=========

The ISP Pipeline design is validated on an Alveo U200 board at 300 MHz frequency. 
Hardware resource utilization is shown in the following table.

.. table:: Table 1: Hardware Resources for the ISP Pipeline
    :align: center

    +------------------------------------------+-----------------+------------+------------+------------+
    |            Dataset                       |      LUT        |    BRAM    |     FF     |    DSP     |
    +------------+------+----------------------+                 |            |            |            |
    | Resolution | NPPC | other params         |                 |            |            |            |
    +============+======+======================+=================+============+============+============+
    |     4K     |   1  |      Filter - 3x3    |     18987       |     24     |    17713   |     91     |
    +------------+------+----------------------+-----------------+------------+------------+------------+
    |     FHD    |   1  |      Filter - 3x3    |     19254       |     19     |    17534   |     88     |
    +------------+------+----------------------+-----------------+------------+------------+------------+


The performance is shown in the following table.

.. table:: Table 2: Performance Numbers in Frames Per Second (FPS) for the ISP Pipeline
    :align: center


    +----------------------+--------------+--------------+
    |       Dataset        |   FPS(CPU)   |   FPS(FPGA)  |
    +======================+==============+==============+
    |     4k (3840x2160)   |     0.11     |     135      |
    +----------------------+--------------+--------------+
    |  Full HD (1920x1080) |     0.44     |     520      |
    +----------------------+--------------+--------------+

.. toctree::
    :maxdepth: 1
