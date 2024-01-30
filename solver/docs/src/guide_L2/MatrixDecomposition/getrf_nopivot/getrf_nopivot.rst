
.. 
   
.. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: GETRF_NOPIVOT, Decomposition
   :description: This function computes the LU decomposition (without pivoting) of matrix.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

*******************************************************
Lower-Upper Decomposition (GETRF_NOPIVOT)
*******************************************************

This function computes the LU decomposition (without pivoting) of matrix :math:`A`

.. math::
    A = L U

where :math:`A` is a dense matrix of size :math:`m \times m`, :math:`L` is a lower triangular matrix with unit diagonal, and :math:`U` is a upper triangular matrix. This function does not implement pivoting.
The maximum matrix size supported in FPGA is templated by NMAX.
