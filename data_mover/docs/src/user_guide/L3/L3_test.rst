.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: BLAS, Library, Vitis BLAS Library, Vitis BLAS, level 3, test
   :description: Vitis BLAS level 3 provides test cases could build xclbin and run it.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


.. _test_l3:

=====================
L3 API Test
=====================

The AMD Vitis™ BLAS level 3 provides test cases that build xclbin and run it.

**1. Vitis BLAS L3 Compilation**

All tests provided here can be built with compilation steps similar to the following; the target can be either hw or hw_emu(for testing hw emulation).

.. code-block:: bash

  make host TARGET=hw
  
**2. Vitis BLAS L3 Run**

Tests can be run with the following steps; the target can be either hw or hw_emu(for testing hw emulation).

.. code-block:: bash

  make run TARGET=hw PLATFORM_REPO_PATHS=LOCAL_PLATFORM_PATH
