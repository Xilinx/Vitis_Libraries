
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
   :keywords: GESVDJ, Alveo, Lapack, Jacobi, matrix
   :description: The hardware resources and performance for double and float datatype for symmetric matrix (GESVDJ).
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


**********************************************************
Singular Value Decomposition for symmetric matrix (GESVDJ)
**********************************************************

The hardware resources and performance for double and float datatype gesvdj are listed in :numref:`tabSVDDouble` and :numref:`tabSVDFloat`.

.. _tabSVDDouble:

.. table:: double Type GESVDJ performance table
    :align: center

    +-------------+--------+------+------+-----+----------+--------+------------------+----------------+
    | Matrix Size | Unroll | URAM | BRAM | DSP | Register |  LUT   | Kernel time (ms) | Frequency(MHz) |
    +-------------+--------+------+------+-----+----------+--------+------------------+----------------+
    |    8x8      |    4   |   20 |    6 | 216 |   46245  |  39365 |      0.0711      |     250        |
    +-------------+--------+------+------+-----+----------+--------+------------------+----------------+
    |  512x512    |    8   |  128 |  333 | 408 |  120837  | 115121 |     2100.833     |     208.3      |
    +-------------+--------+------+------+-----+----------+--------+------------------+----------------+

.. _tabSVDFloat:

.. table:: float Type GESVDJ performance table
    :align: center

    +-------------+--------+------+------+-----+----------+--------+------------------+----------------+
    | Matrix Size | Unroll | URAM | BRAM | DSP | Register |  LUT   | Kernel time (ms) | Frequency(MHz) |
    +-------------+--------+------+------+-----+----------+--------+------------------+----------------+
    |    8x8      |    4   |   20 |    4 | 114 |   23647  |  18529 |      0.0533      |     250        |
    +-------------+--------+------+------+-----+----------+--------+------------------+----------------+
    |  512x512    |    8   |  128 |  307 | 210 |   65569  |  65003 |     1687.274     |     208.3      |
    +-------------+--------+------+------+-----+----------+--------+------------------+----------------+

.. note::
   Board: `Xilinx Alveo U250 Data Center Accelerator Card`

The accuracy of GESVDJ implementation has been verified with Lapack dgesvd (QR based SVD) and dgesvj (Jacobi SVD) functions. 

.. caution::
    The unroll factor is limited by 2 factors, the matrix size and URAM port. The maximum unroll factor should be less than half of matrix size, and :math:`2 \times {Unroll}^{2}` should also be less than available URAM on board. Besides, unroll factor can only be the factorization of 2


.. toctree::
   :maxdepth: 1
