
.. 
   
.. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: POTRF, Decomposition, Cholesky, SPD, matrix
   :description: This function computes the Cholesky decomposition of matrix.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

**********************************************
Cholesky Decomposition for SPD matrix (POTRF)
**********************************************

This function computes the Cholesky decomposition of matrix :math:`A`

.. math::
    A = L {L}^T

where :math:`A` is a dense symmetric positive-definite matrix of size :math:`m \times m`, :math:`L` is a lower triangular matrix, and :math:`{L}^T` is the transposed matrix of :math:`L`.
The maximum matrix size supported in FPGA is templated by NMAX.
