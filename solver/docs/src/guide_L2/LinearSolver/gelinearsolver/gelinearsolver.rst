..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: GELINEARSOLVER
   :description: This function solves a system of linear equation with general matrix along with multiple right-hand side vector.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


*******************************************************
General Linear Solver (GELINEARSOLVER)
*******************************************************

This function solves a system of linear equation with general matrix along with multiple right-hand side vector.

.. math::
      Ax=B

where :math:`A` is a dense general matrix of size :math:`m \times m`, :math:`x` is a vector that needs to be computed, and :math:`B` is an input vector.
The maximum matrix size supported in FPGA is templated by NMAX.

