..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.

   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _SOLVER_SVD:

============================
Singular Value Decomposition
============================

This function computes the Singular Value Decomposition (SVD) of a matrix :math:`M`.

.. math::
    M = U S V^H

where :math:`M` is an input matrix of size :math:`m \times n` (:math:`m` rows, :math:`n` columns), :math:`U` is an :math:`m \times n` matrix with orthonormal columns (the left singular vectors), :math:`S` is a vector of :math:`n` non-negative real singular values sorted in descending order, and :math:`V` is an :math:`n \times n` orthonormal matrix (the right singular vectors). :math:`V^H` denotes the conjugate transpose of :math:`V`.

The SVD library element uses a one-sided Jacobi algorithm with a parallel-pairs schedule. Convergence is controlled by the ``TP_PASSES`` template parameter: each pass performs one complete sweep over all column pairs, and more passes yield a more accurate decomposition. The element supports configurable data types, matrix sizes, number of Jacobi passes, and cascaded multi-kernel execution.

Entry Point
===========

The graph entry point is the following:

.. code-block:: cpp

    xf::solver::aie::svd::svd_graph

Device Support
==============

The SVD library element supports AIE, AIE-ML, and AIE-MLv2 devices.

Supported Types
===============

The data type is controlled by ``TT_DATA`` and can be one of two choices: ``float`` or ``cfloat``.

Template Parameters
===================

To see details on the template parameters for the SVD, see :ref:`SOLVER_API_REFERENCE`.

Access Functions
================

To see details on the access functions for the SVD, see :ref:`SOLVER_API_REFERENCE`.

Ports
=====

To see details on the ports for the SVD, see :ref:`SOLVER_API_REFERENCE`. Note that the port types are determined by the template parameter configuration.

The SVD graph exposes the following port arrays, each of size ``TP_CASC_LEN``:

- **in[TP_CASC_LEN]** — Input matrix partitions (one per cascade kernel). Each kernel receives a distinct contiguous block of rows of the input matrix, in column-major order.
- **outU[TP_CASC_LEN]** — Left singular vector matrix partitions. Each kernel outputs the rows of ``U`` corresponding to its input row partition.
- **outS[TP_CASC_LEN]** — Singular value vector. Each kernel outputs the full ``S`` vector (``TP_DIM_COLS`` values), sorted in descending order. All kernels output identical data; only one port needs to be consumed.
- **outV[TP_CASC_LEN]** — Right singular vector matrix. Each kernel outputs the full ``V`` matrix (``TP_DIM_COLS`` × ``TP_DIM_COLS`` elements). All kernels output identical data; only one port needs to be consumed.

Design Notes
============

Algorithm: One-Sided Jacobi with Parallel Pairs
-------------------------------------------------

The SVD is computed using the one-sided Jacobi algorithm. At each iteration, a Givens rotation is applied to a pair of columns of the working matrix to reduce their inner product towards zero. After convergence, the column norms of the working matrix are the singular values, and the accumulated rotation matrices form ``U`` and ``V``.

To maximize throughput, the implementation uses a *parallel-pairs* schedule. In each *set*, the full column set is partitioned into :math:`\lfloor n/2 \rfloor` independent column pairs that can be processed simultaneously. A complete *pass* consists of :math:`n-1` such sets that together cover all :math:`n(n-1)/2` distinct pairs. The number of passes is configured by ``TP_PASSES``.

.. note::

   Increasing ``TP_PASSES`` improves convergence accuracy at the cost of additional compute cycles. For well-conditioned matrices, 3–5 passes typically suffice. For ill-conditioned matrices, more passes may be required.

Convergence and Sorting
-----------------------

Singular values in the output ``S`` vector are always sorted in descending order. The corresponding columns of ``U`` and ``V`` are permuted consistently with the sort.

The quality of convergence depends on ``TP_PASSES``. The off-diagonal column inner products are driven towards zero over successive passes; the orthonormality of ``U`` improves with each additional pass, while ``V`` remains orthonormal by construction throughout.

Cascading (Resource Tradeoff)
------------------------------

The cascade length is configured using the ``TP_CASC_LEN`` template parameter. When ``TP_CASC_LEN`` > 1, the ``TP_DIM_ROWS`` rows of the input matrix are partitioned evenly across ``TP_CASC_LEN`` AIE kernels, each processing ``TP_DIM_ROWS / TP_CASC_LEN`` rows. The kernels communicate via inter-kernel streams to accumulate the global inner products required for the Givens rotation computation.

The following illustrates an example with ``TP_DIM_ROWS`` = 8 and ``TP_CASC_LEN`` = 2:

.. code-block:: text

    Input M (8 × 4, column-major):
    +---+---+---+---+
    | 0 | 8 |16 |24 |   ← Kernel 0 (rows 0–3)
    | 1 | 9 |17 |25 |
    | 2 |10 |18 |26 |
    | 3 |11 |19 |27 |
    +---+---+---+---+
    | 4 |12 |20 |28 |   ← Kernel 1 (rows 4–7)
    | 5 |13 |21 |29 |
    | 6 |14 |22 |30 |
    | 7 |15 |23 |31 |
    +---+---+---+---+

Each kernel maintains its own partition of ``U`` and a full copy of ``V``. After all Jacobi passes complete, the kernels accumulate column norms via the cascade streams to compute the singular values, which are then broadcast back so that all kernels can output identical ``S`` and ``V`` values.

Cascading increases the maximum supported matrix size and can improve throughput by distributing the per-kernel data memory footprint. It does not reduce latency.

.. note::

   ``TP_DIM_ROWS`` must be divisible by ``TP_CASC_LEN``, and ``TP_DIM_ROWS / TP_CASC_LEN`` must be a multiple of :ref:`SOLVER_vecSampleNum`.

Column Padding (Internal)
--------------------------

The SVD library element handles arbitrary ``TP_DIM_COLS`` values transparently, including odd values and values that are not multiples of the internal SIMD vector width. Internally, the column count is padded to the next even integer (for the Jacobi pair schedule) and then to the next multiple of the internal vector width (for SIMD-aligned storage of ``V``). This padding is fully transparent to the user: input and output port sizes are always in terms of the true, unpadded ``TP_DIM_COLS``.

Constraints
-----------

- ``TT_DATA`` must be ``float`` or ``cfloat``.
- ``TP_DIM_COLS`` must be in the range [2, 128].
- ``TP_PASSES`` must be in the range [1, 10].
- ``TP_CASC_LEN`` must be in the range [1, 16].
- ``TP_DIM_ROWS`` must be at least :ref:`SOLVER_vecSampleNum`.
- ``TP_DIM_ROWS`` must be a multiple of the internal vector width.
- ``TP_DIM_ROWS`` must be divisible by ``TP_CASC_LEN``.
- ``TP_DIM_ROWS / TP_CASC_LEN`` must be a multiple of the internal vector width.
- The per-kernel data memory footprint (input partition, ``U`` partition, ``V``, ``S``, and scratch) must fit within the AIE tile data memory. Reduce ``TP_DIM_ROWS``, ``TP_DIM_COLS``, or increase ``TP_CASC_LEN`` if this constraint is violated.
- Input and output data are in column-major order.

Code Example
============

.. literalinclude:: ../../../../L2/examples/docs_examples/test_svd.hpp
    :language: cpp
    :lines: 17-

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
