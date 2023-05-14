.. 
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

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
