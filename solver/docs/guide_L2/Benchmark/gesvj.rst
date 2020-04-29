
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
   :keywords: GESVJ, Alveo, Lapack, Jacobi, matrix
   :description: The hardware resources and performance for double datatype general matrix (GESVDJ).
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


*******************************************************
Singular Value Decomposition for general matrix (GESVJ)
*******************************************************

The hardware resources and performance for double datatype gesvj is listed in :numref:`tabgesvjDouble`.



.. _tabgesvjDouble:

.. table:: double Type GESVJ performance table
    :align: center

    +-------------+--------+------+------+------+----------+--------+---------------------+-----------------+
    | Matrix Size | Unroll | URAM | BRAM | DSP  | Register |  LUT   | Kernel time (ms)    | Frequency (MHz) |
    +-------------+--------+------+------+------+----------+--------+---------------------+-----------------+
    |    64x64    |    2   |  55  |  27  | 282  |   81753  | 73895  |         27.8        |    300          |
    +-------------+--------+------+------+------+----------+--------+---------------------+-----------------+
    |   512x512   |    4   |  192 |  41  | 500  |   98763  | 92207  |         4827        |    230          |
    +-------------+--------+------+------+------+----------+--------+---------------------+-----------------+
    |   512x512   |   16   |  192 |  125 | 1808 |  203666  | 165800 |        4686.5       |    249          |
    +-------------+--------+------+------+------+----------+--------+---------------------+-----------------+


.. note::
   Board: `Xilinx Alveo U250 Data Center Accelerator Card`


The accuracy of GESVJ implementation has been verified with Lapack dgesvd (QR based SVD) and dgesvj (Jacobi SVD) functions.


.. toctree::
   :maxdepth: 1

