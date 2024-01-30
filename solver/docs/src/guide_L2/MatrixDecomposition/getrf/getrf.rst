
.. 
   
.. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: GETRF, Decomposition
   :description: This function computes the LU decomposition (with partial pivoting) of matrix.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

*******************************************************
Lower-Upper Decomposition (GETRF)
*******************************************************

This function computes the LU decomposition (with partial pivoting) of matrix :math:`A`

.. math::
    A = L U

where :math:`A` is a dense matrix of size :math:`n \times n`, :math:`L` is a lower triangular matrix with unit diagonal, and :math:`U` is a upper triangular matrix. This function implement partial pivoting.
The maximum matrix size supported in FPGA is templated by NMAX.
