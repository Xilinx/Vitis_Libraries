.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _cg_kernels:

**************************
CG Kernels 
**************************

Conjugate Gradient solvers are implemented by multiple streaming kernels. 
In this repository, both sparse-matrix based and dense-matrix based solver kernels are provided. To
accelerate the convergence, a popular preconditioner, Jacobi preconditioner, is integrated with the
solver.


Usage and Benchmark
======================================

.. toctree::
   :maxdepth: 2

   GEMV-based Conjugate Gradient Solver with Jacobi Preconditioner <benchmark/cg_gemv_jacobi.rst>
   SPMV-based Conjugate Gradient Solver with Jacobi Preconditioner <benchmark/cg_spmv_jacobi.rst>

