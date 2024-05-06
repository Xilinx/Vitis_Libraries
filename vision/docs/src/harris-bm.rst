   .. Copyright © 2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.


.. _l2_harris:

========================
Harris Corner Detection
========================

The Harris example resides in the ``L2/examples/harris`` directory.

This benchmark tests the performance of the `harris` function. The harris function detects corners in the image using harris corner detection and non-maximum suppression algorithms.

The tutorial provides a step-by-step guide that covers commands for building and running a kernel.

Executable Usage
================

* **Work Directory (Step 1)**

The steps for library download and environment setup can be found in the EADME file of the L2 folder. For getting the design:

.. code-block:: bash

   cd L2/example/harris

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
   
   -----------Harris Design---------------
   INFO: Running OpenCL section.
   Found Platform
   Platform Name: Xilinx
   XCLBIN File Name: krnl_harris
   INFO: Importing Vitis_Libraries/vision/L2/examples/harris/Xilinx_Harris_L2_Test_vitis_hw_u200/build_dir.hw.xilinx_u200_xdma_201830_2/krnl_harris.xclbin
   Loading: 'Vitis_Libraries/vision/L2/examples/harris/Xilinx_Harris_L2_Test_vitis_hw_u200/build_dir.hw.xilinx_u200_xdma_201830_2/krnl_harris.xclbin'
   Kernel Created
   Kernel Args set
   Kernel called
 
   Data copied from device to host
   Execution done!
   ocv corner count = 428, Hls corner count = 446
   Commmon = 405	 Success = 90.807175	 Loss = 5.373832	 Gain = 9.192825
   Test Passed 

   ------------------------------------------------------------

Profiling 
=========

The harris design is validated on an Alveo u200 board at 300 MHz frequency. 
Hardware resource utilization is listed in the following table.

.. table:: Table 1: Hardware Resources for Harris Corner Detection
    :align: center

    +------------------------------------------+-----------------+------------+------------+------------+
    |            Dataset                       |      LUT        |    BRAM    |     FF     |    DSP     |
    +------------+------+----------------------+                 |            |            |            |
    | Resolution | NPPC | other params         |                 |            |            |            |
    +============+======+======================+=================+============+============+============+
    |     4K     |   8  | L1 Norm,             |     40460       |    227     |    27236   |     208    |
    |            |      | Filter - 3x3         |                 |            |            |            |
    +------------+------+----------------------+-----------------+------------+------------+------------+
    |     FHD    |   8  | L1 Norm,             |     22478       |    113     |    16021   |     138    |
    |            |      | Filter - 3x3         |                 |            |            |            |
    +------------+------+----------------------+-----------------+------------+------------+------------+


The performance is shown in the following table.

.. table:: Table 2: Performance Numbers in terms of FPS (Frames Per Second) for Harris Corner Detection
    :align: center
	
    +----------------------+--------------+--------------+
    |       Dataset        |  FPS (CPU)   |  FPS (FPGA)  |
    +======================+==============+==============+
    |     4k (3840x2160)   |     22       |     289      |
    +----------------------+--------------+--------------+
    |  Full HD (1920x1080) |     62       |     1100     |
    +----------------------+--------------+--------------+


.. toctree::
    :maxdepth: 1
