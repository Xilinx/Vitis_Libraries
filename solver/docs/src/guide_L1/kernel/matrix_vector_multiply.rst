..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: GEMV 
   :description: matrix vector multiply 
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

*******************************************************
GEMV (Matrix Vector Multiply)
*******************************************************

Overview
============
A vector `x` of length `n` can be viewed as a column vector, corresponding to a `nx1` matrix `X`.  If `A` is a `mxn` matrix, the matrix-times-vector product denoted by `Ax` is then the vector `y` that, viewed as a column vector, is equal to the `mx1` matrix `AX`.

.. math::
            y = Ax

In this design, blocked matrix multiplication algorithm is applied.

Implementation
==============

DataType Supported
--------------------
* float
* std::complex<float>

.. note::
   Subnormal values are not supported. If used, the synthesized hardware flushes these to zero, and the behavior differs versus software simulation.

Interfaces
--------------------
* Template parameters:

  * RowsA            : Number of rows in input matrix A
  * ColsA            : Number of columns in input matrix A
  * BR               : Row number of the blocked sub-matrix
  * BC               : Column number of the blocked sub-matrix
  * InputType        : Input data type
  * OutputType       : Output data type

* Arguments:

  * matrixAStrm      : Stream of the first input matrix
  * matrixBStrm      : Stream of the second input vector 
  * matrixLStrm      : Stream of the output vector 


Implementation 
------------------------

Specifications
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code::

    xf::solver::gemv<ROWA, COLA, BR, BC, MATRIX_IN_T, MATRIX_OUT_T>(matrixAStrm, matrixBStrm, matrixCStrm);


To calculate :math:`y=A*x`, blocked matrix multiply algorithm is used in our design. The row number `BR` and column number `BC` of the blocked sub-matrix are defined in template parameters.
