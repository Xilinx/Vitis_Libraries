..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _SOLVER_LIB_FUNC:

========================
Solver Library Functions
========================

The AMD Vitis |trade| solver library (SolverLib) is a configurable library of elements that can be used to develop applications on AMD Versal |trade| AI Engines. This is an Open Source library for linear algebra algorithms. The user entry point for each function in this library is a graph (L2 level). Each entry point graph class will contain one or more L1 level kernels and can contain one or more graph objects. Direct use of kernel classes (L1 level) or any other graph class not identified as an entry point is not recommended as this might bypass legality checking.

The SolverLib consists of the following solver elements:

.. toctree::
   :maxdepth: 2

   Cholesky <func-cholesky.rst>
   QRD <func-qrd.rst>

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:


.. _vecSampleNum:

==================
Vector Granularity
==================

Consider an example where the data type is float (4 bytes). On AIE1 devices, the vector size is 32 bytes, thus `vecSampleNum`_ is 32 / 4 = 8. On AIE-ML and AIE-MLv2 devices, the vector size is 64 bytes, thus `vecSampleNum`_ is 64 / 4 = 16

**vecSampleNum**
    The number of samples per vector, given by:
.. math::

    \text{vecSampleNum} = \frac{\text{vector size (bytes)}}{\text{sizeof(TT_DATA)}}