
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
   :keywords: GTSV, Alveo, Lapack, Jacobi, matrix
   :description: The hardware resources and performance for Tridiagonal Linear Solver (GTSV).
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


**********************************************************
Tridiagonal Linear Solver (GTSV)
**********************************************************

The hardware resource and performance are listed in following table.

.. table:: Kernel resource and performance table (double)
    :align: center

    +-------------+--------+------+------+-----+----------+--------+------------------+-----------------+
    | Matrix Size | Unroll | URAM | BRAM | DSP | Register |  LUT   | Kernel time (us) | Frequency (MHz) |
    +-------------+--------+------+------+-----+----------+--------+------------------+-----------------+
    |  1024x1024  |   16   |  128 |  16  | 960 |  260297  | 223889 |      16.6        |      291        |
    +-------------+--------+------+------+-----+----------+--------+------------------+-----------------+

.. note::
   Board: `Xilinx Alveo U250 Data Center Accelerator Card`

.. toctree::
   :maxdepth: 1
