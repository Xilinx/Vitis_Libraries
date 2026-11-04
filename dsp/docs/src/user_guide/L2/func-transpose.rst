..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2026, Advanced Micro Devices, Inc.

   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _DSP_TRANSPOSE:

=========
Transpose
=========

This library element transposes a ``TP_DIM_ROWS`` x ``TP_DIM_COLS`` matrix of ``TT_DATA`` held in
an input iobuffer, producing a ``TP_DIM_COLS`` x ``TP_DIM_ROWS`` output iobuffer, using the
``::aie::transpose()`` AI Engine API intrinsic.
Template parameters are used to configure the top-level graph of the ``transpose_graph`` class.

Entry Point
===========

The graph entry point is as follows:

.. code-block::

    xf::dsp::aie::transpose::transpose_graph

Device Support
==============

Transpose supports AIE, AIE-ML, and AIE-MLv2 devices.

Supported Data Types
====================

``TT_DATA`` may be one of: int16, cint16, int32, cint32, float, cfloat. See table
:ref:`DSP_TRANSPOSE_mem_access_width` for the device memory access width (in samples) per data
type - this is the granularity used both for internal buffer padding and for
``TP_SPLIT_MODE=1`` (aligned) splitting.

.. _DSP_TRANSPOSE_mem_access_width:

.. table:: Memory Access Width (samples) by Data Type and AIE Variant
   :align: center

   +----------+------+--------+----------+
   | TT_DATA  | AIE  | AIE-ML | AIE-MLv2 |
   +==========+======+========+==========+
   | int16    | 16   | 16     | 32       |
   +----------+------+--------+----------+
   | cint16   | 8    | 8      | 16       |
   +----------+------+--------+----------+
   | int32    | 8    | 8      | 16       |
   +----------+------+--------+----------+
   | float    | 8    | 8      | 16       |
   +----------+------+--------+----------+
   | cint32   | 4    | 4      | 8        |
   +----------+------+--------+----------+
   | cfloat   | 4    | 4      | 8        |
   +----------+------+--------+----------+

Template Parameters
===================

To see details on the template parameters for Transpose, see :ref:`DSP_API_REFERENCE`.

Access Functions
================

``transpose_graph`` exposes two static access functions so a user wiring up a multi-lane/
multi-kernel graph knows exactly how the work was divided:

- ``getRowsPerKernel(ssrPos)`` returns the number of rows (equivalently, output columns)
  processed by SSR lane ``ssrPos``.
- ``getColsPerKernel(cascPos)`` returns the number of columns (equivalently, output rows)
  processed by cascade position ``cascPos`` (the split is identical in every SSR lane).

Ports
=====

To see details on the ports for Transpose, see :ref:`DSP_API_REFERENCE`. Note that the number of
input/output ports is ``TP_SSR * TP_CASC_LEN``.

Design Notes
============

Inputs
------

``TP_DIM_ROWS`` and ``TP_DIM_COLS`` may be any positive integer - they do not need to be a
multiple of the memory access width. Internally, each kernel's buffers are zero-padded up to the
device's memory access width (see table :ref:`DSP_TRANSPOSE_mem_access_width`) so that
``::aie::transpose()`` can be applied on whole, aligned tiles; this padding is not visible at the
graph's ports.

Super Sample Rate (SSR)
------------------------

Transpose is split across a ``TP_SSR`` x ``TP_CASC_LEN`` grid of independent kernels:

- ``TP_SSR`` splits ``TP_DIM_ROWS`` across parallel lanes. Lane ``s`` owns a full-width,
  contiguous row-range of the input and produces the corresponding contiguous column-range of
  the output.
- ``TP_CASC_LEN`` splits ``TP_DIM_COLS`` across kernels, within each ``TP_SSR`` lane. Kernel ``c``
  owns a full-height, contiguous column-range of the input and produces the corresponding
  contiguous row-range of the output.

Because transposing swaps rows and columns, kernel ``(s, c)``'s output tile must be routed to
block position ``(c, s)`` of the overall output matrix - the same axis swap that gives a block
matrix ``M = [[A,B],[C,D]]`` the transpose ``M^T = [[A^T,C^T],[B^T,D^T]]``. Routing each kernel's
output port to the correct block position of the final output is the responsibility of the
parent graph/testbench; the IP itself does no on-chip combination across ports (the same
convention used by ``TP_SSR`` in other DSPLib cores, e.g. ``matrix_mult``).

``TP_SPLIT_MODE`` selects how ``TP_DIM_ROWS``/``TP_DIM_COLS`` are divided across
``TP_SSR``/``TP_CASC_LEN`` when they do not divide evenly:

- ``TP_SPLIT_MODE=0`` (equal): as equal as possible, with any remainder distributed one at a
  time to the first lanes/kernels.
- ``TP_SPLIT_MODE=1`` (aligned): every lane/kernel but the last is sized to a multiple of the
  device's memory access width; the last one takes whatever remains (the "runt").

Conjugation
-----------

Setting ``TP_CONJUGATE=1`` conjugates every element during the transpose, with no reduction in
throughput (the conjugation is folded into the same load/store pass that already moves the data).
``TP_CONJUGATE`` requires a complex ``TT_DATA`` type (cint16, cint32 or cfloat).

Constraints
-----------

Each kernel's padded input and output buffers combined must fit within this AIE variant's tile
data memory; raising ``TP_SSR`` and/or ``TP_CASC_LEN`` shrinks the per-kernel buffer to
accommodate larger matrices.

Code Example
============
.. literalinclude:: ../../../../L2/examples/docs_examples/test_transpose.hpp
    :language: cpp
    :lines: 17-

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
