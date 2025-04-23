.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: benchmark, American, engine, option
   :description: This is a benchmark of MC (Monte-Carlo) European Engine using the AMD Vitis environment to compare with QuantLib.   
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


*************************************************
Benchmark of MCAmericanEngineMultiKernel
*************************************************


Overview
========
This is a benchmark of MC (Monte-Carlo) European Engine using the AMD Vitis |trade| environment to compare with QuantLib.  It supports hardware emulation as well as running the hardware accelerator on the AMD Alveo |trade| U250.

This example resides in ``L2/benchmarks/MCAmericanEngineMultiKernel`` directory. The tutorial provides a step-by-step guide that covers commands for build and running kernel.

Executable Usage
=================

* **Work Directory(Step 1)**

The steps for library download and environment setup can be found in :ref:`l2_vitis_quantitative_finance`. To get the design,

.. code-block:: bash

   cd L2/benchmarks/MCEuropeanEngine

* **Build kernel(Step 2)**

Run the following make command to build your XCLBIN and host binary targeting a specific device. This process takes long.

.. code-block:: bash

   source /opt/xilinx/Vitis/2021.1/settings64.sh
   source /opt/xilinx/xrt/setenv.sh
   export PLATFORM=/opt/xilinx/platforms/xilinx_u250_xdma_201830_2/xilinx_u250_xdma_201830_2.xpfm
   export TARGET=hw
   make run 

* **Run kernel(Step 3)**

To get the benchmark results, run the following command.

.. code-block:: bash

   ./build_dir.hw.xilinx_u250_xdma_201830_2/host.exe -xclbin build_dir.hw.xilinx_u250_xdma_201830_2/MCAE_k.xclbin -cal 4096 -s 100 -p 2

Input Arguments:

.. code-block:: bash

   Usage: test.exe    -[-xclbin -cal -s -p]
          -xclbin     MCEuropeanEngine binary;
          -cal        to config the value `calibSamples`, default value is 4096; 
          -s          to config the value `timeSteps`, default value is 100;
          -p          to config the valude `requiredSamples`, default value is `24576/cu_number`, the cu_number is 2;


* **Example output(Step 4)** 

.. code-block:: bash

   ----------------------MC(American) Engine-----------------
   data_size is 409600
   Found Platform
   Platform Name: Xilinx
   Found Device=xilinx_u250_xdma_201830_2
   INFO: Importing MCAE_k.xclbin
   Loading: 'MCAE_k.xclbin'
   kernel has been created
   kernel start------
   kernel end------
   Execution time 8333us
   output = 3.97936
   PASSED!!! the output is confidential!

.. _MCAE_Profiling:
Profiling 
==========

The application scenario in this case is:

.. _tab_MCAE_input_parameter:

.. table:: Application Scenario

    +---------------+----------------------------+
    |  Option Type  | put                        |
    +---------------+----------------------------+
    |  strike       | 40                         |
    +---------------+----------------------------+
    |  underlying   | 36                         |
    +---------------+----------------------------+
    | risk-free rate| 6%                         |
    +---------------+----------------------------+
    |  volatility   | 20%                        |
    +---------------+----------------------------+
    | dividend yield| 0                          |
    +---------------+----------------------------+
    |  maturity     | 1 year                     |
    +---------------+----------------------------+
    |  tolerance    | 0.02                       |
    +---------------+----------------------------+
    |  workload     | 100 steps, 24576 paths     |
    +---------------+----------------------------+


The performance comparison of the MCAmericanEngine is shown in the following table, where timesteps is 100, requiredSamples is 24576, calibSamples is 4096, and FPGA frequency is 300MHz. 
Baseline is Quantlib, a Widely Used C++ Open Source Library, running on CPU with one thread. The  CPU is Intel(R) Xeon(R) CPU E5-2667 v3 @3.200GHz, eight cores per processor and two threads per core.
Our cold run has 197X and warm run has 590X compared to baseline.

.. _tab_MCAE_Execution_Time:

.. table:: Timing_Performance

   +-------------------------+-----------------+------------------+
   | Platform                |             Execution time         |
   +                         +-----------------+------------------+
   |                         | cold run        | warm run         |
   +=========================+=================+==================+
   | Baseline                | 1156.5ms        | 1156.5ms         |
   +-------------------------+-----------------+------------------+
   | Runtime on U250         | 5.87ms          | 1.96ms           |
   +-------------------------+-----------------+------------------+
   | Accelaration Ratio      | 197X            | 590X             |
   +-------------------------+-----------------+------------------+

