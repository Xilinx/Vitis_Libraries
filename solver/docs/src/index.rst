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
   :keywords: Vitis, Solver, Library, Vitis Solver Library, overview, matrix, linear, eigenvalue
   :description: Vitis Solver Library provides a collection of matrix decomposition operations, linear solvers and eigenvalue solvers.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

Vitis Solver Library
====================

AMD Vitis Solver Library provides a collection of matrix decomposition operations, linear solvers and eigenvalue solvers on PL and AI Engine. Users can see it as containing two sub libraries:

PL Solver library
-----------------

Currently the AMD Vitis PL Solver library includes the following operations for dense matrix
 
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

Currently the AMD Vitis AIE Solver Library provides the following operations on AI Engine.

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


