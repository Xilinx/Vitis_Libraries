..
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

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
------------

Currently, the AMD Vitis AIE Solver Library provides the following operations on AI Engine.

* Matrix decomposition
   * Cholesky decomposition for symmetric positive definite matrix
   * QR decomposition for general matrix
   * Singular value decomposition
   * Pseudoinverse

Requirements
------------

Software requirements
~~~~~~~~~~~~~~~~~~~~~
* Vitis™ Unified Software Platform |ProjectVersion|
* CentOS/RHEL 7.4, 7.5 and Ubuntu 16.04.4 LTS, 18.04.1 LTS.

Hardware requirements
~~~~~~~~~~~~~~~~~~~~~
* For PL Solver library
   * `Alveo U200 <https://www.xilinx.com/products/boards-and-kits/alveo/u200.html>`_
   * `Alveo U250 <https://www.xilinx.com/products/boards-and-kits/alveo/u250.html>`_
* For AI Engine Solver library
   * `VCK190 <https://www.xilinx.com/products/boards-and-kits/vck190.html>`_


License
-------

    Licensed using the `Apache 2.0 license <https://www.apache.org/licenses/LICENSE-2.0>`_::


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

    

