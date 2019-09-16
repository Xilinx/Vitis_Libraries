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
Internal Design of CPI CapFloor Engine
*************************************************


Overview
========
A CPI Cap/Floor is a call/put on the CPI. 

Implemention
============
This engine uses the linear interpolation mothed in L1 to calculate the pricing value based on time (the difference between the maturity date and the reference date, unit is year) and strike rate. The linear interpolation mothed implements 2D linear interpolation. 

Profiling
=========

The hardware resources are listed in the following table (vivado 18.3 report).

.. table:: Table 1 Hardware resources
    :align: center

    +----------------------+----------+----------+----------+----------+---------+-----------------+
    |  Engine              |   BRAM   |   URAM   |    DSP   |    FF    |   LUT   | clock period(ns)|
    +----------------------+----------+----------+----------+----------+---------+-----------------+
    |  CPICapFloorEngine   |    0     |    0     |    22    |   11385  |  7625   |       2.966     |
    +----------------------+----------+----------+----------+----------+---------+-----------------+


