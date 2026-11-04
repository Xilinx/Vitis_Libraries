..
   Copyright (C) 2026, Advanced Micro Devices, Inc.

   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _SOLVER_EIGENVALUE:

==========
EIGENVALUE
==========

This function computes the eigenvalues of a square matrix :math:`A` using the QR-iteration algorithm:

.. math::
    A = U \cdot S \cdot V^H \quad \text{(conceptually; computed here via)} \quad A \to H \to \text{eigenvalues on the diagonal}

Concretely, the implementation is a two-stage pipeline:

1. Reduce :math:`A` to upper Hessenberg form :math:`H` using Householder reflections. This is a similarity
   transform (:math:`H = Q^H A Q` for some unitary :math:`Q`), so :math:`H` has the same eigenvalues as :math:`A`,
   and makes the following step far cheaper.
2. Apply iterative QR decomposition to :math:`H` with a simple (Wilkinson-style) shift and deflation, until
   :math:`H` converges to (block-)upper-triangular form. The diagonal entries are the eigenvalues of :math:`A`.

Entry Point
===========

The graph entry point is the following:

.. code-block::

    xf::solver::aie::eigenvalue::eigenvalue_graph

Device Support
==============

The Eigenvalue library element supports AIE, AIE-ML, and AIE-MLv2 devices.

Supported Types
===============
The data type is controlled by ``TT_DATA`` and can be one of two choices: ``float`` or ``cfloat``.

Template Parameters
===================

To see details on the template parameters for the Eigenvalue solver, see :ref:`SOLVER_API_REFERENCE`.

Access Functions
================

To see details on the access functions for the Eigenvalue solver, see :ref:`SOLVER_API_REFERENCE`.

Ports
=====

To see details on the ports for the Eigenvalue solver, see :ref:`SOLVER_API_REFERENCE`.

Numerical Accuracy
------------------

The QR algorithm run in real (``float``) arithmetic converges to a real Schur form, which is only fully
diagonal (i.e. only ever fully deflates every eigenvalue) when all eigenvalues of the input matrix are real.
A matrix with a complex-conjugate eigenvalue pair leaves a 2x2 block on the diagonal that never deflates
within this algorithm. ``TT_DATA=float`` **is therefore only valid for matrices whose eigenvalues are all
real** (for example, symmetric or normal matrices). As with other numerical preconditions in this library
(see, for example, the LS Cholesky Solver's positive-definiteness requirement), this is not checked at
run time, since doing so would require a data-dependent check that would impose a performance penalty; the
user must ensure the input matrix meets this requirement when using ``TT_DATA=float``.

``TT_DATA=cfloat`` has no such restriction: the QR algorithm in complex arithmetic always converges to a
fully diagonal Schur form, so any square complex matrix is supported.

Input Form
----------

The Eigenvalue library element takes a single operand, :math:`A`, a square matrix, which must be presented
in row-major form.

Because the output is a vector of eigenvalues, the concept of row-major/column-major form does not apply
to it.

Padding
-------

Padding is not a supported feature of the library element. ``TP_DIM`` must already be a multiple of
:ref:`SOLVER_vecSampleNum`.

Constraints
-----------
``TP_DIM`` must be a multiple of the internal vector size (8 for ``float``, 4 for ``cfloat``).

``TP_DIM`` must not exceed a maximum driven by per-tile data memory, which varies by ``AIE_VARIANT``
and ``TT_DATA`` (larger on AIE-ML/AIE-MLv2 than on AIE, and larger for ``float`` than ``cfloat``); for
example [8, 48] for ``float`` or [4, 36] for ``cfloat`` on AIE. See ``kMaxDimCols`` in
``eigenvalue_graph.hpp`` for the exact, compile-time-computed value for a given configuration.

``TP_CASC_LEN`` must be 1; support for larger values (splitting the computation across multiple kernels for
larger matrices) is reserved for a future release.

Code Example
============
.. literalinclude:: ../../../../L2/examples/docs_examples/test_eigenvalue.hpp
    :language: cpp
    :lines: 17-

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
