.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis, Solver, Library, Vitis Solver Library, overview, matrix, linear, eigenvalue
   :description: Vitis Solver Library provides a collection of matrix decomposition operations, linear solvers and eigenvalue solvers.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

Vitis Solver Library
====================

AMD Vitis |trade| Solver Library provides a collection of matrix decomposition operations, linear solvers, and eigenvalue solvers on PL and AI Engine. You can see it as containing two sub libraries:

PL Solver library
-----------------

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


AI Engine Solver library
------------

Currently, the Vitis AIE Solver Library provides the following operations on AI Engine.

* Matrix decomposition
   * Cholesky decomposition for symmetric positive definite matrix
   * QR decomposition for general matrix


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

   L2 AIE User Guide <guide_L2_AIE/L2_AIE.rst>


.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim: