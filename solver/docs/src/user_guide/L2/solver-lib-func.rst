..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _SOLVER_SOLVER_LIB_FUNC:

========================
Solver Library Functions
========================

The AMD Vitis |trade| Solver library (SolverLib) is a configurable open-source library of linear-algebra elements for developing applications on AMD Versal |trade| AI Engines. The entry point for each function is an L2 graph. Each entry-point graph class contains one or more L1 kernels and can include one or more graph objects. Direct use of L1 kernel classes or any graph class not identified as an entry point is not recommended, as this might bypass legality checking.

The SolverLib consists of the following solver elements:

.. toctree::
   :maxdepth: 2

   Cholesky <func-cholesky.rst>
   QRD <func-qrd.rst>

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:


.. _SOLVER_vecSampleNum:

============
vecSampleNum
============

The number of samples per vector is given by:

.. math::

    \text{vecSampleNum} = \frac{\text{vector size (bytes)}}{\text{sizeof(TT_DATA)}}

Consider an example where the data type is float (4 bytes). On AIE1 devices, the vector size is 32 bytes; thus, ``vecSampleNum`` is 32 / 4 = 8. On AIE-ML and AIE-MLv2 devices, the vector size is 64 bytes; thus, ``vecSampleNum`` is 64 / 4 = 16.
