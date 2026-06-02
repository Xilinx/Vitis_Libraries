..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.

   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _SOLVER_QRD_HH:

=======================================
QR Decomposition (Householder)
=======================================

This function computes the QR decomposition of matrix :math:`A` using Householder reflections. The QR decomposition is defined as:

.. math::

    A = Q R

where :math:`A` is the input matrix of size :math:`m \times n` (:math:`m \geq n`). :math:`Q` is an orthonormal matrix of size :math:`m \times n`, and :math:`R` is an upper-triangular matrix of size :math:`n \times n`.

Each Householder reflector has the form :math:`H = I - \beta v v^*`, where :math:`v` is the Householder vector and :math:`\beta = \frac{2}{v^* v}`. Applying a sequence of these reflectors zeros out the sub-diagonal entries of :math:`A` column by column, yielding :math:`R`. The accumulated reflectors produce :math:`Q`.

The QRD-HH library element has configurable data types and matrix sizes, a configurable number of frames, and support for a cascaded kernel implementation to scale to larger matrix dimensions.


Entry Point
===========

The graph entry point is the following:

.. code-block::

    xf::solver::aie::qrd_hh::qrd_hh_graph

Device Support
==============

The QRD-HH library element supports AIE, AIE-ML, and AIE-MLv2 devices.

Supported Types
===============

The data type is controlled by ``TT_DATA`` and can be one of two choices: ``float`` or ``cfloat``.


Template Parameters
===================

To see details on the template parameters for the QRD-HH, see :ref:`SOLVER_API_REFERENCE`.


Access Functions
================

To see details on the access functions for the QRD-HH, see :ref:`SOLVER_API_REFERENCE`.

Ports
=====

To see details on the ports for the QRD-HH, see :ref:`SOLVER_API_REFERENCE`. Note that the port types are determined by the template parameter configuration.

Design Notes
============

Cascaded Implementation for Larger Matrices
--------------------------------------------

The Householder algorithm applies reflectors sequentially across the columns of :math:`A`. To distribute this work across multiple AIE tiles, ``TP_CASC_LEN`` partitions the input matrix **by rows**. Each kernel in the cascade processes an equal-sized row block of :math:`\frac{m}{TP\_CASC\_LEN}` rows across all :math:`n` columns.

**R matrix output** is produced only by the kernels whose row range overlaps with the upper :math:`n` rows of :math:`A` (i.e., kernels for which the starting row index is less than ``TP_DIM_COLS``). For a rectangular matrix where :math:`m > n`, the bottom kernel(s) whose row range lies entirely below row :math:`n` do not output :math:`R`. This is determined automatically by the graph; no user configuration is required.

**Example:** A :math:`24 \times 16` matrix with ``TP_CASC_LEN = 3``:

- Each kernel handles an :math:`8 \times 16` row partition.
- :math:`R` is :math:`16 \times 16`, so only the top two kernels (rows 0–15) produce :math:`R` output.
- The bottom kernel (rows 16–23) does not drive ``outR``.
- Streams flow from the bottom kernel upward through the chain.

.. table:: Cascade Kernel Role Summary (24×16, TP_CASC_LEN=3)
    :align: center

    +------------------+------------+------------+
    | Kernel (cascPos) | Row Range  | R Output   | 
    +==================+============+============+
    | 0 (top)          | 0  – 7     | yes        |
    +------------------+------------+------------+
    | 1 (middle)       | 8  – 15    | yes        |
    +------------------+------------+------------+
    | 2 (bottom)       | 16 – 23    | no         |
    +------------------+------------+------------+


Data Layout and Row-Major Support
----------------------------------

By default, input and output buffers use **column-major** layout. Row-major layout can be enabled independently for each buffer via the tiling template parameters:

- ``TP_DIM_A_LEADING = 1`` — input matrix :math:`A` is supplied in row-major order.
- ``TP_DIM_Q_LEADING = 1`` — output matrix :math:`Q` is returned in row-major order.
- ``TP_DIM_R_LEADING = 1`` — output matrix :math:`R` is returned in row-major order.

Set these to ``0`` (the default) for column-major operation.

.. note::
   Row-major tiling for ``cfloat`` data is not supported on AIE devices. For ``cfloat`` on AIE, all leading-dimension parameters must be set to ``0``.


Padding
-------

Padding is not a supported feature of the library element. ``TP_DIM_ROWS`` and ``TP_DIM_COLS`` must each be set to a multiple of :ref:`SOLVER_vecSampleNum`. If the natural matrix dimensions are not multiples of this value, the input data must be zero-padded and ``TP_DIM_ROWS`` / ``TP_DIM_COLS`` set to the padded sizes. The corresponding rows and columns in the :math:`Q` and :math:`R` outputs should be ignored.


Code Example
============
.. literalinclude:: ../../../../L2/examples/docs_examples/test_qrd_hh.hpp
    :language: cpp
    :lines: 17-

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
