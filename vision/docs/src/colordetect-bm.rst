.. Copyright © 2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.


.. _l3_colordetect:


Color Detect
#############

The Color Detect example resides in ``L3/examples/colordetect`` directory.

This benchmark tests the performance of the `colordetect` function. The Color Detection algorithm is basically used for color object tracking and object detection, based on the color of the object. 

The tutorial provides a step-by-step guide that covers commands for building and running a kernel.

Executable Usage
================

* **Work Directory (Step 1)**

The steps for library download and environment setup can be found in the README file of the L3 folder. For getting the design:

.. code-block:: bash

   cd L3/example/colordetect

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
   
   -----------Color Detect Design---------------
   INFO: Running OpenCL section.
   Found Platform
   Platform Name: Xilinx
   INFO: Device found - xilinx_u200_xdma_201830_2
   XCLBIN File Name: krnl_colordetect
   INFO: Importing Vitis_Libraries/vision/L3/examples/colordetect/Xilinx_Colordetect_L3_Test_vitis_hw_u200/build_dir.hw.xilinx_u200_xdma_201830_2/krnl_colordetect.xclbin
   Loading: 'Vitis_Libraries/vision/L3/examples/colordetect/Xilinx_Colordetect_L3_Test_vitis_hw_u200/build_dir.hw.xilinx_u200_xdma_201830_2/krnl_colordetect.xclbin'
   INFO: Verification results:
	Percentage of pixels above error threshold = 0
   ------------------------------------------------------------

Profiling 
=========

The Color Detect design is validated on Alveo U200 board at 300 MHz frequency. 
Hardware resource utilization is shown in the following table.

.. table:: Table 1: Hardware resources for Color Detection
    :align: center

    +------------------------------------------+-----------------+------------+------------+------------+
    |            Dataset                       |      LUT        |    BRAM    |     FF     |    DSP     |
    +------------+------+----------------------+                 |            |            |            |
    | Resolution | NPPC | other params         |                 |            |            |            |
    +============+======+======================+=================+============+============+============+
    |     4K     |   1  |      Filter - 3x3    |     16523       |    176     |    9145    |      3     |
    +------------+------+----------------------+-----------------+------------+------------+------------+
    |     FHD    |   1  |      Filter - 3x3    |      9961       |     91     |    5432    |      1     |
    +------------+------+----------------------+-----------------+------------+------------+------------+


Performance is shown in the following table.

.. table:: Table 2: Performance Numbers in terms of FPS (Frames Per Second) for Color Detection
    :align: center
	
    +----------------------+--------------+--------------+
    |       Dataset        |   FPS(CPU)   |   FPS(FPGA)  |
    +======================+==============+==============+
    |     4k (3840x2160)   |     15       |     289      |
    +----------------------+--------------+--------------+
    |   Full HD(1920x1080) |     28       |     1100     |
    +----------------------+--------------+--------------+


.. toctree::
    :maxdepth: 1
