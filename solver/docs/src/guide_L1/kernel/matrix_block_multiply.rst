..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: matrixBlockMultiply 
   :description: matrix block multiply 
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

*******************************************************
Matrix Block Multiply
*******************************************************

Overview
============
If :math:`A` is a `m ×n` matrix and :math:`B` is a `n ×p` matrix, the matrix product :math:`C = AB` (denoted without multiplication signs or dots) is defined to be the m × p matrix.

.. math::
            C = AB

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
  * RowsB            : Number of rows in input matrix B
  * ColsB            : Number of columns in input matrix B
  * RowsC            : Number of rows in input matrix C
  * ColsC            : Number of columns in input matrix C
  * TILE_SIZE        : Dimension of the blocked squared sub-matrix
  * BLK              : The number of rows when loading input matrix to memory. 
  * InputType        : Input data type
  * OutputType       : Output data type

* Arguments:

  * matrixAStrm      : Stream of the first input matrix
  * matrixBStrm      : Stream of the second input matrix
  * matrixLStrm      : Stream of the output matrix 


Implementation 
------------------------

Specifications
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code::

    xf::solver::matrixMultiply<ROWA, COLA, ROWB, COLB, ROWC, COLC, TILE_SIZE, BLK_SIZE, MATRIX_IN_T, MATRIX_OUT_T>(matrixAStrm, matrixBStrm, matrixCStrm);


To calculate :math:`C=A*B`, blocked matrix multiply algorithm is used in our design. Considering the memory limitation, data of the first `matrix A` with size `mxn` are loaded row by row. The number of rows loaded each time is `BLK` which is defined in the template parameter. All of the second input matrix is loaded to memory. After loading is done, blocked matrix multiplication is implemented. Blocked sub-matrix is a square matrix, and the dimension of the sub-matrix is `TILE_SIZE` which is defined in the template parameter.
