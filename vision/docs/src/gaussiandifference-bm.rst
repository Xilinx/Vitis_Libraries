.. Copyright © 2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.


.. _l3_gaussiandifference:


Gaussian Difference
####################

The Gaussian Difference example resides in the ``L3/examples/gaussiandifference`` directory.

This benchmark tests the performance of the `gaussiandifference` function. The Difference of Gaussian Filter function can be implemented by applying the Gaussian Filter on the original source image, and that Gaussian blurred image is duplicated as two images. The Gaussian blur function is applied to one of the duplicated images, whereas the other one is stored as it is. Later, perform the Subtraction function on, two times Gaussian applied image and one of the duplicated image.

The tutorial provides a step-by-step guide that covers commands for building and running a kernel.

Executable Usage
================

* **Work Directory (Step 1)**

The steps for library download and environment setup can be found in README file of L3 folder. For getting the design,

.. code-block:: bash

   cd L3/example/gaussiandifference

* **Build Kernel (Step 2)**

Run the following make command to build your XCLBIN and host binary targeting a specific device. Be aware that this process can take up to a couple of hours.

.. code-block:: bash

   export OPENCV_INCLUDE=< path-to-opencv-include-folder >
   export OPENCV_LIB=< path-to-opencv-lib-folder >
   export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:< path-to-opencv-lib-folder > 
   export PLATFORM=< path-to-platform-directory >/< platform >.xpfm
   make host xclbin TARGET=hw


* **Run kernel(Step 3)**

To get the benchmark results, run the following command.

.. code-block:: bash

   make run TARGET=hw 


* **Example Output (Step 4)** 

.. code-block:: bash
   
   -----------Guassian Difference Design---------------
   INFO: Running OpenCL section.
   Found Platform
   Platform Name: Xilinx
   INFO: Device found - xilinx_u200_xdma_201830_2
   XCLBIN File Name: krnl_gaussiandifference
   INFO: Importing Vitis_Libraries/vision/L3/examples/gaussiandifference/Xilinx_Gaussiandifference_L3_Test_vitis_hw_u200/build_dir.hw.xilinx_u200_xdma_201830_2/krnl_gaussiandifference.xclbin
   Loading: 'Vitis_Libraries/vision/L3/examples/gaussiandifference/Xilinx_Gaussiandifference_L3_Test_vitis_hw_u200/build_dir.hw.xilinx_u200_xdma_201830_2/krnl_gaussiandifference.xclbin'
   Test Passed 
   ------------------------------------------------------------

Profiling 
=========

The Gaussian Difference design is validated on an Alveo U200 board at 300 MHz frequency. 
Hardware resource utilization is listed in the following table.

.. table:: Table 1: Hardware Resources for Gaussian Difference
    :align: center

    +------------------------------------------+-----------------+------------+------------+------------+
    |            Dataset                       |      LUT        |    BRAM    |     FF     |    DSP     |
    +------------+------+----------------------+                 |            |            |            |
    | Resolution | NPPC | other params         |                 |            |            |            |
    +============+======+======================+=================+============+============+============+
    |     4K     |   1  |      Filter - 3x3    |     27235       |     35     |    37426   |     281    |
    +------------+------+----------------------+-----------------+------------+------------+------------+
    |     FHD    |   1  |      Filter - 3x3    |     15146       |     19     |    22032   |     193    |
    +------------+------+----------------------+-----------------+------------+------------+------------+


The performance is shown in the following table.

.. table:: Table 2: Performance nNumbers in terms of FPS (Frames Per Second) for Gaussian Difference
    :align: center
	
    +----------------------+--------------+--------------+
    |       Dataset        |   FPS(CPU)   |   FPS(FPGA)  |
    +======================+==============+==============+
    |     4k (3840x2160)   |     126      |     289      |
    +----------------------+--------------+--------------+
    |  Full HD (1920x1080) |     500      |     1100     |
    +----------------------+--------------+--------------+


.. toctree::
    :maxdepth: 1
