
.. 
   
.. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: TRTRS
   :description: This function solves a system of linear equation with triangular coefficient matrix along with multiple right-hand side vector.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

*********************************************************
Triangular Solver with multiple right-hand sides (TRTRS)
*********************************************************

This function solves a system of linear equation with triangular coefficient matrix along with multiple right-hand side vectors

.. math::
        Ax=B

where :math:`A` is a dense lower or upper triangular matrix of size :math:`m \times m`, :math:`x` is a vector that needs to be computed, and :math:`B` is an input vector.
The maximum matrix size supported in FPGA is templated by NMAX.
