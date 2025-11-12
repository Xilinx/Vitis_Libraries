..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _CHOLESKY:

========
Cholesky
========

This function computes the Cholesky decomposition of matrix :math:`A`

.. math::
    A = L {L}^*

where :math:`A` is a Hermitian positive-definite matrix of size :math:`n \times n`, :math:`L` is a lower triangular matrix with real and positive diagonal entries, and :math:`{L}^*` denotes the conjugate transpose of matrix of :math:`L`.
Every Hermitian positive-definite matrix (and thus also every real-valued symmetric positive-definite matrix) has a unique Cholesky decomposition.

The Cholesky has configurable data types and matrix sizes, along with a configurable number of frames, and support for parallelism.

Entry Point
===========

The graph entry point is the following:

.. code-block::

    xf::solver::cholesky::cholesky_graph

Device Support
==============

The Cholesky library element supports AIE, AIE-ML and AIE-MLv2 devices.

Supported Types
===============
The data type is controlled by ``TT_DATA``, and can be one of 2 choices: float or cfloat.


Template Parameters
===================

To see details on the template parameters for the Cholesky, see :ref:`API_REFERENCE`.


Access Functions
================

To see details on the access functions for the Cholesky, see :ref:`API_REFERENCE`.

Ports
=====

To see details on the ports for the Cholesky, see :ref:`API_REFERENCE`. Note that the type of ports are determined by the configuration of template parameters.


Parallelism (Grid Tiling)
-------------------------

Parallelism for the Cholesky is configured using the ``TP_GRID_DIM`` template parameter. This parameter scales the size of the lower-triangular grid of tiles used to split up the input matrix. Only the lower-triangular tiles are used since the upper-triangular output can be assumed to resolve to 0.
``TP_GRID_DIM`` must be a factor of ``TP_DIM``, and the resulting sub-matrix dimension must be a multiple of ``vecSampleNum``. 
The number of AIE tiles used in the design scales according to ``TP_GRID_DIM`` * (``TP_GRID_DIM`` + 1) / 2.

Following is an example of ``TP_DIM`` and ``TP_GRID_DIM`` being used:

- ``vecSampleNum`` = 2
- ``TP_DIM`` = 6
- ``TP_GRID_DIM`` = 3

.. code-block::

        +-----+
        |0  6 |12 14 20 26
        |1  7 |13 15 21 27 
        +-----+-----+
        |2  8 |14 16|22 28 
        |3  9 |15 17|23 29 
        +-----+-----+-----+
        |4  10|16 18|24 30|
        |5  11|17 19|25 31|
        +-----+-----+-----+

.. note:: The numbers represent sample indices of the global matrix (must be column-major), and the drawn boundaries represent the samples which are processed by each sub-kernel. To minimise tile wastage, tiles whose samples are assumed to entirely resolve to 0 are not hooked up.

Padding
-------

Padding is not a supported feature of the IP. For matrices whose dimensions are not multiples of `vecSampleNum`_, the input matrix must be padded such that it is a multiple of `vecSampleNum`_ (the output locations corresponding to the pads should be ignored). ``TP_DIM`` must be set to the padded dimension size.

Following is an example of a 6*6 matrix with a `vecSampleNum`_  of 4 (thus ``TP_DIM`` set to 8):

.. code-block::

        +----------------------+
        |1  2  3  4  5  6  0  0|
        |2  7  8  9  10 11 0  0|
        |3  8  12 13 14 15 0  0|
        |4  9  13 16 17 18 0  0|
        |5  10 14 17 19 20 0  0|
        |6  11 15 18 20 21 0  0|
        |0  0  0  0  0  0  0  0|
        |0  0  0  0  0  0  0  0|
        +----------------------+

Constraints
-----------
Input matrix data must be written in a column-major fashion, and the matrix is assumed to be Hermetian positive-definite. The cholesky operation is susceptible to catastrophic cancellation, and thus it is encouraged to ensure your matrix is well-conditioned. 
Data is operated on in ``vecSampleNum`` * ``vecSampleNum`` chunks, and thus only chunks along and below the diagonal are operated on. Upper matrix data is assumed to be zero, thus chunks in the upper-triangular output are undefined.
In a single-tile implementation, ``TP_DIM`` must be a multiple of ``vecSampleNum``. In multi-tile implementations, ``TP_DIM`` / ``TP_GRID_DIM`` must be a multiple of ``vecSampleNum``.


Code Example
============
.. literalinclude:: ../../../../L2/examples/docs_examples/test_cholesky.hpp
    :language: cpp
    :lines: 17-

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim: