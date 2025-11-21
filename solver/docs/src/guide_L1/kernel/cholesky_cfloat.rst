..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Cholesky_cfloat
   :description: Cholesky Decomposition for cfloat datatype
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

*******************************************************
Cholesky 
*******************************************************

Overview
============
Cholesky decomposition is a decomposition of a Hermitian, positive-definite matrix into the product of a lower triangular matrix and its conjugate transpose, in the form of :math:`A = LL^*`. :math:`A` is a Hermitian positive-definite matrix, :math:`L` is a lower triangular matrix with real and positive diagonal entries, and :math:`L^*` denotes the conjugate transpose of :math:`L`. 
Cholesky decomposition is useful for efficient numerical solutions. 

.. math::

            A = L*L^* 

Implementation
==============

DataType Supported
--------------------
* std::complex<float>

.. Note::
   * The function assumes that the input matrix is a Hermitian positive definite for complex-valued inputs.

Interfaces
--------------------
* Template parameters:

   -  LowerTriangularL       Defines the output matrix is lower or upper triangular matrix 
   -  DIM                    Defines the input matrix dimensions
   -  InputType              Input data type
   -  OutputType             Output data type
   
* Arguments:

    - matrixAStrm             Stream of Square Hermitian positive definite input matrix
    - matrixLStrm             Stream of Lower or upper triangular output matrix 


Implementation Controls
------------------------

Specifications
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code::

    xf::solver::cholesky_cfloat<LOWER_TRIANGULAR, DIM, MATRIX_IN_T, MATRIX_OUT_T>(matrixAStrm, matrixLStrm);
    

