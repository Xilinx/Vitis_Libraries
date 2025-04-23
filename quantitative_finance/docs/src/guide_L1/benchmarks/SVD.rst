.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: benchmark, European, engine, option
   :description: This is a benchmark of MC (Monte-Carlo) European Engine using the AMD Vitis |trade| environment to compare with QuantLib.  
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _guide_l1_benchmark_SVD_

*************************************************
Benchmark of Singular Value Decomposition (SVD) 
*************************************************


Overview
========
This is a benchmark of Singular Value Decomposition.  It supports hardware emulation as well as running the hardware accelerator on the AMD Alveo |trade| U250.

This example resides in ``L1/benchmarks/SVD`` directory. The tutorial provides a step-by-step guide that covers commands for build and running kernel.


Executable Usage
================

* **Work Directory(Step 1)**

The steps for library download and environment setup can be found in :ref:`l2_vitis_quantitative_finance`. To get the design,

.. code-block:: bash

   cd L1/benchmarks/SVD

* **Build kernel(Step 2)**

Run the following make command to build your XCLBIN and host binary targeting a specific device. This process might take long.

.. code-block:: bash

   source /opt/xilinx/Vitis/2021.1/settings64.sh
   source /opt/xilinx/xrt/setenv.sh
   export PLATFORM=/opt/xilinx/platforms/xilinx_u250_xdma_201830_2/xilinx_u250_xdma_201830_2.xpfm
   export TARGET=hw
   make run 

* **Run kernel(Step 3)**

To get the benchmark results, run the following command.

.. code-block:: bash

   ./build_dir.hw.xilinx_u250_xdma_201830_2/host.exe -xclbin build_dir.hw.xilinx_u250_xdma_201830_2/kernel_svd_0.xclbin 


Input Arguments:

.. code-block:: bash

   Usage: test.exe    -[-xclbin -rep]
          -xclbin     MCEuropeanEngine binary;

* **Example output(Step 4)** 

.. code-block:: bash
    Found Platform
    Platform Name: Xilinx
    Found Device=xilinx_u250_xdma_201830_2
    INFO: Importing ./build_dir.hw.xilinx_u250_xdma_201830_2/kernel_svd_0.xclbin
    Loading: './build_dir.hw.xilinx_u250_xdma_201830_2.xclbin'
    kernel has been created
    finished data transfer from h2d
    Kernel 0 done!
    kernel execution time : 22 us
    result correct
    

.. _SVD_Profiling:
Profiling 
==========

The timing performance of the 4x4 SVD is shown in the following table, where matrix size is 4 x 4, and FPGA frequency is 300MHz.

.. _tab_SVD_Execution_Time:

.. table:: Timing_Performance

   +-----------------------------------+----------------------------------+
   | Platform                          |          Execution time          |
   |                                   +-----------------+----------------+
   |                                   | cold run        | warm run       |
   +-----------------------------------+-----------------+----------------+
   | MKL Intel(R) Xeon(R) E5-2690 v3   |   N/A           |   8 us         |
   +-----------------------------------+-----------------+----------------+
   | FinTech on U250                   |   196 us        |   22 us        |
   +-----------------------------------+-----------------+----------------+
   | Accelaration Ratio                |   N/A           |   0.36X        |
   +-----------------------------------+-----------------+----------------+


The hardware resources are listed in the following table (vivado 18.3 report without platform).

.. _tab_SVD_resource:

.. table:: Resource utilization report of SVD on U250 
    :align: center

   +---------------+------+------+------+--------+--------+
   | Inmlemetation | BRAM | URAM | DSP  | FF     | LUT    |
   +---------------+------+------+------+--------+--------+
   |       SVD     |  9   |  0   | 126  | 46360  | 40313  |
   +---------------+------+------+------+--------+--------+

.. toctree::
   :maxdepth: 1

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
