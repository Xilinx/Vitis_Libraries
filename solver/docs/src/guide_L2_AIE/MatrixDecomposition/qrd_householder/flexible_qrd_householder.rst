.. 
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
  
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
   :keywords: QR Decomposition with Householder
   :description: This function computes the QR decomposition of matrix with Householder algorithm using a flexible way.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

********************************************
Flexible QR Decomposition with Householder
********************************************

Introduction
==============

This function computes the QR decomposition of matrix :math:`A` based on householder algorithm.
Input matrix :math:`A` is a matrix of size :math:`m \times n` with `m >= n`, output matrix :math:`Q` is orthogonal matrix of size :math:`m \times m`, and output matrix :math:`R` is an upper triangular matrix with zero rows from the (n+1)-st row, the size of matrix R is :math:`m \times n`. 
If matirx A is a complex square matrix, then there is a decomposition A = QR where Q is a unitary matrix.

.. math::
    A = Q R

The Householder method: Apply a succession of orthogonal matrices :math:`Q_k` to :math: `A` to compute upper triangular matrix R. :math:`Q_k` introduces zeros below the diagonal in column k, while preserving all the zeros previously introduced. 

.. math::
    * Q_N*...*Q_2*Q_1 * A = R

.. math::
    * conjugate_transpose(Q) = Q_N* ...*Q_2*Q_1

In this design, customer could instantiate some parameters to trade-off AIE tile resouces for throughput. 

Entry Point 
==============

The graph entry point is the following:

.. code::
    xf::solver::QRDHouseholderComplexFlexible

Template Parameters
---------------------
* `ROW`: the row number of input matrix;
* `COL`: the column number of input matrix;
* `CoreNum`: the number of AIE cores used.
* `BlkNum`: the number of matrix columns are calculated by each core.

To access more details, see :ref:`AIE APIs Overview`.

Ports
-------
To access more details, see :ref:`AIE APIs Overview`.


AIE Graph
===============

Design Notes
--------------------
* Target: :math:`A=QR`, :math:`A[M*N]` is input matrix, :math:`Q[M*M]` is orthogonal matrix and  :math:`R[M*N]` is an upper trapezoidal matrix. 
* DataType: `cfloat` is supported.
* DataSize: input matrix column number :math:`N` is times of `BlkNum`, `BlkNum` is the number of columns calculated by each core. :math:`N` shoulb be no more than 256.
* Description: 
    * This design utilzied "HouseHolder" method to solve QR decomposition.
    * For each AIE core, number of `BlkNum` reflection vectors are calculated, the related :math:`matA` datas and the orthogonal matrix math:`matQ` datas are updated using the `BlkNum` reflection vectors. The number of `BlkNum` could be instantiated by users.
    * For the whole design, :math:`CoreNum` AIE cores are used, :math:`CoreNum = COL/BlkNum`. 
    * The previous core's output is fed to the next core's input, and on and on, till the last column is computed done.

Graph Interfaces
--------------------

.. code::
    void QRDHouseholderComplexFlexible::run(
                         adf::input_buffer<cfloat, adf::extents<adf::inherited_extent> >& __restrict in,
                         adf::output_buffer<cfloat, adf::extents<adf::inherited_extent> >& __restrict out);

* Input:

  *  ``input_buffer<cfloat>* in``    input buffer port, contains the input matrix datas column by column. 

* Output:

  *  ``output_buffer<cfloat>* out``  output buffer port, contains the output matrix datas column by column.


Performance
==============

Test_1
--------------------
* DataSize: matrix size is 128x64
* DataType: cflaot
* BlkNum: 4
* CoreNum: 16 
* AIE simulation time: 799912ns 

Test_2
--------------------
* DataSize: matrix size is 256x128
* DataType: cflaot
* BlkNum: 4
* CoreNum: 32
* AIE simulation time: 2875us 

.. toctree::
   :maxdepth: 1

