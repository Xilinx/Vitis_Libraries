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
   :keywords: Cholesky Decomposition
   :description: This function computes the Cholesky decomposition of matrix.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

***********************
QR Decomposition on AIE
***********************

Overview
========

This function computes the QR decomposition of matrix :math:`A`

.. math::
    A = Q R

where :math:`A` is a matrix of size :math:`m \times n`, :math:`Q` is orthogonal matrix of size :math:`m \times m`, and :math:`R` is an upper triangular matrix. 
If instead A is a complex square matrix, then there is a decomposition A = QR where Q is a unitary matrix.


Implementation
==============

Specifications
--------------------
* Target: :math:`A=QR`, :math:`A[M*N]` is input matrix, :math:`Q[M*M]` and  :math:`R[M*N]` are the output matrix via QR decomposition. 
* DataType supported: `cfloat`.
* DataSize supported: input matrix size :math:`M` is the times of 4 and no bigger than 1024, and :math:`N` shoulb be no bigger than 256.
* Description: 
    * The AIE core function calculate one column of :math:`Q` and update the rest elements of :math:`matA` 
    * To calculate all columns of :math:`Q`, N AIE cores are used, N is the number of input matrix's column number. The first core's output is fed to the second core's input, and on and on, till the last column is computed;
    * The information of matrix dimension, the target column id which will be updated, etc. are read from the head of input streams;

Kernel Interfaces
--------------------

.. code::

   void GramSchmidtKernelComplexFloat::process(input_stream_cfloat* in_0,
                                               input_stream_cfloat* in_1,
                                               output_stream_cfloat* out_0,
                                               output_stream_cfloat* out_1);

.. note::

   * To utilize bandwidth of input / output stream, the input matrix and output result are transfered in such way: Elem[N*4] and Elem[N*4+1] are transferred with in_0 / out_0, Elem[N*4 + 2] and Elem[N*4 + 3] are transferred with in_1 / out_1.


* Input:

  *  ``input_stream_cfloat* in_0``    stream of input matrix, contains lower two elements of each 4 elements.
  *  ``input_stream_cfloat* in_1``    stream of input matrix, contains higher two elements of each 4 elements.

* Output:

  *  ``input_stream_cfloat* out_0``    stream of output matrix, contains lower two elements of each 4 elements.
  *  ``input_stream_cfloat* out_1``    stream of output matrix, contains higher two elements of each 4 elements.

Performance
==============

Test_1
--------------------
* DataSize: matrix size is 64x64;
* Total cycles consumed: 10752

Test_2
--------------------
* DataSize: matrix size is 512x256;
* Total cycles consumed: 344064

