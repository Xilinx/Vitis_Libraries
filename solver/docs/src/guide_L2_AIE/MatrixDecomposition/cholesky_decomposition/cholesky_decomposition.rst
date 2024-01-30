.. 
   
.. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Cholesky Decomposition
   :description: This function computes the Cholesky decomposition of matrix.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

*****************************
Cholesky Decomposition 
*****************************

Introduction
==============

This function computes the Cholesky decomposition of matrix :math:`A`

.. math::
    A = L {L}^*

where :math:`A` is a Hermitian positive-definite matrix of size :math:`n \times n`, :math:`L` is a lower triangular matrix with real and positive diagonal entries, and :math:`{L}^*` denotes the conjugate transpose of matrix of :math:`L`.
Every Hermitian positive-definite matrix (and thus also every real-valued symmetric positive-definite matrix) has a unique Cholesky decomposition.


Entry Point
==============

The graph entry point is the following:

.. code::

    xf::solver::CholeskyGraph

Data Format
---------------------
For Cholesky decomposition in our design, float and cfloat data type are supported. 
* For cfloat data type, to improve the performance of Choleksy decomposition on AIE, input matrix datas are divied into real part and image part.
Specifically, for real part of input matrix, the first two values are `N` (row number of input matrix) and `0`, the rest values are the real parts of lower triangle elements of input matrix, stored column by column. 
For image part of input matrix, the fist two values are `0`, the rest values are the image parte of lower triangle elements of input matrix, stored column by column.
* For float data type, the first value is `N` (row number of input matrix), the next three value are `0`, the rest values are lower triangle elements of input matrix, stored column by column.


Template Parameters
---------------------
* `NUM`: the number of aie cores used in the design;

To access more details, see :ref:`AIE APIs Overview`.

Ports
-------
To access more details, see :ref:`AIE APIs Overview`.


AIE Kernel 
==============

Design Notes
---------------

* Target: :math:`A=LL^*`, :math:`A[N*N]` is input matrix, :math:`L[N*N]` is the output matrix via cholesky decomposition. 
* DataType supported: `cfloat`.
* DataSize supported: input matrix size :math:`N` is the times of 4, and :math:`N` shoulb be no more than 256.
* Description: 
    * The AIE core function calculate one column of :math:`matL` and update the rest elements of :math:`matA` 
    * To calculate all columns of :math:`matL`, N AIE cores are used, N is the number of input matrix's row/column number. The first core's output is fed to the second core's input, and continued till the last column is computed;
    * The information of matrix dimension, the target column id which is updated, and so on are read from the head of input streams;

Kernel Interfaces
--------------------

.. code::

    void cholesky_complex(input_stream<float>* __restrict matA_real,
                          input_stream<float>* __restrict matA_imag,
                          output_stream<float>* __restrict matL_real,
                          output_stream<float>* __restrict matL_imag);

.. note::

    * As the input data type is cfloat, the input matrix datas are divided into two parts: the one stores the real part of cfloat data, and the other stores the image part of cfloat data.
    * Accordingly, the output data is divided into two parts as well, the first one stores the real part of cfloat data, and the other stores the image part of cfloat data;


* Input:

  *  ``input_stream<float>* matA_real``    stream of the real part of input matrix, contains the lower triangle elements of matrix, stored column-major.
  *  ``input_stream<float>* matA_imag``    stream of the image part of input matrix, contains the lower triangle elements of matrix, stored column-major.

* Output:

  *  ``output_stream<float>* matL_real``    stream of the real part of output matrix, contains the lower triangle elements of matrix, stored column-major.
  *  ``output_stream<float>* matL_imag``    stream of the image part of output matrix, contains the lower triangle elements of matrix, stored column-major.

.. note::
   * The function assumes that the input matrix is a Hermitian positive definite matrix.

.. code::

    void cholesky_float(input_stream<float>* __restrict matA,
                        output_stream<float>* __restrict matL);

* Input:

  *  ``input_stream<float>* matA``    stream of the input matrix, contains the lower triangle elements of matrix, stored column by column.

* Output:

  *  ``output_stream<float>* matL``    stream of the output matrix, contains the lower triangle elements of matrix, stored column by column.

.. Note::
   * The function assumes that the input matrix is a symmetric positive definite matrix.


Performance
==============

Test_1
--------------------
* DataSize: matrix size is 64x64;
* DataType: cflaot
* Total cycles consumed: 21294

Test_2
--------------------
* DataSize: matrix size is 256x256;
* DataType: cflaot
* Total cycles consumed: 207199


.. toctree::
   :maxdepth: 1

