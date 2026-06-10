..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _SOLVER_SUBSTITUTION:

============
Substitution
============

This function performs forwards or backwards substitution to solve :math:`x` in the equation  

.. math::
    Lx = y  (forwards)   or  {L}^{*}x = y  (backwards)

where L is a lower triangular matrix with real diagonal elements, y is a vector and :math:`{L}^{*}` is an upper triangular matrix, the conjugate transpose of L.

The Substitution IP is designed for standalone operation, or in conjunction with the Cholesky IP to solve a set of linear equations.  

The Substitution library element has configurable data types, input form (row-major or column major), matrix sizes, and support for parallelism. The interface is designed to be interoperable with 
the Cholesky library even when parallelism is used.

Entry Point
===========

The graph entry point is the following:

.. code-block::

    xf::solver::aie::substitution::substitution_graph

Device Support
==============

The Substitution library element supports AIE, AIE-ML, and AIE-MLv2 devices.

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

To see details on the ports for the Substitution, see :ref:`SOLVER_API_REFERENCE`. Note that the port types are determined by the template parameter configuration.


Parallelism (Grid Tiling)
-------------------------

Parallelism for the Substitution is configured using the ``TP_GRID_DIM`` template parameter. This parameter scales the size of the lower-triangular grid of tiles used to split up the input matrix. A triangle of tiles are used since this allows a 1:1 connection with the Cholesky when similarly configured.
``TP_GRID_DIM`` must be a factor of ``TP_DIM``, and the resulting sub-matrix dimension must be a multiple of :ref:`SOLVER_vecSampleNum`.
The number of AIE tiles used in the design scales according to ``TP_GRID_DIM`` * (``TP_GRID_DIM`` + 1) / 2.

The following is an example of ``TP_DIM`` and ``TP_GRID_DIM`` being used:

- ``vecSampleNum`` = 2
- ``TP_DIM`` = 6
- ``TP_GRID_DIM`` = 3

.. code-block::

        +-----+
        |0  1 |
        |2  3 |
        +-----+-----+
        |0  1 |0  1 |
        |2  3 |2  3 |
        +-----+-----+-----+
        |0  1 |0  1 |0  1 |
        |2  3 |2  3 |2  3 |
        +-----+-----+-----+

.. note:: The numbers represent sample indices of the input sub-matrices, and the drawn boundaries represent the samples processed by each sub-kernel. 

.. note:: Diagonal elements must be real (any value on the imaginary is ignored and assumed to be zero) and samples in the off-diagonal upper triangle are ignored and assumed to be zero.

Input Form
----------

The Substitution library element takes 2 operands, L and y. L is a lower-triangular matrix and y is a vector. L may be presented in either row-major or column-major 
form because the IP supports the parameter ``TP_L_LEADING``. The IP will perform any necessary transpose internally.
For backwards substitution, the Substitution library element still requires L, not the transpose conjugate of L. This convention allows the Substitution library element 
to be used more easily in conjunction with the Cholesky library element to solve a set of linear equations because the Cholesky library element outputs L.

Solving a System of Linear Equations
-------------------------------------

The Substitution library element may be used in conjunction with the Cholesky library element to solve a system of linear equations 
:math:`Ax=y` where A is an mxm matrix suitable for input to the Cholesky library element and x and y are vectors of length m.
Since the Cholesky produces L such that
:math:`A = L {L}^*`
we may re-write the original equation as
:math:`L ({L}^* x) = y`
if we substitute :math:`b` for :math:`{L}^* x`
we get
:math:`Lb = y`
We can use forward substitution to solve for b.
Then, we use backward substitution to solve for :math:`x` in
:math:`{L}^{*} x = b`

Padding
-------

Padding is not a supported feature of the library element. For matrices whose dimensions are not multiples of :ref:`SOLVER_vecSampleNum`, the input matrix must be 
padded such that it is a multiple of :ref:`SOLVER_vecSampleNum` (the output locations corresponding to the pads should be ignored). ``TP_DIM`` must be set to the 
padded dimension size. Note that although the pad samples are shown as having value 0 in the diagram below, this is not necessary for correct operation. Values outside of the 6x6 grid are not used by the IP and will not influence the values output.

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
``TP_DIM`` must be a multiple of ``TP_GRID_DIM``.
``TP_DIM``/``TP_GRID_DIM`` must be a multiple of :ref:`SOLVER_vecSampleNum`.


Code Example
============
.. literalinclude:: ../../../../L2/examples/docs_examples/test_substitution.hpp
    :language: cpp
    :lines: 17-

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
