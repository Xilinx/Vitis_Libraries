..
   Copyright (C) 2026, Advanced Micro Devices, Inc.

   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _SOLVER_MP_PSEUDOINVERSE_SVD:

====================
MP_PSEUDOINVERSE_SVD
====================

This function computes the Moore-Penrose pseudoinverse :math:`A^+` of a matrix :math:`A` via Singular Value Decomposition (SVD):

.. math::
    A = U S V^H \quad \Rightarrow \quad A^+ = V S^+ U^H

where :math:`A` is an input matrix of size :math:`m \times n` (:math:`m` rows, :math:`n` columns), and :math:`S^+` is formed by replacing each non-zero singular value :math:`s_i` in :math:`S` by :math:`1/s_i`.

The MP Pseudoinverse (via SVD) library element is a three-stage pipeline built from existing SolverLib elements: an internal :ref:`SOLVER_SVD` graph decomposes :math:`A` into :math:`U`, :math:`S`, and :math:`V`; a fused kernel scales each column of :math:`V` by the reciprocal of the corresponding singular value to form :math:`T = V S^+`; and a matrix multiply forms :math:`A^+ = T U^H`. Because :math:`S^+` is diagonal, scaling the columns of :math:`V` directly is mathematically equivalent to, and far cheaper than, multiplying :math:`V` by an explicit diagonal matrix.

Entry Point
===========

The graph entry point is the following:

.. code-block::

    xf::solver::aie::mp_pseudoinverse_svd::mp_pseudoinverse_svd_graph

Device Support
==============

The MP Pseudoinverse (via SVD) library element supports AIE, AIE-ML, and AIE-MLv2 devices.

Supported Types
===============
The data type is controlled by ``TT_DATA`` and can be one of two choices: ``float`` or ``cfloat``.

Template Parameters
===================

To see details on the template parameters for the MP Pseudoinverse (via SVD), see :ref:`SOLVER_API_REFERENCE`.

Access Functions
================

To see details on the access functions for the MP Pseudoinverse (via SVD), see :ref:`SOLVER_API_REFERENCE`.

Ports
=====

To see details on the ports for the MP Pseudoinverse (via SVD), see :ref:`SOLVER_API_REFERENCE`.

Numerical Accuracy
------------------

The accuracy of :math:`A^+` depends on the accuracy of the internal SVD, which is controlled by the ``TP_PASSES`` template parameter: more passes yield a more accurate decomposition, in particular improving the orthonormality of :math:`U`, at the cost of higher latency.

Singular values are inverted using a zero-guard: any singular value at or below a fixed threshold (:math:`1 \times 10^{-30}`) is treated as zero rather than inverted. This produces a truncated pseudoinverse for rank-deficient or near-singular input matrices, rather than amplifying noise by dividing by a near-zero value. As with other numerical preconditions in this library (see, for example, the LS Cholesky Solver's positive-definiteness requirement), this threshold is fixed and not user-configurable.

Input Form
----------

The MP Pseudoinverse (via SVD) library element takes a single operand, :math:`A`, which must be presented in column-major form. The output :math:`A^+` is also produced in column-major form.

Padding
-------

The output buffer holds ``TP_DIM_COLS`` (rounded up to an even number, then up to a multiple of the store alignment) rows by ``TP_DIM_ROWS`` columns. When ``TP_DIM_COLS`` already meets this alignment, every sample in the output buffer is valid. Otherwise, the additional rows beyond ``TP_DIM_COLS`` are padding and must be discarded by the user.

Padding of the input is also a supported technique for solving a matrix whose true row count :math:`m_0` is not a multiple of :ref:`SOLVER_vecSampleNum`. The
customer may append :math:`(TP\_DIM\_ROWS - m_0)` all-zero rows to the bottom of the matrix so that its row count equals ``TP_DIM_ROWS``, a multiple of
:ref:`SOLVER_vecSampleNum`. Unlike the LS Cholesky Solver, where pad values are don't-cares, the padding rows here **must be exactly zero**. The internal
SVD stage forms dot products across whole columns, so a non-zero pad value would alter those dot products and corrupt the result for the real
:math:`m_0 \times n` sub-matrix, whereas an all-zero row contributes nothing to any column dot product.

The following is an example of a matrix with a true row count :math:`m_0` of 7, and :ref:`SOLVER_vecSampleNum` of 4 (thus ``TP_DIM_ROWS`` set to 8):

.. code-block::

        +---------------------+
        |a11 a12 a13 a14 a15  |
        |a21 a22 a23 a24 a25  |
        |a31 a32 a33 a34 a35  |
        |a41 a42 a43 a44 a45  |
        |a51 a52 a53 a54 a55  |
        |a61 a62 a63 a64 a65  |
        |a71 a72 a73 a74 a75  |
        |0   0   0   0   0    |
        +---------------------+

With all-zero row padding, :math:`A^H A` is unchanged by the pad rows, so the computed :math:`V` and singular values :math:`S` are identical to those of the
true :math:`m_0 \times n` matrix, and the padded rows of :math:`U` come out as exactly zero. Consequently, in the output :math:`A^+`, the columns beyond the
true :math:`m_0` (that is, the columns corresponding to the padding rows of the input) are also exactly zero, and the leading :math:`n \times m_0` block of
:math:`A^+` is the correct pseudoinverse of the original, unpadded matrix.

Constraints
-----------
``TP_CASC_LEN`` must be 1; support for larger values is reserved for a future release.

``TP_DIM_COLS`` must be even and in the range [2, 128].

``TP_DIM_ROWS`` must be a multiple of :ref:`SOLVER_vecSampleNum` and must fit within one tile's data memory with ``TP_CASC_LEN`` = 1.

``TP_PASSES`` must be in the range [1, 10].

Code Example
============
.. literalinclude:: ../../../../L2/examples/docs_examples/test_mp_pseudoinverse.hpp
    :language: cpp
    :lines: 17-

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
