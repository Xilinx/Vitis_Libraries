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
   :keywords: benchmark, tree, engine
   :description: This is a benchmark based on tree structure using the Xilinx Vitis environment to compare with QuantLib, where the Rate Model supports multiple models, including Vasicek, HullWhite, BlackKarasinski, CoxIngersollRoss, ExtendedCoxIngersollRoss, Two-additive-factor gaussian, and the Instrument supports multiple Instruments, including swaption, swap, capfloor, callablebond. 
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


*************************************************
Benchmark of Tree Engine
*************************************************

Overview
========
This is a benchmark based on tree structure using the Xilinx Vitis environment to compare with QuantLib, where the Rate Model supports multiple models, including Vasicek, HullWhite, BlackKarasinski, CoxIngersollRoss, ExtendedCoxIngersollRoss, Two-additive-factor gaussian, and the Instrument supports multiple Instruments, including swaption, swap, capfloor, callablebond. It supports software and hardware emulation as well as running the hardware accelerator on the Alveo U250.


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
   |            | Baseline (ms)          | 1.0   | 4.8   | 353.9 | 2493.5 | 
   |            +------------------------+-------+-------+-------+--------+
   | HWModel    | FinTech on U250 (ms)   | 0.018 | 0.042 | 0.485 | 1.650  |
   |            +------------------------+-------+-------+-------+--------+
   |            | Accelaration Ratio     | 55X   | 114X  | 729X  | 1511X  |
   +------------+------------------------+-------+-------+-------+--------+
   |            | Baseline (ms)          | 1.9   | 8.6   | 438.2 | 2813.1 | 
   |            +------------------------+-------+-------+-------+--------+
   | BKModel    | FinTech on U250 (ms)   | 0.069 | 0.156 | 1.471 | 4.601  |
   |            +------------------------+-------+-------+-------+--------+
   |            | Accelaration Ratio     | 27X   |  55X  | 297X  |  611X  |
   +------------+------------------------+-------+-------+-------+--------+
   |            | Baseline (ms)          | 0.5   | 1.4   |  26.2 | 100.7  | 
   |            +------------------------+-------+-------+-------+--------+
   | CIRModel   | FinTech on U250 (ms)   | 0.007 | 0.014 | 0.119 | 0.361  |
   |            +------------------------+-------+-------+-------+--------+
   |            | Accelaration Ratio     | 71X   | 100X  | 223X  |  278X  |
   +------------+------------------------+-------+-------+-------+--------+
   |            | Baseline (ms)          | 1.1   | 5.5   | 439.5 | 3322.5 | 
   |            +------------------------+-------+-------+-------+--------+
   | ECIRModel  | FinTech on U250 (ms)   | 0.058 | 0.114 | 0.997 | 3.088  |
   |            +------------------------+-------+-------+-------+--------+
   |            | Accelaration Ratio     | 19X   |  48X  | 440X  | 1093X  |
   +------------+------------------------+-------+-------+-------+--------+
   |            | Baseline (ms)          | 0.5   | 1.8   | 40.1  | 161.7  | 
   |            +------------------------+-------+-------+-------+--------+
   | VModel     | FinTech on U250 (ms)   | 0.005 | 0.010 | 0.096 | 0.322  |
   |            +------------------------+-------+-------+-------+--------+
   |            | Accelaration Ratio     | 100X  | 180X  | 417X  |  502X  |
   +------------+------------------------+-------+-------+-------+--------+
   |            | Baseline (ms)          | 258.0 | 2133.5|       |        | 
   |            +------------------------+-------+-------+-------+--------+
   | G2Model    | FinTech on U250 (ms)   | 0.574 | 4.496 |       |        |
   |            +------------------------+-------+-------+-------+--------+
   |            | Accelaration Ratio     | 449X  | 474X  |       |        |
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
   |            | Baseline (ms)          | 1.0   | 4.8   | 353.9 | 2493.5 | 
   |            +------------------------+-------+-------+-------+--------+
   | Swaption   | FinTech on U250 (ms)   | 0.018 | 0.042 | 0.485 | 1.650  |
   |            +------------------------+-------+-------+-------+--------+
   |            | Accelaration Ratio     | 55X   | 114X  | 729X  | 1511X  |
   +------------+------------------------+-------+-------+-------+--------+
   |            | Baseline (ms)          | 1.0   | 4.3   | 291.2 | 2056.5 | 
   |            +------------------------+-------+-------+-------+--------+
   | Swap       | FinTech on U250 (ms)   | 0.014 | 0.032 | 0.361 | 1.226  |
   |            +------------------------+-------+-------+-------+--------+
   |            | Accelaration Ratio     | 71X   | 134X  | 806X  | 1677X  |
   +------------+------------------------+-------+-------+-------+--------+
   |            | Baseline (ms)          | 0.7   | 3.4   | 217.6 | 1581.3 | 
   |            +------------------------+-------+-------+-------+--------+
   | CapFloor   | FinTech on U250 (ms)   | 0.014 | 0.031 | 0.344 | 1.160  |
   |            +------------------------+-------+-------+-------+--------+
   |            | Accelaration Ratio     | 50X   | 109X  | 632X  | 1363X  |
   +------------+------------------------+-------+-------+-------+--------+
   |            | Baseline (ms)          | 1.4   | 3.5   | 155.2 | 1142.1 | 
   |            +------------------------+-------+-------+-------+--------+
   | Callable   | FinTech on U250 (ms)   | 0.015 | 0.033 | 0.374 | 1.260  |
   |            +------------------------+-------+-------+-------+--------+
   |            | Accelaration Ratio     | 93X   | 106X  | 414X  |  906X  |
   +------------+------------------------+-------+-------+-------+--------+





