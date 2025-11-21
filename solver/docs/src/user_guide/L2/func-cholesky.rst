..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _CHOLESKY:

========
Cholesky
========

This function computes the Cholesky decomposition of matrix :math:`A`.

.. math::
    A = L {L}^*

where :math:`A` is a Hermitian positive-definite matrix of size :math:`n \times n`, :math:`L` is a lower triangular matrix with real and positive diagonal entries, and :math:`{L}^*` denotes the conjugate transpose of the matrix :math:`L`.
Every Hermitian positive-definite matrix (and thus also every real-valued symmetric positive-definite matrix) has a unique Cholesky decomposition.

The Cholesky library element has configurable data types and matrix sizes, a configurable number of frames, and support for parallelism.

Entry Point
===========

The graph entry point is the following:

.. code-block::

    xf::solver::cholesky::cholesky_graph

Device Support
==============

The Cholesky library element supports AIE, AIE-ML, and AIE-MLv2 devices.

Supported Types
===============
The data type is controlled by ``TT_DATA`` and can be one of two choices: ``float`` or ``cfloat``.


Template Parameters
===================

To see details on the template parameters for the Cholesky, see :ref:`API_REFERENCE`.


Access Functions
================

To see details on the access functions for the Cholesky, see :ref:`API_REFERENCE`.

Ports
=====

To see details on the ports for the Cholesky, see :ref:`API_REFERENCE`. Note that the port types are determined by the template parameter configuration.


Parallelism (Grid Tiling)
-------------------------

Parallelism for the Cholesky is configured using the ``TP_GRID_DIM`` template parameter. This parameter scales the size of the lower-triangular grid of tiles used to split up the input matrix. Only the lower-triangular tiles are used since the upper-triangular output can be assumed to resolve to 0.
``TP_GRID_DIM`` must be a factor of ``TP_DIM``, and the resulting sub-matrix dimension must be a multiple of :ref:`vecSampleNum`. 
The number of AIE tiles used in the design scales according to ``TP_GRID_DIM`` * (``TP_GRID_DIM`` + 1) / 2.

The following is an example of ``TP_DIM`` and ``TP_GRID_DIM`` being used:

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

.. note:: The numbers represent sample indices of the global matrix (must be column-major), and the drawn boundaries represent the samples processed by each sub-kernel. To minimize tile wastage, tiles whose samples are assumed to entirely resolve to 0 are not hooked up.

Padding
-------

Padding is not a supported feature of the library element. For matrices whose dimensions are not multiples of :ref:`vecSampleNum`, the input matrix must be padded such that it is a multiple of :ref:`vecSampleNum` (the output locations corresponding to the pads should be ignored). ``TP_DIM`` must be set to the padded dimension size.

The following is an example of a 6x6 matrix with :ref:`vecSampleNum` of 4 (thus ``TP_DIM`` set to 8):

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
Input matrix data must be written in a column-major fashion, and the matrix is assumed to be Hermitian positive-definite. The Cholesky operation is susceptible to catastrophic cancellation; therefore, it is recommended to ensure your matrix is well-conditioned.
Data is operated on in :ref:`vecSampleNum` * :ref:`vecSampleNum` chunks, and thus only chunks along and below the diagonal are operated on. Upper matrix data is assumed to be zero, thus chunks in the upper-triangular output are undefined.
In a single-tile implementation, ``TP_DIM`` must be a multiple of :ref:`vecSampleNum`. In multi-tile implementations, ``TP_DIM`` / ``TP_GRID_DIM`` must be a multiple of :ref:`vecSampleNum`.


Code Example
============
.. literalinclude:: ../../../../L2/examples/docs_examples/test_cholesky.hpp
    :language: cpp
    :lines: 17-

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
