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
   :keywords: Singular value decomposition
   :description: This function computes the Singular value decomposition of matrix.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

*****************************
Singular value Decomposition 
*****************************

Introduction
==============

This function computes the Singular value decomposition of matrix :math:`A`. Current implementation takes one-sided Jacobi algorithm to solve. One-sided Jacobi is an iterative convergence algorithm, and each graph call will perform one "column-sweep" on a batch of input matrixs. To achieve desired precision, you need to store the output of one graph call and feed them as input to the graph again. Such "output-input" needs to be executed multiple times and the number of repetitions depends on precision requirements. Due to "one-way" through character of current implementation, it will need PL data-movers to servers as the mechanism to loop back last iteration's output into next iteration's input. We will release such PL data-movers in the future.

.. math::
    A = U M V*

where :math:`A` is a matrix of size :math:`m \times n`, :math:`U` is orthogonal matrix of size :math:`m \times m`, :math:`M` is rectangular diagonal matrix of size :math:`m \times n`, :math:`V*` is orthogonal matrix of size :math:`n \times n`


Entry Point 
==============

The graph entry point is the following:

.. code::
    xf::solver::SVDComplexFloat

Template Parameters
---------------------
* `column_num`: the number of columns;
* `row_num`: the number of rows;
* `k_rep`: Each graph call will perform one "column-sweep" on all `k_rep` input matrixs and make columns.

To access more details, see :ref:`AIE APIs Overview`.

Ports
-------
To access more details, see :ref:`AIE APIs Overview`.


AIE Kernel
===============

Design Notes
--------------------
* Target: :math:`A=UMV*`, :math:`A[M*N]` is input matrix, :math:`U[M*M]`, :math:`V*[N*N]` and  :math:`R[M*N]` are the output matrix via singular value decomposition. 
* DataType supported: `cfloat`.
* DataSize supported: input matrix size :math:`M` is the times of 4 and no bigger than 1024, and :math:`N` shoulb be no bigger than 256.
* Description: 
    Singular value decomposition took one-sided Jacobi algorithm to solve. It's an iterative approximation algorithm. Each time the AIE graph perform "column-sweep" on the input matrixs and make columns of its inputs more "orthogonal". After number of iterations, the final result is considered to be good enough as approximation of result of singular value decomposition. The number of iteration needed depends on size of matrix and precision requirements. Thus our design does not put any assumption on it and leave it to users to determine how many iterations they need.

Kernel Interfaces
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

