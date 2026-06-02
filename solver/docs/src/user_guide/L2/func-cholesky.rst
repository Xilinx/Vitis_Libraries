..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _SOLVER_CHOLESKY:

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

    xf::solver::aie::cholesky::cholesky_graph

Device Support
==============

The Cholesky library element supports AIE, AIE-ML, and AIE-MLv2 devices.

Supported Types
===============
The data type is controlled by ``TT_DATA`` and can be one of two choices: ``float`` or ``cfloat``.


Template Parameters
===================

To see details on the template parameters for the Cholesky, see :ref:`SOLVER_API_REFERENCE`.


Access Functions
================

To see details on the access functions for the Cholesky, see :ref:`SOLVER_API_REFERENCE`.

Ports
=====

To see details on the ports for the Cholesky, see :ref:`SOLVER_API_REFERENCE`. Note that the port types are determined by the template parameter configuration.


Cascading (resource tradeoff)
------------------------------------

Pipelining the Cholesky with multiple chained kernels is configured using the ``TP_CASC_LEN`` template parameter. This parameter trades resource (number of tiles used) for throughput performance. It is generally more efficient at increasing throughput than ``TP_GRID_DIM``, however will not reduce latency or increase the size of supported matrices. It can be used in conjunction with ``TP_GRID_DIM`` to increase throughput on larger matrices / lower latency solutions.


Grid Tiling (parallelism)
-------------------------

Parallelism for the Cholesky is configured using the ``TP_GRID_DIM`` template parameter. This parameter scales the size of the lower-triangular grid of tiles used to split up the input matrix, increasing supported matrix size, lowering latency, and increasing throughput. Only the lower-triangular tiles are used since the upper-triangular output can be assumed to resolve to 0.
``TP_GRID_DIM`` must be a factor of ``TP_DIM``, and the resulting sub-matrix dimension must be a multiple of :ref:`SOLVER_vecSampleNum`. The smallest supported matrix size is that which can fit on a sub-matrix (:ref:`SOLVER_vecSampleNum` * :ref:`SOLVER_vecSampleNum`), and the largest is depdendent on local storage and ``TP_GRID_DIM``. For example, a ``TP_DIM`` of 512 with a ``TP_GRID_DIM`` of 8 results in each kernel operating on a 64 * 64 sub-matrix.

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

The number of AIE tiles used in the design scales according to ``TP_GRID_DIM`` * (``TP_GRID_DIM`` + 1) / 2. When used with ``TP_CASC_LEN``, the triangular grid of kernels at a given point in the cascade chain will only be as wide as to accomodate all sub-matrices which are still being operated on, as shown in the following diagram:

.. code-block::

             + ____  ____  ----------+    ↑ 
           / /____ /____ /|        / |    | 
         /  | in  | out | |      /   |    | 
        +-- |_____|_____|/|___ -+    |    | 
        |  /____ /____ /____ /| |    |    | TP_GRID_DIM (Y-axis)   
        | | in  |     | out | | |    |    | 
        | |_____|_____|_____|/|_|_   |    | 
        |/____ /____ /____ /____|/|--+  ↑ ↓       
        ‖ in  |     |     | out ‖ | /  /         
        ‖_____|_____|_____|_____‖//  /  TP_GRID_DIM (X-axis)   
        +-----------------------+  ↓  
        ------------------------→ 
           TP_CASC_LEN (Z-axis)  

.. note:: The diagram above demonstrates a TP_GRID_DIM of 3 and a TP_CASC_LEN of 4. There is a one-to-one correspondence between an in-kernel to the cascade chain and an out-kernel. The other in and out kernels are occluded in this diagram.


Padding
-------

Padding is not a supported feature of the library element. For matrices whose dimensions are not multiples of :ref:`SOLVER_vecSampleNum`, the input matrix must be padded such that it is a multiple of :ref:`SOLVER_vecSampleNum` (the output locations corresponding to the pads should be ignored). ``TP_DIM`` must be set to the padded dimension size.

The following is an example of a 6x6 matrix with :ref:`SOLVER_vecSampleNum` of 4 (thus ``TP_DIM`` set to 8):

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
Input matrix data must be written in a column-major fashion, and the matrix is assumed to be Hermitian positive-definite. The output matrix will be written in column-major fashion. The Cholesky operation is susceptible to catastrophic cancellation; therefore, it is recommended to ensure your matrix is well-conditioned.
Data is operated on in :ref:`SOLVER_vecSampleNum` * :ref:`SOLVER_vecSampleNum` chunks, and thus only chunks along and below the diagonal are operated on. Upper matrix data is assumed to be zero, thus chunks in the upper-triangular output are undefined.
In a single-tile implementation, ``TP_DIM`` must be a multiple of :ref:`SOLVER_vecSampleNum`. In multi-tile implementations, ``TP_DIM`` / ``TP_GRID_DIM`` must be a multiple of :ref:`SOLVER_vecSampleNum`, with a maximum ``TP_DIM`` of 1024. Resulting sub-matrices must fit in local storage.
``TP_CASC_LEN`` must be between 1 and ``TP_DIM``, with a max value of 32.


Code Example
============
.. literalinclude:: ../../../../L2/examples/docs_examples/test_cholesky.hpp
    :language: cpp
    :lines: 17-

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
