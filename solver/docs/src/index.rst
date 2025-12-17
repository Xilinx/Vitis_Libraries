..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis, Solver, Library, Vitis Solver Library, overview, matrix, linear, eigenvalue
   :description: Vitis Solver Library provides a collection of matrix decomposition operations, linear solvers and eigenvalue solvers.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

====================
Vitis Solver Library
====================

AMD Vitis |trade| Solver Library (SolverLib) provides a collection of matrix decomposition operations, linear solvers, and eigenvalue solvers on PL and AI Engine.

The SolverLib contains:

- :ref:`SOLVER_INTRODUCTION_PL`.

- :ref:`SOLVER_INTRODUCTION_AIE`.

.. _SOLVER_INTRODUCTION_PL:

PL Solver library
=================

Currently, the Vitis PL Solver library includes the following operations for dense matrix:

* Matrix decomposition
   * Cholesky decomposition for symmetric positive definite matrix
   * LU decomposition without pivoting and with partial pivoting
   * QR decomposition for general matrix
   * SVD decomposition (single value decomposition) for symmetric matrix and non-symmetric matrix (Jacobi method)

* Linear solver
   * Tridiagonal linear solver (Parallel cyclic reduction method)
   * Linear solver for triangular matrix
   * Linear solver for symmetric and non-symmetric matrix
   * Matrix inverse for symmetric and non-symmetric matrix

* Eigenvalue solver
   * Jacobi eigenvalue solver for symmetric matrix

.. _SOLVER_INTRODUCTION_AIE:

AI Engine Solver Library
========================

Currently, the Vitis AIE Solver Library provides the following operations on AI Engine.
The AMD Vitis AI Engine Solver library encapsulates several solver algorithms, optimized to take full advantage of the processing power of AMD Versal |trade| Adaptive SoC devices, which contain an array of AI Engines.

* Matrix decomposition
   * Cholesky decomposition for symmetric positive definite matrix
   * QR decomposition (Gram-Schmidt method)

.. toctree::
   :caption: Introduction
   :maxdepth: 2

   Overview <overview.rst>
   Release Note <release.rst>

.. toctree::
   :caption: PL Solver Library User Guide
   :maxdepth: 2

   Vitis Solver Library Tutorial <tutorial.rst>
   L1 PL User Guide <guide_L1/L1.rst>
   L2 PL User Guide <guide_L2/L2.rst>
   Benchmark <benchmark.rst>

.. toctree::
   :caption: AIE Solver Library User Guide
   :maxdepth: 2

   Introduction <user_guide/L2/introduction.rst>
   Solver Library Functions <user_guide/L2/solver-lib-func.rst>
   Compiling and Simulating <user_guide/L2/compiling-and-simulating.rst>
   Benchmark/QoR <user_guide/L2/benchmark.rst>

.. toctree::
   :caption: API Reference

   API Reference Overview <user_guide/L2/api-reference.rst>
   Cholesky <rst/group_cholesky.rst>
   QRD <rst/group_qrd_graph.rst>


.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim: