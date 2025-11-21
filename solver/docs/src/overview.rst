..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis, Solver, Library, Vitis Solver Library, overview, matrix, linear, eigenvalue
   :description: Vitis Solver Library provides a collection of matrix decomposition operations, linear solvers and eigenvalue solvers.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _overview:

.. toctree::
      :hidden:

Library Overview
=================

AMD Vitis |trade| Solver Library provides a collection of matrix decomposition operations, linear solvers, and eigenvalue solvers on PL and AI Engine：
 
PL Solver library
-----------------

Currently the AMD Vitis PL Solver library includes the following operations for dense matrix
 
* Matrix decomposition
   * Cholesky decomposition for symmetric positive definite matrix
   * LU decomposition without pivoting and with partial pivoting
   * QR decomposition for general matrix
   * SVD decomposition (single value decomposition) for symmetric matrix and non-symmetric matrix (Jacobi method)
 
* Linear solver
   * Tridiagonal linear solver (Parallel cyclic reduction method)
   * Linear solver for triangular matrix
   * Linear solver for symmetric and non-symmetric matrix
   * Matrix inverse for symmetric and non-symmetric matrix
 
* Eigenvalue solver
   * Jacobi eigenvalue solver for symmetric matrix


AI Engine Solver library
------------------------

Currently, the AMD Vitis AIE Solver Library provides the following operations on AI Engine.

* Matrix decomposition
   * Cholesky decomposition
   * QR decomposition (Gram-Schmidt method)

Requirements
------------

Software requirements
~~~~~~~~~~~~~~~~~~~~~
* This library is designed to work with AMD Vitis |trade|, and therefore inherits the system requirements of Vitis and XRT.
* Supported operating systems are RHEL/CentOS RHEL 9.4, RHEL 9.5, RHEL 9.6, and RHEL 10.0, Ubuntu 22.04.3 LTS, 22.04.4 LTS, and 22.04.5 LTS, Ubuntu 24.04 LTS, 24.04.1 LTS, and 24.04.2 LTS, as well as AlmaLinux OS ver. 8.10, 9.4, 9.5, 9.6, and 10.0, Rocky 8.10, 9.6, and 10.0.

Hardware requirements
~~~~~~~~~~~~~~~~~~~~~
* For PL Solver library
   * `Alveo U200 <https://www.xilinx.com/products/boards-and-kits/alveo/u200.html>`_
   * `Alveo U250 <https://www.xilinx.com/products/boards-and-kits/alveo/u250.html>`_
* For AI Engine Solver library
   * Hardware modules and kernels are designed to work with AMD Alveo |trade| U200 and U280 cards.
   * AI Engine graphs and kernels are designed to work with AMD |trade| Versal AI Core, Versal AI Edge, and Versal AI Edge Gen 2 devices, which are available on the VCK190, VEK280, and VEK385 boards, respectively.

License
-------

Licensed using the `Apache 2.0 license <https://www.apache.org/licenses/LICENSE-2.0>`_.

Trademark Notice
----------------

    AMD, the AMD logo, Artix, ISE, Kintex, Spartan, Virtex, Zynq, and
    other designated brands included herein are trademarks of AMD in the
    United States and other countries.  All other trademarks are the property
    of their respective owners.

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
