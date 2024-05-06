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
   :description: This function computes the QR decomposition of matrix based on Householder algorithm.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

**********************************
QR Decomposition with Householder
**********************************

Introduction
==============

This function computes the QR decomposition of matrix :math:`A` based on householder algorithm.

.. math::
    A = Q R

where :math:`A` is a matrix of size :math:`m \times n` with `m >= n`, :math:`Q` is orthogonal matrix of size :math:`m \times m`, and :math:`R` is an upper triangular matrix with zero rows from the (n+1)-st row, the size of matrix R is :math:`m \times n`. 
If matirx A is a complex square matrix, then there is a decomposition A = QR where Q is a unitary matrix.
The Householder method: Apply a succession of orthogonal matrices :math:`Q_k` to :math: `A` to compute upper triangular matrix R. :math:`Q_k` introduces zeros below the diagonal in column k, while preserving all the zeros previously introduced. 

.. math::
    Target: A = Q R
    Algorithm: 
    * Q_N*...*Q_2*Q_1 * A = R
    * conjugate_transpose(Q) = Q_N* ...*Q_2*Q_1

Entry Point 
==============

The graph entry point is the following:

.. code::
    xf::solver::QRD_Householder_Graph

Template Parameters
---------------------
* `row_num`: the number of rows;
* `column_num`: the number of columns;

To access more details, see :ref:`AIE APIs Overview`.

Ports
-------
To access more details, see :ref:`AIE APIs Overview`.


AIE Graph
===============

Design Notes
--------------------
* Target: :math:`A=QR`, :math:`A[M*N]` is input matrix, :math:`Q[M*M]` is orthogonal matrix and  :math:`R[M*N]` is an upper trapezoidal matrix. 
* DataType supported: `cfloat`.
* DataSize supported: input matrix size :math:`M` is the times of 4 and no bigger than 512, and :math:`N` shoulb be no bigger than 256.
* Description: 
    * For single AIE core, AIE core function calculate the reflection vector in column k, and then update the rest of elements of :math:`matA` and update orthogonal matrix math:`matQ` using the reflection vector.
    * For the whole design, :math:`N` AIE cores are used, :math:`N` is the number of input matrix's column number. The reason is that zeros below diagonal is introduced column by column.
    * The previous core's output is fed to the next core's input, and on and on, till the last column is computed;
* Implementation Notes:
    * This design utilzied "HouseHolder" method to solve QR decomposition.
    * This design takes two streams as input interface and two streams as output interface.
    * It takes input of matrix A and matrix I (identity matrix) as inputs, and generate matrix Q and R as output.
    * Matrix A and I are concated in row to form a (m+n) rows x (n) columns matrix, so Elements[0:M-1] of each column are from A and Elements[M:M+N-1] are from I.
    * Matrix Q and R are concated in row to form a (m+n) rows x (n) columns matrix, so Elements[0:M-1] of each column are from Q and Elements[M:M+N-1] are from R.
    * Concated inputs are injected column by column, concated outputs are extracted in the same way.
    * Each column of inputs are injected to two input stream in such way: Elem[N*4] and Elem[N*4+1] to stream 0, Elem[N*4+2], Elem[N*4+3] to stream 1.
    * Each column of outputs are extracted in the same way as inputs, but from output stream 0 and 1.

Graph Interfaces
--------------------

.. code::

   template <int M, int N>
   void qrd_householder(input_stream<cfloat>* __restrict matAU_0,
                        input_stream<cfloat>* __restrict matAU_1,
                        output_stream<cfloat>* __restrict matRQ_0,
                        output_stream<cfloat>* __restrict matRQ_1,
                        const int column_id);

.. note::

   * To utilize bandwidth of input / output stream, the input matrix and output result are transfered in such way: Elem[N*4] and Elem[N*4+1] are transferred with matAU_0/matRQ_0, Elem[N*4+2] and Elem[N*4+3] are transferred with matAU_1/matRQ_1. 


* Input:

  *  ``input_stream<cfloat>* matAU_0``    stream of input matrix, contains lower two elements of each 4 elements.
  *  ``input_stream<cfloat>* matAU_1``    stream of input matrix, contains higher two elements of each 4 elements.
  *  ``column_id``                        column id, the elements below diagonal will be zeroed.

* Output:

  *  ``input_stream<cfloat>* matRQ_0``    stream of output matrix, contains lower two elements of each 4 elements.
  *  ``input_stream<cfloat>* matRQ_1``    stream of output matrix, contains higher two elements of each 4 elements.