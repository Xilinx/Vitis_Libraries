.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: BLAS, Library, Vitis BLAS Library, data mover
   :description: Vitis BLAS library L1 data mover modules are used to move matrix and vector data between their on-chip storage and the input/output streams of the computation modules.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


.. _user_guide_data_mover_l1:

***********************
L1 Data Mover
***********************

The L1 data mover modules are used to move matrix and vector data between their on-chip storage and the input/output streams of the computation modules. These data movers are intended to be used in conjunction with computation modules to form the HLS implementations for BLAS level 1 and 2 functions. You can find this usage in the ``uut_top.cpp`` files of the BLAS function name folders under the ``L1/tests`` directory.

1. Matrix Storage Format
=========================

The following matrix storage formats are supported by L1 data mover modules.

* Row-based storage in a contiguous array
* Packed storage for symmetric and triangular matrices
* Banded storage for banded matrices

For symmetric, triangular, and banded storage, both Up and Lo storage modes are supported. More details about each storage format can be found in `xf_blas/L1/matrix_storage`_.

.. _xf_blas/L1/matrix_storage: https://www.netlib.org/blas/blast-forum/chapter2.pdf

2. Data Mover APIs
===================

.. toctree:
   :maxdepth: 2

.. include:: ./namespace_xf_linear_algebra_blas_DM.rst
   :start-after: Global Functions
