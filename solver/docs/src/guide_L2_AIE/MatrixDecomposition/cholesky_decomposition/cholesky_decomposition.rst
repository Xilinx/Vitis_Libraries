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

*****************************
Cholesky Decomposition on AIE
*****************************

Overview
========

This function computes the Cholesky decomposition of matrix :math:`A`

.. math::
    A = L {L}^*

where :math:`A` is a Hermitian positive-definite matrix of size :math:`n \times n`, :math:`L` is a lower triangular matrix with real and positive diagonal entries, and :math:`{L}^*` denotes the conjugate transpose of matrix of :math:`L`.
Every Hermitian positive-definite matrix (and thus also every real-valued symmetric positive-definite matrix) has a unique Cholesky decomposition.


Implementation
==============

Specifications
--------------------
* Target: :math:`A=LL^*`, :math:`A[N*N]` is input matrix, :math:`L[N*N]` is the output matrix via cholesky decomposition. 
* DataType supported: `cfloat`.
* DataSize supported: input matrix size :math:`N` is the times of 4, and :math:`N` shoulb be no more than 256.
* Description: 
    * The AIE core function calculate one column of :math:`matL` and update the rest elements of :math:`matA` 
    * To calculate all columns of :math:`matL`, N AIE cores are used, N is the number of input matrix's row/column number. The first core's output is fed to the second core's input, and on and on, till the last column is computed;
    * The information of matrix dimension, the target column id which will be updated, etc. are read from the head of input streams;

Interfaces
--------------------
.. code::

    void cholesky_complex(input_stream<float>* __restrict matA_real,
                          input_stream<float>* __restrict matA_imag,
                          output_stream<float>* __restrict matL_real,
                          output_stream<float>* __restrict matL_imag);

.. note::

    * As the input data type is cfloat, the input matrix datas are divided into two parts: the one stores the real part of cfloat data, and the other stores the image part of cfloat data.
    * Accordingly, the output datas are divided into two parts as well, the one stores the real part of cfloat data, and the other stores the image part of cfloat data;


* Input:

  *  ``input_stream<float>* matA_real``    stream of the real part of input matrix, contains the lower triangle elements of matrix, column-major.
  *  ``input_stream<float>* matA_imag``    stream of the imag part of input matrix, contains the lower triangle elements of matrix, column-major.

* Output:

  *  ``output_stream<float>* matL_real``    stream of the real part of output matrix, contains the lower triangle elements of matrix, column-major.
  *  ``output_stream<float>* matL_imag``    stream of the image part of output matrix, contains the lower triangle elements of matrix, column-major.


Performance
==============

Test_1
--------------------
* DataSize: matrix size is 64x64;
* Total cycles consumed: 21294

Test_2
--------------------
* DataSize: matrix size is 256x256;
* Total cycles consumed: 207199


.. toctree::
   :maxdepth: 1

