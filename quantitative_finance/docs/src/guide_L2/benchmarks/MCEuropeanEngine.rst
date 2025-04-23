.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: benchmark, European, engine, option
   :description: This is a benchmark of MC (Monte-Carlo) European Engine using the AMD Vitis environment to compare with QuantLib.  
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _guide_l2_benchmark_MCEuropeanEngine_

*************************************************
Benchmark of MCEuropeanEngine
*************************************************


Overview
========
This is a benchmark of MC (Monte-Carlo) European Engine using the AMD Vitis |trade| environment to compare with QuantLib.  It supports hardware emulation as well as running the hardware accelerator on the AMD Alveo |trade| U250.

This example resides in ``L2/benchmarks/MCEuropeanEngine`` directory. The tutorial provides a step-by-step guide that covers commands for build and running kernel.


Executable Usage
================

* **Work Directory(Step 1)**

The steps for library download and environment setup can be found in :ref:`l2_vitis_quantitative_finance`. To get the design,

.. code-block:: bash

   cd L2/benchmarks/MCEuropeanEngine

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

   ./build_dir.hw.xilinx_u250_xdma_201830_2/test.exe -xclbin build_dir.hw.xilinx_u250_xdma_201830_2/kernel_mc.xclbin -rep 1000

Input Arguments:

.. code-block:: bash

   Usage: test.exe    -[-xclbin -rep]
          -xclbin     MCEuropeanEngine binary;
          -rep        repeat number; 

.. Note:: Default num_rep(repeat number) is set in host code. For hw_emu, num_rep is cu_number; for hw, the default value is 1, user could reset num_rep by paramter ``rep``. As this case is a 4CU design, cu_number is 4.   

* **Example output(Step 4)** 

.. code-block:: bash
   ----------------------MC(European) Engine-----------------
   loop_nm = 1000
   Found Platform
   Platform Name: Xilinx
   Found Device=xilinx_u250_xdma_201830_2
   INFO: Importing kernel_mc.xclbin
   Loading: 'kernel_mc.xclbin'
   kernel has been created
   FPGA execution time: 0.273633s
   option number: 20480
   opt/sec: 74844.8
   Expected value: 3.833452
   FPGA result:
            Kernel1 0 - 3.85024         Kernel 1 - 3.8436           Kernel 2 - 3.85006          Kernel 3 - 3.85304
   Execution time 454551


.. _MCEE_Profiling:
Profiling 
==========

The application scenario in this case is:

.. _tab_MCEE_input_parameter:

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
    |  workload     | 1 steps, 47000 paths       |
    +---------------+----------------------------+

The performance comparison of the MCEuropeanEngine is shown in the following table, where timesteps is 1, requiredSamples is 16383, and FPGA frequency is 250MHz. The execution time is the average of 1000 runs. 
Our cold run has 380X and warm run has 1521X compared to baseline.
Baseline is Quantlib, a Widely Used C++ Open Source Library, running on platform with 2 Intel(R) Xeon(R) CPU E5-2690 v4 @3.20GHz, eight cores per processor, and two threads per core.

.. _tab_MCEE_Execution_Time:

.. table:: Timing_Performance

   +-------------------------+-----------------------------------------+
   | Platform                |             Execution time              |
   |                         +-----------------+-----------------------+
   |                         | cold run        | warm run              |
   +-------------------------+-----------------+-----------------------+
   | QuantLib 1.15 on CentOS | 20.155ms        | 20.155ms              |
   +-------------------------+-----------------+-----------------------+
   | Runtime on U250         | 0.053ms         | 0.01325ms             |
   +-------------------------+-----------------+-----------------------+
   | Accelaration Ratio      | 380X            | 1521X                 |
   +-------------------------+-----------------+-----------------------+

.. Note:: 
  What is cold run and warm run? 

  - Cold run means to run one application on board 1 time. 
  - Warm run means to run the application multiple times on board. The E2E is calculated as the average time of multiple runs.
 
The resource utilization and performance of MCEuropeanEngine on U250 FPGA card is listed in the following tables (with Vivado 2021.1).
There are 4CUs on Alveo U250 to pricing the option in parallel. Each CU have the same resource utilization.

.. _tab_MCEE_resource:

.. table:: Resource utilization report of European Option APIs on U250 
    :align: center

    +---------------+---------------------------------+--------+---------+------+------+-------+
    | Implemetation |            Kernels              | LUT    | FF      | BRAM | URAM | DSP   |
    +---------------+---------------------------------+--------+---------+------+------+-------+
    | 4 CUs         |       kernel_mc_0 (UN config:8) | 936288 | 1504828 | 196  | 0    | 6376  |
    |               |       kernel_mc_1 (UN config:8) |        |         |      |      |       |
    |               |       kernel_mc_2 (UN config:8) |        |         |      |      |       |
    |               |       kernel_mc_3 (UN config:8) |        |         |      |      |       |
    +---------------+---------------------------------+--------+---------+------+------+-------+
    | total resource of board                         | 1728000| 3456000 | 2688 | 1280 | 12288 |
    +---------------+---------------------------------+--------+---------+------+------+-------+
    | utilization ratio (not include platform)        | 54.18% | 43.54%  | 7.29%| 0    | 51.88%|
    +---------------+---------------------------------+--------+---------+------+------+-------+

:numref:`tab_MCEE_resource` gives the resource utilization report of four MCEuropeanEngine CUs. Note that the resource statistics are under specific UN (Unroll Number) configurations. These UN configurations are the templated parameters of the corresponding API.

The complete Vitis demo of MCEuropeanEngine is executed with a U250 card on Nimbix. 
The performance of this demo is listed in :numref:`tab_MCEE_performance`. In this table, kernel execution time and end-to-end execution time (E2E) are calculated. 

.. _tab_MCEE_performance:

.. table:: Performance of European Option on U250 
    :align: center

    +---------------+-----------+--------------------------+
    |    Engine     | Frequency | Execution Time (ms)      |
    |               |           +------------+-------------+
    |               |           | kernel     | E2E         | 
    +---------------+-----------+------------+-------------+
    |   4 CUs       |  250MHz   | 7.1ms      | 53ms        |  
    |               |           | (1000 loop)| (1000 loop) |   
    +---------------+-----------+------------+-------------+

Because only one output data is transferred from device to host for each CU, The kernel execution time does not differentiate so much to E2E time.


To maximize the resource utilization on FPGA, four MCEuropeaEngine CUs are placed on different SLRs on U250. Due to place and route on FPGA, the kernel runs at 250MHz finally. 

.. note:: 
  **Analyzation of the execution time of MCEuropeanEngine**

  There are four CUs. Each CU could execution one application at one time. When there are multiple applications, they are distributed on different CUs and could be executed at the same time. So the warm run time is 1/4 of the cold run.


.. toctree::
   :maxdepth: 1

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
