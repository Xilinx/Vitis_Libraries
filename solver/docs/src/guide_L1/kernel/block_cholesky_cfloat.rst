..
   Copyright (C) 2025-2025, Advanced Micro Devices, Inc.
  
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Block_Cholesky_cfloat
   :description: Blocked Cholesky decomposition for cfloat datatype
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

*******************************************************
Block Cholesky CFloat
*******************************************************

Overview
============
Blocked Cholesky decomposition factorizes a Hermitian positive-definite matrix into an upper triangular factor :math:`R`, in the form :math:`A = R^H R`. The implementation processes the input matrix as compile-time sized square blocks to reduce local storage pressure and improve reuse in HLS.

This API supports complex float block Cholesky use cases, including the :math:`256 \times 256` matrix size with a :math:`32 \times 32` block size.

Implementation
==============

DataType Supported
--------------------
* std::complex<float>

.. note::
   The function assumes that the input matrix is Hermitian positive definite. The matrix dimension must be divisible by the block size.

Interfaces
--------------------
* Template parameters:

  * N                  : Input matrix dimension
  * B                  : Block size

* Arguments:

  * A                  : In-place input/output matrix. On exit, the upper triangle contains :math:`R` and the strict lower triangle is cleared.

Implementation Controls
------------------------

Specifications
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code::

    xf::solver::xf_block_cholesky_inplace<N, B>(A);

The template entry point can be instantiated for :math:`N=256` and :math:`B=32` to support :math:`256 \times 256` matrices.

.. code::

    xf::solver::xf_block_cholesky_inplace<256, 32>(A);

The convenience wrapper below instantiates the legacy default test configuration with :math:`N=128` and :math:`B=32`.

.. code::

    xf::solver::xf_block_cholesky_128_inplace(A);
