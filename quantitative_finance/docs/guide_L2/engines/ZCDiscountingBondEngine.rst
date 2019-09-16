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
Internal Design of Zero Coupon Bond Engine
*************************************************


Overview
========
A zero-coupon bond is a bond that is purchased at a price below the face value of the bond, does not pay coupon during the contract period, and repays the face value at the time of maturity.

Implemention
============
This engine uses the linear interpolation mothed in L1 to calculate the pricing value based on time (the difference between the maturity date and the reference date, unit is year) and face value. The linear interpolation mothed implements 1D linear interpolation. 

Profiling
=========

The hardware resources are listed in the following table (vivado 18.3 report).

.. table:: Table 1 Hardware resources
    :align: center

    +-----------------+----------+----------+----------+----------+---------+-----------------+
    |  Engine         |   BRAM   |   URAM   |    DSP   |    FF    |   LUT   | clock period(ns)|
    +-----------------+----------+----------+----------+----------+---------+-----------------+
    |  ZCBondEngine   |    0     |    0     |    46    |   12478  |  7997   |       2.580     |
    +-----------------+----------+----------+----------+----------+---------+-----------------+


The following table shows the performance improvement in compare with CPU based Quantlib result on U250. (FPGA System Clock: 300MHz)



