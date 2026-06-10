..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2026, Advanced Micro Devices, Inc.

   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _SOLVER_SVD:

============================
Singular Value Decomposition
============================

This function computes the Singular Value Decomposition (SVD) of a matrix :math:`M`.

.. math::
    M = U S V^H

where :math:`M` is an input matrix of size :math:`m \times n` (:math:`m` rows, :math:`n` columns), :math:`U` is an :math:`m \times n` matrix with orthonormal columns (the left singular vectors), :math:`S` is a vector of :math:`n` non-negative real singular values sorted in descending order, and :math:`V` is an :math:`n \times n` orthonormal matrix (the right singular vectors). :math:`V^H` denotes the conjugate transpose of :math:`V`.

The SVD library element uses a one-sided Jacobi algorithm. Convergence is controlled by the ``TP_PASSES`` template parameter: each pass performs one complete sweep over all column pairs, and more passes yield a more accurate decomposition. The element supports configurable data types, matrix sizes, and cascaded multi-kernel execution.

Entry Point
===========

The graph entry point is the following:

.. code-block::

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

Design Notes
============

Choosing TP_PASSES
------------------

``TP_PASSES`` directly controls the accuracy of the decomposition. As a guide:

- **3-5 passes** typically suffice for well-conditioned matrices.
- **More passes** may be needed for ill-conditioned matrices or when high accuracy of ``U`` is required.
- **Fewer passes** reduce latency and resource use at the cost of accuracy.

Singular values in the output ``S`` vector are sorted in descending order, with the columns of ``U`` and ``V`` permuted consistently. ``V`` is orthonormal by construction regardless of pass count; the orthonormality of ``U`` improves with each additional pass.

Cascading (Resource Tradeoff)
------------------------------

When ``TP_CASC_LEN`` > 1, the ``TP_DIM_ROWS`` rows of the input matrix are partitioned evenly across ``TP_CASC_LEN`` AIE kernels, each processing ``TP_DIM_ROWS / TP_CASC_LEN`` rows. The kernels communicate via inter-kernel streams to coordinate the decomposition across partitions.

The following illustrates an example with ``TP_DIM_ROWS`` = 8 and ``TP_CASC_LEN`` = 2:

.. code-block:: text

    Input M (8 x 4, column-major):
    +---+---+---+---+
    | 0 | 8 |16 |24 |   <- in[0]: rows 0-3
    | 1 | 9 |17 |25 |
    | 2 |10 |18 |26 |
    | 3 |11 |19 |27 |
    +---+---+---+---+
    | 4 |12 |20 |28 |   <- in[1]: rows 4-7
    | 5 |13 |21 |29 |
    | 6 |14 |22 |30 |
    | 7 |15 |23 |31 |
    +---+---+---+---+

Cascading increases the maximum supported matrix size by distributing the per-kernel data memory footprint. It does not reduce latency.

Output Buffers and Port Connectivity
--------------------------------------

Each ``outU[k]`` port carries a unique row partition of ``U`` and must be consumed independently. The buffer holds ``TP_DIM_ROWS / TP_CASC_LEN`` rows; the column stride is padded beyond ``TP_DIM_COLS`` for alignment. Only the first ``TP_DIM_COLS`` columns of each row contain valid data.

All ``outS`` ports carry identical data and the buffer holds ``TP_DIM_COLS`` valid singular values followed by padding. Only one port needs to be consumed, but all must be connected.

All ``outV`` ports carry identical data. The valid ``V`` matrix occupies the first ``TP_DIM_COLS`` x ``TP_DIM_COLS`` elements of the buffer; any remaining elements should be discarded. Only one port needs to be consumed, but all must be connected.

Constraints
===========

Input matrix data must be provided in column-major order. ``TP_DIM_ROWS`` must be a multiple of :ref:`SOLVER_vecSampleNum` and divisible by ``TP_CASC_LEN``; ``TP_DIM_ROWS / TP_CASC_LEN`` must also be a multiple of :ref:`SOLVER_vecSampleNum`. Not all combinations of ``TP_DIM_ROWS``, ``TP_DIM_COLS``, and ``TP_CASC_LEN`` are valid; the per-kernel data memory footprint must fit within the AIE tile data memory. If this constraint is violated, increase ``TP_CASC_LEN`` or reduce ``TP_DIM_ROWS`` or ``TP_DIM_COLS``.

Code Example
============

.. literalinclude:: ../../../../L2/examples/docs_examples/test_svd.hpp
    :language: cpp
    :lines: 17-

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
