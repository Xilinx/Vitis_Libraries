.. 
   
.. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Matrix, Decomposition, Linear, Solver, Eigenvalue
   :description: Vitis Solver library L2 application programming interface reference.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

***************************
Supported Numerical Methods
***************************

Matrix Decomposition
====================

.. toctree::
   :maxdepth: 1

   Singular Value Decomposition for symmetric matrix (GESVDJ) <MatrixDecomposition/gesvdj/gesvdj.rst>
   Singular Value Decomposition for general matrix (GESVJ) <MatrixDecomposition/gesvj/gesvj.rst>
   General QR Decomposition (GEQRF) <MatrixDecomposition/geqrf/geqrf.rst>
   Lower-Upper Decomposition (GETRF) <MatrixDecomposition/getrf/getrf.rst>
   Lower-Upper Decomposition (GETRF_NOPIVOT) <MatrixDecomposition/getrf_nopivot/getrf_nopivot.rst>
   Cholesky Decomposition for SPD matrix (POTRF) <MatrixDecomposition/potrf/potrf.rst>

Linear Solver
=============

.. toctree::
   :maxdepth: 2

   Triangular Solver (GTSV) <LinearSolver/gtsv/gtsv.rst>
   Symmetric Linear Solver (POLINEARSOLVER) <LinearSolver/polinearsolver/polinearsolver.rst>
   Symmetric Matrix Inverse (POMATRIXINVERSE) <LinearSolver/pomatrixinverse/pomatrixinverse.rst>
   General Linear Solver (GELINEARSOLVER) <LinearSolver/gelinearsolver/gelinearsolver.rst>
   General Matrix Inverse (GEMATRIXINVERSE) <LinearSolver/gematrixinverse/gematrixinverse.rst>
   Triangular Solver with multiple right-hand sides (TRTRS) <LinearSolver/trtrs/trtrs.rst>

Eigenvalue Solver
==================

.. toctree::
   :maxdepth: 2

   Eigenvalue Solver (SYEVJ) <EigenValueSolver/syevj/syevj.rst>