.. note:: 
  What is cold run and warm run? 

  - Cold run means to run the application on board 1 time. 
  - Warm run means to run the application multiple times on board. And for pipeline mode, the application is always running multiple times.  
 

The resource utilization and performance of MCAmericanEngine on U250 FPGA card is listed in the following tables (using viviado 2021.1).

.. _tab_MCAE_Resource:

.. table:: Resource Utilization Report of American Option APIs on U250 
    :align: center

    +---------------+----------------------------+--------+--------+------+------+-----+
    |    Engine     |            APIs            | LUT    | FF     | BRAM | URAM | DSP |
    +---------------+----------------------------+--------+--------+------+------+-----+
    |               | MCAmericanEnginePreSamples | 120756 | 185169 | 43   | 0    | 416 | 
    |               |  (k0: UN=2)                |        |        |      |      |     |
    |               +----------------------------+--------+--------+------+------+-----+
    |   3-kernel    |  MCAmericanEngineCalibrate | 181793 | 267405 | 68   | 0    | 462 |
    |               |  (k1: UN=(2, 2))           |        |        |      |      |     |
    |               +----------------------------+--------+--------+------+------+-----+
    |               |  MCAmericanEnginePricing   | 251370 | 368839 | 71   | 0    | 911 |
    |               |  (k2_0: UN=4; k2_1: UN=4)  |        |        |      |      |     |
    +---------------+----------------------------+--------+--------+------+------+-----+

:numref:`tab_MCAE_resource` gives the resource utilization report of four American option APIs. The resource statistics are under specific UN (Unroll Number) configurations. These UN configurations are the templated parameters of the corresponding API.

The complete Vitis demos of MCAmericanEngineMultiKernel is executed with a U250 card on Nimbix. The performance is listed in :numref:`tab_MCAE_performance`. In this table, kernel execution time and end-to-end execution time (E2E) are calculated. 

.. _tab_MCAE_performance:

.. table:: Performance of MCAmericanEngineMultiKernel on U250 
    :align: center

    +---------------+----------------------------+------------------------+-----------+------------------------------+--------------------------------+
    |    Engine     |    Kernel                  | Configuration          | Frequency | Total Execution Time (ms)    | Execution Time - pipeline (ms) | 
    |               |                            |                        |           +--------+------+--------------+                                |
    |               |                            |                        |           | kernel | E2E  | E2E pipeline |                                |
    +---------------+----------------------------+------------------------+-----------+--------+------+--------------+--------------------------------+
    |               | MCAmericanEnginePreSamples | k0: UN=2               |           |        |      |              | 1.23                           |
    |               +----------------------------+------------------------+           |        |      |              +--------------------------------+
    |   3-kernel    | MCAmericanEngineCalibrate  | k1: Un=2               |  260MHz   | 5.33   | 5.45 | 1.96         | 1.35                           |
    |               +----------------------------+------------------------+           |        |      |              +--------------------------------+
    |               | MCAmericanEnginePricing    | k2_0: UN=4, k2_1: UN=4 |           |        |      |              | 1.56                           |
    +---------------+----------------------------+--------------------+---+-----------+--------+------+--------------+--------------------------------+


Due to place and route on FPGA, the kernel runs at 260MHz finally. Because the last kernel takes the longest time while execution, it is duplicated. The total execution time of k2_0 and k2_1 is 1.56ms. The end-to-end execution time in pipeline mode is 1.96ms, which is only slightly bigger than the execution time of Pricing kernels. 


.. hint::
  Why instance two kernels of MCAmericanEnginePricing with UN=4 instead of using one pricing kernel with UN=8? 

  For one pricing kernel with UN=4, the resource it used is around 74% of one SLR. When UN=8, two SLRs are required to fit one kernel. This means that cross SLR for single kernel has to be processed. This may decrease the overall frequency dramatically. In real applications, we are always trying to avoid cross SLR place & routing. Using two exactly the same pricing kernel with UN=4, each kernel can be successfully placed on one SLR. The cross SLR logic can be avoided. Therefore, two pricing kernels with UN=4 are employed in the end.



.. toctree::
   :maxdepth: 1

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
