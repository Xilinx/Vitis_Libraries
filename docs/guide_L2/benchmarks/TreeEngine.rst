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


*************************************************
Benchmark of Tree Engine
*************************************************

Overview
========
This is a benchmark based on tree structure using the SDx environment to compare with QuantLib, where the Rate Model supports multiple models, including Vasicek, HullWhite, BlackKarasinski, CoxIngersollRoss, ExtendedCoxIngersollRoss, Two-additive-factor gaussian, and the Instrument supports multiple Instruments, including swaption, swap, capfloor, callablebond. It supports software and hardware emulation as well as running the hardware accelerator on the Alveo U250.


Highlights
==========
the benchmarks include 2 parts: TreeSwaptionEngineHWModel and TreeHWModelEngine.

Baseline is Quantlib, a Widely Used C++ Open Source Library, running on platform with 2 Intel(R) Xeon(R) CPU E5-2667 v3 @3.200GHz, 8 cores per procssor and 2 threads per core.

TreeSwaptionEngine
------------------
The performance of the TreeEngine based on the swaption and different models is shown in the table below.

.. _tab_performance1:

.. table:: performance based on swaption
   :align: center

   +------------+------------------------+--------------------------------+
   |            |                        | Timesteps                      |
   | Model      | platform               +-------+-------+-------+--------+
   |            |                        | 50    | 100   | 500   | 1000   |
   +------------+------------------------+-------+-------+-------+--------+
   |            | Baseline               | 1.0   | 4.8   | 353.9 | 2493.5 | 
   |            +------------------------+-------+-------+-------+--------+
   | HWModel    | FinTech on U250        |
   |            +------------------------+-------+-------+-------+--------+
   |            | Accelaration Ratio     |
   +------------+------------------------+-------+-------+-------+--------+


TreeHWModelEngine
-----------------
The performance of the TreeEngine based on the HullWhite model and different instruments is shown in the table below.

.. _tab_performance2:

.. table:: performance based HullWhite
   :align: center

   +------------+------------------------+--------------------------------+
   |            |                        | Timesteps                      |
   | Instrument | platform               +-------+-------+-------+--------+
   |            |                        | 50    | 100   | 500   | 1000   |
   +------------+------------------------+-------+-------+-------+--------+
   |            | Baseline               | 1.0   | 4.8   | 353.9 | 2493.5 | 
   |            +------------------------+-------+-------+-------+--------+
   | Swaption   | FinTech on U250        |
   |            +------------------------+-------+-------+-------+--------+
   |            | Accelaration Ratio     |
   +------------+------------------------+-------+-------+-------+--------+





Profiling
=================
The resource utilization and performance of Engine on U250 FPGA card is listed in the following tables.

.. _tab_MCAE_resource:

.. table:: Resource utilization report on U250
    :align: center

    +-------------------------------+-------+---------+--------+---------+----------+---------+--------------+
    |  Engine                       |  PUs  |  BRAM   |  URAM  |   DSP   |    FF    |   LUT   |  FPGA Clock  |
    +-------------------------------+-------+---------+--------+---------+----------+---------+--------------+
    |  treeSwaptionEngineHWModel    |       |   112   |   0    |   452   |   87469  |  67212  |      3.053   |
    +-------------------------------+-------+---------+--------+---------+----------+---------+--------------+
    |  treeSwaptionEngineBKModel    |       |   116   |   0    |   495   |   99209  |  82034  |      3.190   |
    +-------------------------------+-------+---------+--------+---------+----------+---------+--------------+
    |  treeSwaptionEngineCIRModel   |       |   104   |   0    |   417   |   82910  |  51160  |      3.110   |
    +-------------------------------+-------+---------+--------+---------+----------+---------+--------------+
    |  treeSwaptionEngineECIRModel  |       |   116   |   0    |   442   |   102802 |  81395  |      3.205   |
    +-------------------------------+-------+---------+--------+---------+----------+---------+--------------+
    |  treeSwaptionEngineVModel     |       |   104   |   0    |   377   |   74551  |  48322  |      3.054   |
    +-------------------------------+-------+---------+--------+---------+----------+---------+--------------+
    |  treeSwaptionEngineG2Model    |       |   18    |   136  |   625   |   139467 |  90205  |      3.896   |
    +-------------------------------+-------+---------+--------+---------+----------+---------+--------------+
    |  treeSwapEngineHWModel        |       |   104   |   0    |   408   |   84628  |  65744  |      3.896   |
    +-------------------------------+-------+---------+--------+---------+----------+---------+--------------+
    |  treeCapFloprEngineHWModel    |       |   104   |   0    |   364   |   79489  |  64863  |      3.180   |
    +-------------------------------+-------+---------+--------+---------+----------+---------+--------------+
    |  treeCallableEngineHWModel    |       |   104   |   0    |   320   |   76577  |  62445  |      3.043   |
    +-------------------------------+-------+---------+--------+---------+----------+---------+--------------+




.. toctree::
   :maxdepth: 1
