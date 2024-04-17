.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: BLAS, Library, Vitis BLAS Library, L2 Kernel, Gemm
   :description: Vitis BLAS library L2 applications.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


.. _user_guide_overview_content_l2:

Vitis BLAS L2 predefined kernels are the C++ implementation of BLAS functions. These implementations are intended to demonstrate how FPGA kernels are defined and how L1 primitive functions can be used by any AMD Vitis™ user to build kernels for applications. 

1. Introduction
================

L2 kernel implementations include memory datamovers and computation components composed by L1 primitive functions. The kernels always have memoy (DDR/HBM) interfaces. The data mover modules move data between the vectors' and matrices' off-chip storage and the computation modules.  The L1 primitive functions with stream interfaces can be quickly chained with the data mover modules together to form a computation kernel. The organization of the Vitis BLAS L2 files and directories, as described below, reflects this design strategy:

* **L2/include/hw/xf_blas/**: The directory that contains the kernel modules.
* **L2/include/sw/**: The directory that contains the host modules.
* **L2/test/hw**: The directory that contains the Makefiles used for testing each implemented kernel.
   
More information about computation and data mover modules can be found in :doc:`L2 GEMM kernel<L2_gemm>`. 

2. L2 Kernel Usage
========================

Vitis BLAS L2 predefined kernels can be used in your applications based on BLAS functions. These kernels are also examples to present how to use the L1 primitive funtions and datamovers to build a kernel.
