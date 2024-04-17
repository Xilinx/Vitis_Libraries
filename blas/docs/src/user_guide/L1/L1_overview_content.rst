.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: BLAS, Library, Vitis BLAS Library, primitives, matrix storage
   :description: Vitis BLAS library L1 primitives are the C++ implementation of BLAS functions.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


.. _user_guide_overview_content_l1:


The AMD™ Vitis BLAS L1 primitives are the C++ implementation of BLAS functions. These implementations are intended to be used by high level synthesis (HLS) users to build FPGA logic for their applications. 

1. Introduction
================

The L1 primitives' implementations include computation and data mover modules. The computation modules always have stream interfaces. The data mover modules move data between vectors and matrices on-chip storage and the computation modules. This design strategy allows FPGA application programmers to quickly develop a high-performed logic by simply chaining several computation and data mover modules together. The organization of Vitis BLAS L1 files and directories, as described below, reflects this design strategy.

* **L1/include/hw/xf_blas**: The directory that contains the computation modules.
* **L1/include/hw/xf_blas.hpp**: The header file for L1 primitivers' users.
* **L1/include/hw/helpers/dataMover**: The directory that contains the data mover modules.
* **L1/include/hw/helpers/funcs**: The directory that contains the common computation modules used by several primitives.
* **L1/include/hw/helpers/utils**: The directory that contains the utilities used in the primitives' implementations.
* **L1/test/hw**: The directory that contains the top modules used for testing each implemented primitive, including its computation and data mover modules.
* **L1/test/sw**: The directory that contains the test bench and test infrastructure support for the primitives.
* **L1/test/build**: The directory that includes the vivado_hls script used for creating the vivado_hls project to test each primitive's implementation.
* **L1/test/run_test.py**: The Python script for testing the L1 primitives' implementations.
* **L1/test/set_env.sh**: The shell script for setting up the environment used for testing the L1 primitives.

More information about computation and data mover modules can be found in :doc:`L1 computation APIs<L1_compute_api>` and :doc:`L1 data mover APIs<L1_data_mover>`. 

2. L1 Primitives Usage
========================

Vitis BLAS L1 primitives are intended to be used by hardware developers to implement an application or algorithm specific FPGA logic in HLS. The following example code shows a typical usage of L1 primitives.
 
The ``uut_top.cpp`` file in each primitive folder under the ``L1/tests/hw`` directory provides a usage example of combining computation and data mover modules of the primitive. More information about testing L1 primitives can be found in :doc:`Test L1 primitives<L1_test>`.

3. Matrix storage used in L1 primitives 
========================================

The data mover components move matrice and vector data stored in the on-chip memory, normally block RAM or URAM slices, into streams to feed the computation modules. The following matrix storage formats are supported:

* row-major matrix
* row-major symmetric matrix
* row-major packed symmetric matrix
* row-major triangular matrix
* row-major packed triangular matrix
* column-major banded matrix
* column-major banded symmetric matrix

More information about matrix storage formats and data mover components can be found in :doc:`Data movers used in L1 primitives<L1_data_mover>`.
 