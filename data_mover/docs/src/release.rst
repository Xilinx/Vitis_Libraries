.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: BLAS, Library, Vitis BLAS Library, linear algebra, Subroutines
   :description: Vitis BLAS library release notes.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _release_note:

Release Note
============

.. toctree::
   :hidden:
   :maxdepth: 1

2020.1
-------

The 1.0 release introduces HLS primitives for Basic Linear Algebra Subroutines (BLAS) operations. These primitives are implemented with HLS::stream interfaces to allow them to operate in parallel with other hardware components. 

2021.1
-------

The 2021.1 release introduces L2 kernels for GEMM and GEMV. It also introduces L3 APIs based on the XRT Native APIs.

2024.1
-------

The example design of L3/benchmarks/gemm/memKernel are deprecated and removed.


The helper function of "xfblasGetByAddress" has been removed, due to deprecate of xrt api of "xclUnmgdPread"
