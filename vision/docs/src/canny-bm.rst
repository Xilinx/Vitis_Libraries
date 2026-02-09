.. Copyright © 2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.


.. _l2_canny:


Canny Edge Detection
#####################

The Canny example resides in the ``L2/examples/canny`` directory.

This benchmark tests the performance of the `canny` function. The Canny edge detector finds the edges in an image or video frame. It is one of the most popular algorithms for edge detection. 

The tutorial provides a step-by-step guide that covers commands for building and running a kernel.

Executable Usage
================

* **Work Directory (Step 1)**

The steps for library download and environment setup can be found in the README file of the L2 folder. For getting the design:

.. code-block:: bash

   cd L2/example/canny

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
   
   -----------Canny Design---------------
   Found Platform
   Platform Name: Xilinx
   XCLBIN File Name: krnl_canny
   INFO: Importing Vitis_Libraries/vision/L2/examples/canny/Xilinx_Canny_L2_Test_vitis_hw_u200/build_dir.hw.xilinx_u200_xdma_201830_2/krnl_canny.xclbin
   Loading: 'Vitis_Libraries/vision/L2/examples/canny/Xilinx_Canny_L2_Test_vitis_hw_u200/build_dir.hw.xilinx_u200_xdma_201830_2/krnl_canny.xclbin'
   before kernelafter kernel
   before kernelafter kernel
   actual number of cols is 3840
   Total Execution time 10.5ms
   ------------------------------------------------------------

Profiling 
=========

The canny design is validated on Alveo U200 board at 300 MHz frequency. 
Hardware resource utilization is shown in the following table.

.. table:: Table 1: Hardware resources for Canny edge detection
    :align: center

    +------------------------------------------+-----------------+------------+------------+------------+
    |            Dataset                       |      LUT        |    BRAM    |     FF     |    DSP     |
    +------------+------+----------------------+                 |            |            |            |
    | Resolution | NPPC | other params         |                 |            |            |            |
    +============+======+======================+=================+============+============+============+
    |     4K     |   8  | L1 Norm,             |     31408       |    132     |    19148   |     96     |
    |            |      | Filter - 3x3         |                 |            |            |            |
    +------------+------+----------------------+-----------------+------------+------------+------------+
    |     FHD    |   8  | L1 Norm,             |     17451       |    65      |    11256   |     63     |
    |            |      | Filter - 3x3         |                 |            |            |            |
    +------------+------+----------------------+-----------------+------------+------------+------------+


The performance is shown below

.. table:: Table 2: Performance numbers in terms of FPS (Frames Per Second) for Canny edge detection
    :align: center
	
    +----------------------+--------------+--------------+
    |       Dataset        |   FPS(CPU)   |   FPS(FPGA)  |
    +======================+==============+==============+
    |     4k (3840x2160)   |     9        |     95       |
    +----------------------+--------------+--------------+
    |   Full HD(1920x1080) |     25       |     333      |
    +----------------------+--------------+--------------+


.. toctree::
    :maxdepth: 1