Profiling
=================
The resource utilization and performance of Engine on U250 FPGA card is listed in the following tables.

.. _tab_MCAE_resource:

.. table:: Resource utilization report on U250
    :align: center

    +-------------------------------+-------+---------+--------+---------+----------+---------+-------------+
    |  Engine                       |  PUs  |  BRAM   |  URAM  |   DSP   |   REG    |   LUT   |  FPGA Clock |
    +-------------------------------+-------+---------+--------+---------+----------+---------+-------------+
    |  treeSwaptionEngineHWModel    |  16   |   1136  |   0    |   6128  | 1080922  | 1051628 |   198.3MHz  |
    +-------------------------------+-------+---------+--------+---------+----------+---------+-------------+
    |  treeSwaptionEngineBKModel    |  12   |   936   |   0    |   5128  |  951861  |  970889 |   223.6MHz  |
    +-------------------------------+-------+---------+--------+---------+----------+---------+-------------+
    |  treeSwaptionEngineCIRModel   |  20   |   1064  |   0    |   4896  |  977294  |  870446 |   266.5MHz  |
    +-------------------------------+-------+---------+--------+---------+----------+---------+-------------+
    |  treeSwaptionEngineECIRModel  |  12   |   932   |   0    |   4456  |  963209  |  962656 |   229.8MHz  |
    +-------------------------------+-------+---------+--------+---------+----------+---------+-------------+
    |  treeSwaptionEngineVModel     |  20   |   1276  |   0    |   6636  | 1166999  | 1076988 |   249.4MHz  |
    +-------------------------------+-------+---------+--------+---------+----------+---------+-------------+
    |  treeSwaptionEngineG2Model    |   8   |   308   |  1088  |   4112  |  736199  |  699702 |   204.9MHz  |
    +-------------------------------+-------+---------+--------+---------+----------+---------+-------------+
    |  treeSwapEngineHWModel        |  16   |   1056  |   0    |   5552  | 1045252  | 1024854 |   252.9MHz  |
    +-------------------------------+-------+---------+--------+---------+----------+---------+-------------+
    |  treeCapFloorEngineHWModel    |  16   |   1036  |   0    |   5040  |  981075  | 1009950 |   267.2MHz  |
    +-------------------------------+-------+---------+--------+---------+----------+---------+-------------+
    |  treeCallableEngineHWModel    |  16   |   1056  |   0    |   4528  |  961311  |  983068 |   242.0MHz  |
    +-------------------------------+-------+---------+--------+---------+----------+---------+-------------+




.. toctree::
   :maxdepth: 1
