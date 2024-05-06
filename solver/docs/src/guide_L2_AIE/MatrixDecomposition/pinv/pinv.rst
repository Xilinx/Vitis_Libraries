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
   :keywords: Pseudoinverse
   :description: This function computes the Pseudoinverse of matrix.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

***************
Pseudoinverse
***************

Introduction
==============

This function computes the pseudoinverse of matrix :math:`A`

Entry Point 
==============

The graph entry point is the following:

.. code::
    xf::solver::PInvComplexFloat

Template Parameters
---------------------
* `column_num`: the number of columns;
* `row_num`: the number of rows;
* `round_num`: the number of sweeps to get result converged. `round_num - 1` rounds are for SVD converegence, and the last round to generate final result.
* `batch_num`: the number of number of matrix to be process in one graph call.

To access more details, see :ref:`AIE APIs Overview`.

Ports
-------
To access more details, see :ref:`AIE APIs Overview`.


AIE Graph
===============

Design Notes
--------------------
* DataType supported: `cfloat`.
* DataSize supported: input matrix size :math:`M` is the times of 4 and no bigger than 1024, and :math:`N` shoulb be no bigger than 256.
* Description: 
    Pseudoinverse took SVD to calculate. Underlying SVD takes one-sided Jacobi algorithm to solve. 
    It's an iterative approximation algorithm. Each time the AIE graph perform "column-sweep" on the input matrixs.
    It makes columns of its inputs more "orthogonal". After number of iterations, the final result is considered to be good enough as approximation of result of SVD. 
    The number of iteration needed depends on size of matrix and precision requirements. Thus this design leaves it to users to determine how many iterations they need.
* Implementation Notes:
    * This design utilize "One-sided Jacobi" method to solve SVD first, then calculate pseudoinverse from SVD.
    * This design takes two streams as input interface and two streams as output interface.
    * It needs to take `round_num` rounds to get the final result. Each rounds contains inputs of `batch_num` matrixes. 
    * The first rounds are raw inputs of matrix `A`, identity matrix `I`, and vector of zeros.
    * Each following round after the first one takes last round's output as its inputs.
    * So for first round, inputs are first matrix A, identity matrix, vector of zeros, second matrix A, identity matrix, vector of zeros... `batch_num` th matrix A, identity matrix, vector of zeros.
    * Matrix A, I, zero vectors are concated in row to form a (m+n+1) rows x (n) columns matrix, thus Elements[0:M-1] of each column are from A, Elements[M:M+N-1] is from I, Elements[M+N] is zeros.
    * Matrix U, V, diagonal value of M are concated in row to form a (m+n+1) rows x (n) columns matrix, thus Elements[0:M-1] of each column are from U, Elements[M:M+N-1] is from V, Elements[M+N] is from diagonal of M.
    * Concated inputs are injected column by column, concated outputs are extracted in the same way.
    * Each column of inputs are injected to two input stream in such way: Elem[N*4] and Elem[N*4+1] to stream 0, Elem[N*4+2], Elem[N*4+3] to stream 1.
    * Each column of outputs are extracted in the same way as inputs, but from output stream 0 and 1.
    * In the last round, effective outputs are Elem[0:M-1] of each column, the rests will be placeholder to keep traffic. And each column are actually rows of result matrix.

Graph Interfaces
--------------------

.. code::

   void OneSidedJacobiComplexFloat::process(input_stream_cfloat* in_0,
                                            input_stream_cfloat* in_1,
                                            output_stream_cfloat* out_0,
                                            output_stream_cfloat* out_1);

.. note::

   * To utilize bandwidth of input / output stream, the input matrix and output result are transfered in such way: Elem[N*4] and Elem[N*4+1] are transferred with in_0 / out_0, Elem[N*4 + 2] and Elem[N*4 + 3] are transferred with in_1 / out_1. Input matrix contains :math: `A[M*N]` and :math: `U[M*M]` (original input matrix and zero matrix as place holder if it's first graph call, or output matrix from last graph call) and comes in column-major order, like A[:,0], U[:,0], A[:,1], U[:,1], ect.


* Input:

  *  ``input_stream_cfloat* in_0``    stream of input matrix, contains lower two elements of each 4 elements.
  *  ``input_stream_cfloat* in_1``    stream of input matrix, contains higher two elements of each 4 elements.

* Output:

  *  ``input_stream_cfloat* out_0``    stream of output matrix, contains lower two elements of each 4 elements.
  *  ``input_stream_cfloat* out_1``    stream of output matrix, contains higher two elements of each 4 elements.