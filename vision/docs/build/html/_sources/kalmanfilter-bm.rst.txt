.. xfOpenCVlib

   .. Copyright © 2024 Advanced Micro Devices, Inc


.. _l2_kalmanfilter:


Kalman Filter
##############

The Kalman Filter example resides in the ``L2/examples/kalmanfilter`` directory.

This benchmark tests the performance of the `kalmanfilter` function. The classic Kalman Filter is proposed for linear systems.

The tutorial provides a step-by-step guide that covers the commands for building and running a kernel.

Executable Usage
================

* **Work Directory (Step 1)**

The steps for library download and environment setup can be found in the README file of the L2 folder. For getting the design:

.. code-block:: bash

   cd L2/example/kalmanfilter

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
   
   -----------Kalman Design---------------
   INFO: Init cv::Mat objects.
   INFO: Kalman Filter Verification:
	Number of state variables: 16
	Number of measurements: 16
	Number of control input: 16
   INFO: Running OpenCL section.
   Found Platform
   Platform Name: Xilinx
   INFO: Device found - xilinx_u200_xdma_201830_2
   XCLBIN File Name: krnl_kalmanfilter
   INFO: Importing Vitis_Libraries/vision/L2/examples/kalmanfilter/Xilinx_Kalmanfilter_L2_Test_vitis_hw_u200/build_dir.hw.xilinx_u200_xdma_201830_2/krnl_kalmanfilter.xclbin
   Loading: 'Vitis_Libraries/vision/L2/examples/kalmanfilter/Xilinx_Kalmanfilter_L2_Test_vitis_hw_u200/build_dir.hw.xilinx_u200_xdma_201830_2/krnl_kalmanfilter.xclbin'
   INFO: Test Pass
   ------------------------------------------------------------

Profiling 
=========

The Kalman Filter design is validated on an Alveo u200 board at 300 MHz frequency. 
Hardware resource utilization is listed in the following table.

.. table:: Table 1: Hardware Resources for Kalman Filter
    :align: center

    +------------------------------------------+-----------------+------------+------------+------------+
    |            Dataset                       |      LUT        |    BRAM    |     FF     |    DSP     |
    +------------+------+----------------------+                 |            |            |            |
    | Resolution | NPPC |    other params      |                 |            |            |            |
    +============+======+======================+=================+============+============+============+
    |     NA     |   1  |   SV - 16x16x16      |     59342       |     98     |    90762   |     361    |
    +------------+------+----------------------+-----------------+------------+------------+------------+


The performance is shown in the following table.

.. table:: Table 2: Performance numbers for Kalman Filter
    :align: center
	
    +----------------------+-------------------+--------------+
    |       Dataset        |   Latency (CPU)   | Latency(FPGA)|
    +======================+===================+==============+
    |   SV - 16x16x16      |     6.75 ms       |     0.55 ms  |
    +----------------------+-------------------+--------------+

.. toctree::
    :maxdepth: 1
