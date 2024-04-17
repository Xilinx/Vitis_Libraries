.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: BLAS, Library, Vitis BLAS Library, L2 Kernel, GEMM
   :description: Vitis BLAS library L2 applications.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


.. _user_guide_gemm_l2:

********************
Blas Function Kernel
********************

Blas Function Kernels
========================

BLAS kernels in this library have a uniform top function. This top function has only two memory interfaces for communicating with external memories via an AXI memory controller.  The external memory interface can be DDR, HBM, or PLRAM. As shown in the following figure, the top function blasKernel is composed by an instruction process unit, timer, and operation functional unit e.g., GEMM. The functional unit can be a single BLAS function or more. 

.. image:: images/blasKernel.png
    :align: center


GEMM Kernels
========================
General matrix multiply (GEMM) is a very common and important function in the BLAS library. 
It is a core operation in many applications, such as machine learning algorithms. The GEMM operation C = A * B + X is implemented as a kernel in this library.

.. toctree::
   :maxdepth: 1

   GEMM Kernel <L2_gemm_content.rst>
