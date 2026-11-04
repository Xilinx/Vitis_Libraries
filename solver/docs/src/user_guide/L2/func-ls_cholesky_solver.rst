..
   Copyright (C) 2026, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _SOLVER_LS_CHOLESKY_SOLVER:

==================
LS_CHOLESKY_SOLVER
==================

This function performs the Least Squares (LS) Cholesky to solve :math:`x` in the equation  

.. math::
    Ax = y 

where A is a positive-definite Hermitian matrix, y is a vector and x is a vector.

The LS Cholesky Solver library element has configurable data type, matrix dimension size, and support for resource vs performance tradeoffs. 

Entry Point
===========

The graph entry point is the following:

.. code-block::

    xf::solver::ls_cholesky_solver::ls_cholesky_solver_graph

Device Support
==============

The LS Cholesky Solver library element supports AIE, AIE-ML, and AIE-MLv2 devices.

Supported Types
===============
The data type is controlled by ``TT_DATA`` and can be one of two choices: ``float`` or ``cfloat``.


Template Parameters
===================

To see details on the template parameters for the Substitution, see :ref:`SOLVER_API_REFERENCE`.


Access Functions
================

To see details on the access functions for the Substitution, see :ref:`SOLVER_API_REFERENCE`.

Ports
=====

To see details on the ports for the LS Cholesky Solver, see :ref:`SOLVER_API_REFERENCE`. Note that the port types are determined by the template parameter configuration.


Cascading (resource tradeoff)
-----------------------------

Pipelining the Cholesky decomposition stage with multiple chained kernels is configured using the ``TP_CASC_LEN`` template parameter. This parameter trades resource (number of tiles used) for throughput performance.



Numerical Accuracy
------------------

The LS Cholesky Solver uses the Cholesky transform which requires the input matrix to be positive-definite Hermitian. However, the Cholesky transform algorithm itself
is sensitive to condition number of the matrix. That is to say that if the input matrix is ill-conditioned then the error in the output values versus an infinite precision model
can be large. The core has no check for the condition number of the input matrix because this would be a run-time check which would impose a significant penalty on performance.
The user must therefore ensure that input matrices are well conditioned or apply their own test of condition as a run-time check.

Input Form
----------

The LS Cholesky Solver library element takes 2 operands, A and y. A is a positive-definite Hermitian matrix and y is a vector. L must be presented in row-major  
form.

Padding
-------

Padding is not a supported feature of the library element. For matrices whose dimensions are not multiples of :ref:`SOLVER_vecSampleNum`, the input matrix must be 
padded (aligned) such that it is a multiple of :ref:`SOLVER_vecSampleNum` (the output locations corresponding to the pads should be ignored). ``TP_DIM`` must be set 
to the padded dimension size. Note that although the pad samples are shown as having value 0 in the diagram below, this is not necessary for correct operation. 
Values outside of the 6x6 grid are not used by the IP and will not influence the values output.

The following is an example of a 6x6 matrix with :ref:`SOLVER_vecSampleNum` of 4 (thus ``TP_DIM`` set to 8):

.. code-block::

        +----------------------+
        |1  0  0  0  0  0  0  0|
        |2  3  0  0  0  0  0  0|
        |4  5  6  0  0  0  0  0|
        |7  8  9  10 0  0  0  0|
        |11 12 13 14 15 0  0  0|
        |16 17 18 19 20 21 0  0|
        |0  0  0  0  0  0  0  0|
        |0  0  0  0  0  0  0  0|
        +----------------------+

Constraints
-----------
``TP_DIM`` must be a multiple of :ref:`SOLVER_vecSampleNum`.
``TP_GRID_DIM`` must be 1.
``TP_DIM`` must be a multiple of ``TP_GRID_DIM``.
``TP_CASC_LEN`` must be between 1 and ``TP_DIM``, with a max value of 32.


Code Example
============
.. literalinclude:: ../../../../L2/examples/docs_examples/test_ls_cholesky_solver.hpp
    :language: cpp
    :lines: 17-

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
