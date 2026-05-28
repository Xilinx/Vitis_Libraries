..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.

   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _SOLVER_QRD:

================
QR Decomposition
================

This function computes the QR decomposition of matrix :math:`A` using the modified Gram-Schmidt process. The QR decomposition is defined as:

.. math::
    A = Q R

where :math:`A` is the input matrix of size :math:`m \times n` (:math:`m` is the number of rows and :math:`n` is the number of columns). :math:`Q` is an orthonormal matrix of size :math:`m \times n` (with orthonormal columns), and :math:`R` is an upper-triangular matrix of size :math:`n \times n`.
The QRD library element has configurable data types and matrix sizes, a configurable number of frames, and support for cascaded kernel implementation to allow larger matrix decomposition.


Entry Point
===========

The graph entry point is the following:

.. code-block::

    xf::solver::qrd::qrd_graph

Device Support
==============

The QRD library element supports AIE, AIE-ML, and AIE-MLv2 devices.

Supported Types
===============

The data type is controlled by ``TT_DATA`` and can be one of two choices: ``float`` or ``cfloat``.


Template Parameters
===================

To see details on the template parameters for the QRD, see :ref:`SOLVER_API_REFERENCE`.


Access Functions
================

To see details on the access functions for the QRD, see :ref:`SOLVER_API_REFERENCE`.

Ports
=====

To see details on the ports for the QRD, see :ref:`SOLVER_API_REFERENCE`. Note that the port types are determined by the template parameter configuration.

Design Notes
============

Cascaded Implementation for Pipelining Larger Matrices
-------------------------------------------------------

QR decomposition is accomplished using the modified Gram-Schmidt algorithm; therefore a fully parallel architecture is not possible due to its sequential nature. However, the algorithm can be pipelined by cascading multiple kernels together, with each kernel processing a subset of the input matrix columns.

The input matrix is split into sub-matrices by the load-splitting algorithm. Please refer to :ref:`SOLVER_load-splitting` for detailed information. The idea of load-splitting is that each kernel in the cascade topology processes a different number of columns, while trying to balance projection operations across the kernels.

The first kernel processes the first sub-matrix, the second kernel processes the second sub-matrix, and so on. As soon as the first kernel has processed the first column of the input matrix, it passes the resulting column of the Q matrix to the second kernel, which can then begin processing the second sub-matrix. While processing the received Q column, the second kernel passes the column to the third kernel, and so on. This enables the kernels to work in a pipelined fashion, with each kernel processing its sub-matrix and passing Q columns to the next kernel in the cascade. Utilizing ``TP_CASC_LEN`` is necessary for larger matrix implementations where a single kernel's local memory is insufficient to hold the entire matrix. It can also be used to increase throughput and decrease latency for smaller matrices.


.. _SOLVER_load-splitting:


Load-Splitting
~~~~~~~~~~~~~~~

The load-splitting algorithm is used to split the input matrix into sub-matrices. The algorithm aims to balance the number of projection operations across the kernels, whilst ensuring that each kernel can process its sub-matrix within its local memory constraints. The number of projection operations is defined as the number of times a column vector is projected onto another column vector during the modified Gram-Schmidt process.

The load-splitting algorithm is implemented in the ``qrd_col_dist.py`` script, which is provided as part of the QRD library element. The script outputs the number of columns assigned to each kernel in the cascade. The user can be informed beforehand about the load-splitting by running the script with the appropriate parameters. The script takes the following parameters:

- aie_variant: The AIE variant being used (e.g., aie, aie-ml, aie-mlv2).
- data_type: The data type being used (e.g., float, cfloat).
- dim_rows: The number of rows in the input matrix.
- dim_cols: The number of columns in the input matrix.
- casc_len: The number of kernels in the cascade.
- num_frames: The number of frames being processed.

Column Distribution Script Example
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

    python qrd_col_dist.py -params aie_variant,data_type,dim_rows,dim_cols,casc_len,num_frames

Due to the nature of the algorithm, it is not always possible to perfectly balance the projection operations across the kernels. However, the algorithm aims to minimize the difference in projection operations between the kernels as much as possible.

The following is an example of load-splitting for a 32x32 matrix with a cascade length of 4:

.. table:: Load-Splitting Example

    +--------------+-----+-----+-----+-----+
    |  Kernels     | K0  | K1  | K2  | K3  |
    +==============+=====+=====+=====+=====+
    | #cols        | 16  |  6  |  5  |  5  |
    +--------------+-----+-----+-----+-----+
    | #projections | 120 | 111 | 120 | 145 |
    +--------------+-----+-----+-----+-----+

Padding
-------

Padding is not a supported feature of the library element. ``TP_DIM_ROWS`` and ``TP_DIM_COLS`` must be set to multiples of :ref:`SOLVER_vecSampleNum` (which is 8 for float and 4 for cfloat on AIE-ML and AIE-MLv2 devices and 16 for float and 8 for cfloat on AIE2 devices).

If a float input matrix of 5x5 is to be processed, the user must set ``TP_DIM_ROWS`` and ``TP_DIM_COLS`` to 8x8, and pad the input matrix with zeros to make it 8x8. The resulting Q and R matrices will also be of size 8x8, with the additional rows and columns being padded with zeros.

Constraints
-----------

Input matrix data must be provided in column-major form. The input matrix is assumed to be full rank for the QR decomposition to be valid.

The ``float`` data type supports row-major operation, but the ``cfloat`` data type supports only column-major operation. To enable row-major data reads and writes, set the ``DIM_A_LEADING``, ``DIM_Q_LEADING``, and ``DIM_R_LEADING`` template parameters to 1.


Code Example
============

.. literalinclude:: ../../../../L2/examples/docs_examples/test_qrd.hpp
    :language: cpp
    :lines: 17-

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
