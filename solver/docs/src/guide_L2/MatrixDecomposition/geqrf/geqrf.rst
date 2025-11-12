..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: GEQRF
   :description: This function solves a system of linear equation with triangular coefficient matrix along with multiple right-hand side vector.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

*******************************************************
General QR Decomposition (GEQRF)
*******************************************************

This function computes QR factorization of matrix :math:`A`

.. math::
            A = Q R

where :math:`A` is a dense matrix of size :math:`m \times n`, :math:`Q` is a :math:`m \times n` matrix with orthonormal columns, and :math:`R` is an
upper triangular matrix.
The maximum matrix size supported in FPGA is templated by NRMAX and NCMAX.
