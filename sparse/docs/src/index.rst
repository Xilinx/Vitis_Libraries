.. 
   
.. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: SPARSE, Library, Vitis SPARSE Library, linear, algebra, subroutines, vitis
   :description: Vitis SPARSE Library is a fast FPGA-accelerated implementation of the basic linear algebra subroutines for handling sparse matrices.
   :xlnxdocumentclass: Document
   :xlnxdocumenttypes: Tutorials

=====================
Vitis SPARSE Library
=====================

AMD Vitis |trade| SPARSE library is a fast FPGA-accelerated implementation of the basic
linear algebra subroutines for handling sparse matrices. The library provides two types of implementations: L1 primitives and  L2 kernels. These implementations are organized in their corresponding L1 and L2 directories.
- L1 primitives implementation can be leveraged by FPGA hardware developers.
- L2 kernels implementation provides usage examples for system and host code developers.

Advanced users can easily tailor, optimize, or combine the kernel code as it is developed with the permissive Apache 2.0 license.

Demos and usage examples of different level implementations are also provided
for reference. 

.. toctree::
   :caption: Introduction
   :maxdepth: 1

   Overview <overview.rst>
   Release Note <release.rst>
 
.. toctree::
   :caption: User Guide
   :maxdepth: 2

   L1 Primitives User Guide <user_guide/L1_user_guide.rst>
   L2 Kernel User Guide <user_guide/L2_user_guide.rst>

.. toctree::
   :caption: Benchmark Result
   :maxdepth: 1

   Benchmark <benchmark.rst>

Index
-----

* :ref:`genindex` 
* 
.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim: