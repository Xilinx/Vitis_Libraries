   .. Copyright © 2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.


.. _l2_houghlines:


Houghlines
###########

The Houghlines example resides in the ``L2/examples/houghlines`` directory.

This benchmark tests the performance of the `houghlines` function. The HoughLines function here is equivalent to the HoughLines Standard in OpenCV. The HoughLines function is used to detect straight lines in a binary image.

The tutorial provides a step-by-step guide that covers commands for building and running a kernel.

Executable Usage
=================

* **Work Directory (Step 1)**

The steps for library download and environment setup can be found in the README file of the L2 folder. For getting the design:

.. code-block:: bash

   cd L2/example/houglines

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
   
   -----------HoughLines Design---------------
   INFO: Running OpenCL section.
   Found Platform
   Platform Name: Xilinx
   INFO: Device found - xilinx_u200_xdma_201830_2
   XCLBIN File Name: krnl_houghlines
   INFO: Importing Vitis_Libraries/vision/L2/examples/houghlines/Xilinx_Houghlines_L2_Test_vitis_hw_u200/build_dir.hw.xilinx_u200_xdma_201830_2/krnl_houghlines.xclbin
   Loading: '/Vitis_Libraries/vision/L2/examples/houghlines/Xilinx_Houghlines_L2_Test_vitis_hw_u200/build_dir.hw.xilinx_u200_xdma_201830_2/krnl_houghlines.xclbin'
   INFO: Verification results:
	Success = 98%
	Number of matched lines = 98
   ------------------------------------------------------------

Profiling 
=========

The Houghlines design is validated on an Alveo u200 board at 300 MHz frequency. 
Hardware resource utilization is listed in the following table.

.. table:: Table 1: Hardware Resources for Houghlines
    :align: center

    +------------------------------------------+-----------------+------------+------------+------------+
    |            Dataset                       |      LUT        |    BRAM    |     FF     |    DSP     |
    +------------+------+----------------------+                 |            |            |            |
    | Resolution | NPPC | other params         |                 |            |            |            |
    +============+======+======================+=================+============+============+============+
    |     4K     |   1  | THETA=1,RHO=1        |     98421       |   1056     |    96523   |     11     |
    +------------+------+----------------------+-----------------+------------+------------+------------+
    |     FHD    |   1  | THETA=1,RHO=1        |     55426       |     538    |    58246   |     8      |
    +------------+------+----------------------+-----------------+------------+------------+------------+


The performance is shown below

.. table:: Table 2: Performance Numbers in terms of FPS (Frames Per Second) for Houghlines
    :align: center
	
    +----------------------+--------------+--------------+
    |       Dataset        |   FPS(CPU)   |   FPS(FPGA)  |
    +======================+==============+==============+
    |     4k (3840x2160)   |     30       |     27       |
    +----------------------+--------------+--------------+
    |   Full HD(1920x1080) |     83       |     80       |
    +----------------------+--------------+--------------+


.. toctree::
    :maxdepth: 1
