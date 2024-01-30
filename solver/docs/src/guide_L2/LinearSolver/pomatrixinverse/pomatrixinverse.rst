
.. 
   
.. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: POMATRIXINVERSE
   :description: This function computes the inverse matrix of math:A
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

*******************************************************
Symmetric Matrix Inverse (POMATRIXINVERSE)
*******************************************************

This function computes the inverse matrix of :math:`A`

.. math::
        {A}^{-1}

where :math:`A` is a dense symmetric positive-definite matrix of size :math:`m \times m`.
The maximum matrix size supported in FPGA is templated by NMAX.
