.. 
   Copyright 2019 Xilinx, Inc.
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

.. meta::
   :keywords: benchmark, American, engine, option
   :description: This is a benchmark of MC (Monte-Carlo) European Engine using the Xilinx Vitis environment to compare with QuantLib.   
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


*************************************************
Benchmark of American Option Pricing Engine
*************************************************


Overview
========
This is a benchmark of MC (Monte-Carlo) European Engine using the Xilinx Vitis environment to compare with QuantLib.  It supports software and hardware emulation as well as running the hardware accelerator on the Alveo U250.


Highlights
==========

The performance of the MCAmericanEngine is shown in the table below, our cold run has 176X and warm run has 529X compared to baseline.
Baseline is Quantlib, a Widely Used C++ Open Source Library, running on CPU with 1 thread. The  CPU is Intel(R) Xeon(R) CPU E5-2667 v3 @3.200GHz, 8 cores per procssor and 2 threads per core.


.. _tab_MCAE_Execution_Time:

.. table:: performance 

   +-------------------------+-----------------+-----------------------+
   | Platform                |             Execution time              |
   +                         +-----------------+-----------------------+
   |                         | cold run        | warm run              |
   +=========================+=================+=======================+
   | Baseline                | 1038.105ms      | 1038.105ms            |
   +-------------------------+-----------------+-----------------------+
   | Runtime on U250         | 5.87ms          | 1.96ms                |
   +-------------------------+-----------------+-----------------------+
   | Accelaration Ratio      | 176X            | 529X                  |
   +-------------------------+-----------------+-----------------------+


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

Profiling
=================
The resource utilization and performance of MCAmericanEngine on U250 FPGA card is listed in the following tables (using viviado 2018.3).

.. _tab_MCAE_resource:

.. table:: Resource utilization report of American Option APIs on U250 
    :align: center

    +---------------+----------------------------+--------+--------+------+------+-----+
    |    Engine     |            APIs            | LUT    | FF     | BRAM | URAM | DSP |
    +---------------+----------------------------+--------+--------+------+------+-----+
    | 1-kernel ver. | MCAmericanEngine           | 295301 | 435249 | 103  | 0    | 923 |
    |               |      (UN config: 112)      |        |        |      |      |     |
    +---------------+----------------------------+--------+--------+------+------+-----+
    |               | MCAmericanEnginePreSamples | 120756 | 185169 | 43   | 0    | 416 | 
    |               |      (UN config: 2)        |        |        |      |      |     |
    |               +----------------------------+--------+--------+------+------+-----+
    | 3-kernel ver. |  MCAmericanEngineCalibrate | 181793 | 267405 | 68   | 0    | 462 |
    |               |      (UN config: 22)       |        |        |      |      |     |
    |               +----------------------------+--------+--------+------+------+-----+
    |               |  MCAmericanEnginePricing   | 251370 | 368839 | 71   | 0    | 911 |
    |               |      (UN config: 4)        |        |        |      |      |     |
    +---------------+----------------------------+--------+--------+------+------+-----+

:numref:`tab_MCAE_resource` gives the resource utilization report of four American option APIs. Note that the resource statistics are under specific UN (Unroll Number) configurations. These UN configurations are the templated parameters of the corresponding API.

The complete Vitis demos of American Option Engine are executed with a U250 card on Nimbix. The performance of two demos is listed in :numref:`tab_MCAE_performance`. In this table, kernel execution time and end-to-end execution time (E2E) are calculated. Besides, for end-to-end execution time, since pipeline mode can be used, especially for 3-kernel version American option engine, pipeline mode execution time is also collected. 

.. _tab_MCAE_performance:

.. table:: Performance of American Option demos on U250 
    :align: center

    +---------------+-------------------------+-----------+------------------------------+
    |    Engine     | Configuration(UN)       | Frequency | Execution Time (ms)          | 
    |               |                         |           +--------+------+--------------+
    |               |                         |           | kernel | E2E  | E2E pipeline | 
    +---------------+-------------------------+-----------+--------+------+--------------+
    | 1-kernel ver. | 112 (UN_PATH=1,         |  300MHz   | 7.1    | 7.25 | 5.87         |  
    |               | UN_STEP=1,UN_PRICING=2) |           |        |      |              |   
    +---------------+-------------------------+-----------+--------+------+--------------+
    | 3-kernel ver. | 2244 (k0:UN=2, k1:UN=2, |  260MHz   | 5.33   | 5.45 | 1.96         |          
    |               | k2: UN=4, k3: UN=4)     |           |        |      |              |    
    +---------------+-------------------------+-----------+--------+------+--------------+


We can observe that the 1-kernel version engine may run at the frequency of 300MHz. And the execution time for end-to-end is 7.25ms. However, for pipeline mode, the execution time is 5.87ms. But in fact, only one output data is transferred from device to host. The reason that E2E and E2E pipeline time differentiate so much is that E2E time is the cold run time and the time of E2E pipeline is warm run.

.. note:: 
  What is cold run and warm run? 

  - Cold run means to run the application on board 1 time. 
  - Warm run means to run the application multiple times on board. And for pipeline mode, the application is always running multiple times.  

For 3-kernel version MCAmerican Engine, in order to maximize the resource utilization on FPGA, the kernel MCAmericanEnginePricing is duplicated. Each Pricing kernel is unrolled with UN 4. Due to place and route on FPGA, the kernel runs at 260MHz finally. However, since the kernel may run in pipeline mode. The final execution time may reach 1.96ms.

.. note:: 
  **Analyzation of the execution time of 3-kernel version American Engine in pipeline mode**

  The execution time of each kernel is collected and listed in the table below. Because the last kernel takes the longest time while execution, it is duplicated. Which means two MCAmericanEnginePricing kernels, k2 and k3, are included in the final Vitis demo of 3-kernel MCAmerican Engine. The execution time of k2+k3 is 1.56ms. Finally, the end-to-end execution time in pipeline mode is 1.96ms, which is only slightly bigger than the execution time of Pricing kernels. 

  .. _tab_MCAE_3kernel:

  .. table:: Execution time of each kernel in American Option demo on U250 
    :align: center

    +----------------------------+--------------------+--------------------------------+
    |    Kernel                  | Configuration      | Execution Time - pipeline (ms) | 
    +----------------------------+--------------------+--------------------------------+
    | MCAmericanEnginePreSamples | UN = 2             | 1.23                           |
    +----------------------------+--------------------+--------------------------------+
    | MCAmericanEngineCalibrate  | Un = 2             | 1.35                           |
    +----------------------------+--------------------+--------------------------------+
    | MCAmericanEnginePricing    | UN = 4, duplicated | 1.56                           |
    +----------------------------+--------------------+--------------------------------+

.. hint::
  Why instance two kernels of MCAmericanEnginePricing with UN=4 instead of using one pricing kernel with UN=8? 

  For one pricing kernel with UN=4, the resource it used is around 74% of one SLR. When UN=8, two SLRs are required to fit 1 kernel. This means that cross SLR for single kernel has to be processed. This may decrease the overall frequency dramatically. In real applications, we are always trying to avoid cross SLR place & routing. Using two exactly the same pricing kernel with UN=4, each kernel can be successfully placed on one SLR. The cross SLR logic can be avoided. Therefore, two pricing kernels with UN=4 are employed in the end.



.. toctree::
   :maxdepth: 1
