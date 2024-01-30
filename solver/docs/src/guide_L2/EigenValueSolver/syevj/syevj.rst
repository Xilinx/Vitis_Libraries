
.. 
   
.. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: SYEVJ, Eigenvalue, Solver, Jacobi, Eigen
   :description: Symmetric Matrix Jacobi based Eigen Value Decomposition (SYEVJ).
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


*******************************************************
Eigenvalue Solver (SYEVJ)
*******************************************************

Symmetric Matrix Jacobi based Eigen Value Decomposition (SYEVJ)

.. math::
  A U = U \Sigma

where :math:`A` is a dense symmetric matrix of size :math:`m \times m`, :math:`U` is a :math:`m \times m` matrix with orthonormal columns, each column of U is the eigenvector :math:`v_{i}`, and :math:`\Sigma` is diagonal matrix, which contains the eigenvalues :math:`\lambda_{i}` of matrix A.
The maximum matrix size supported in FPGA is templated by NMAX.
