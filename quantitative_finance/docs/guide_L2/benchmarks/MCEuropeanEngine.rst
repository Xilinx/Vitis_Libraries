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
   :keywords: benchmark, European, engine, option
   :description: This is a benchmark of MC (Monte-Carlo) European Engine using the Xilinx Vitis environment to compare with QuantLib.  
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

*************************************************
Benchmark of European Option Pricing Engine
*************************************************


Overview
========
This is a benchmark of MC (Monte-Carlo) European Engine using the Xilinx Vitis environment to compare with QuantLib.  It supports software and hardware emulation as well as running the hardware accelerator on the Alveo U250.


Highlights
==========

The performance of the MCEuropeanEngine is shown in the table below, our cold run has 488X and warm run has 1954X compared to baseline.
Baseline is Quantlib, a Widely Used C++ Open Source Library, running on platform with 2 Intel(R) Xeon(R) CPU E5-2690 v4 @3.20GHz, 8 cores per processor and 2 threads per core.

.. _tab_MCEE_Execution_Time:

.. table:: Performance

   +-------------------------+-----------------------------------------+
   | Platform                |             Execution time              |
   |                         +-----------------+-----------------------+
   |                         | cold run        | warm run              |
   +-------------------------+-----------------+-----------------------+
   | Baseline                | 20.155ms        | 20.155ms              |
   +-------------------------+-----------------+-----------------------+
   | Runtime on U250         | 0.053ms         | 0.01325ms             |
   +-------------------------+-----------------+-----------------------+
   | Accelaration Ratio      | 380X            | 1521X                 |
   +-------------------------+-----------------+-----------------------+

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

Profiling
=================
The resource utilization and performance of MCEuropeanEngine on U250 FPGA card is listed in the following tables (with Vivado 2018.3).
There are 4 PUs on Alveo U250 to pricing the option in parallel.  Each PU have the same resource utilization.

.. _tab_MCEE_resource:

.. table:: Resource utilization report of European Option APIs on U250 
    :align: center

    +---------------+----------------------------+--------+---------+------+------+-------+
    | Implemetation |            Kernels         | LUT    | FF      | BRAM | URAM | DSP   |
    +---------------+----------------------------+--------+---------+------+------+-------+
    | 1 PU          | kernel_mc_0 (UN config:8)  | 234072 | 376207  | 49   | 0    | 1594  | 
    +---------------+----------------------------+--------+---------+------+------+-------+
    | 4 PUs         |       kernel_mc_0          | 936288 | 1504828 | 196  | 0    | 6376  |
    |               |       kernel_mc_1          |        |         |      |      |       |
    |               |       kernel_mc_2          |        |         |      |      |       |
    |               |       kernel_mc_3          |        |         |      |      |       |
    +---------------+----------------------------+--------+---------+------+------+-------+
    | total resource of board                    | 1728000| 3456000 | 2688 | 1280 | 12288 |
    +---------------+----------------------------+--------+---------+------+------+-------+
    | utilization ratio (not include platform)   | 54.18% | 43.54%  | 7.29%| 0    | 51.88%|
    +---------------+----------------------------+--------+---------+------+------+-------+

:numref:`tab_MCEE_resource` gives the resource utilization report of four MCEuropeanEngine PUs (Processing Unit). Note that the resource statistics are under specific UN (Unroll Number) configurations. These UN configurations are the templated parameters of the corresponding API.

The complete Vitis demos of European Option Engine are executed with a U250 card on Nimbix. 
The performance of this demo is listed in :numref:`tab_MCEE_performance`. In this table, kernel execution time and end-to-end execution time (E2E) are calculated. 

.. _tab_MCEE_performance:

.. table:: Performance of European Option demos on U250 
    :align: center

    +---------------+-----------+--------------------------+
    |    Engine     | Frequency | Execution Time (ms)      |
    |               |           +------------+-------------+
    |               |           | kernel     | E2E         | 
    +---------------+-----------+------------+-------------+
    |   4 PUs       |  250MHz   | 7.1ms      | 53ms        |  
    |               |           | (1000 loop)| (1000 loop) |   
    +---------------+-----------+------------+-------------+

Because only output data is transferred from device to host, The kernel execution time doesn't differentiate so much to E2E time.

.. note:: 
  What is cold run and warm run? 

  - Cold run means to run one application on board 1 time. 
  - Warm run means to run the application multiple times on board. The E2E is calculated as the average time of multiple runs.
 

In order to maximize the resource utilization on FPGA, the MCEuropeanEngine PU is duplicated.
There four MCEuropeaEngine PUs are placed different SLRs on U250. Due to place and route on FPGA, the kernel runs at 250MHz finally. 

.. note:: 
  **Analyzation of the execution time of MCEuropeanEngine**

  There are 4 PUs. Each PU could execution one application at one time. When there are multiple applications, they are distributed on different PUs and could by executed at the same time. So the warm run time is 1/4 of the cold run.


.. toctree::
   :maxdepth: 1
