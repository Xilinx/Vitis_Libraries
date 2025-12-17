..
    Copyright (C) 2022-2025, Advanced Micro Devices, Inc.

    `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _DSP_Cumsum:

======
Cumsum
======

This library element implements Cumulative Sum function on a vector of samples. It has configurable data type, output data type, vector length, number of vectors, number of frames, operation mode,
scaling (as a shift), rounding, and saturation modes.

Entry Point
===========

The graph entry point is as follows:

.. code-block::

    xf::dsp::aie::cumsum::cumsum_graph

Device Support
==============

Cumsum supports AIE, AIE-ML, and AIE-MLv2 devices with the following exceptions:

- ``TT_DATA`` and ``TT_OUT_DATA`` support ``bfloat16`` and ``cbfloat16`` on AIE-ML and AIE-MLv2, but not on AIE.
- ``TT_DATA`` and ``TT_OUT_DATA`` support ``float`` and ``cfloat`` on AIE, but not on AIE-ML or AIE-MLv2.

Round modes and their enumerated values are the same for AIE-ML and AIE-MLv2 devices, but differ from those for AIE devices. See :ref:`DSP_COMPILING_AND_SIMULATING`.

Supported Data Types
====================

The input data type to the Cumsum is controlled by the ``TT_DATA`` template parameter and must be one of: ``int16``, ``int32``, ``float``, ``bfloat16``, or complex versions of these.
``TT_OUT_DATA`` selects the output type, which must be the same type as ``TT_DATA`` with equal or greater precision. For example, for ``cint16`` input, valid output types are ``cint16`` and ``cint32``.

Template Parameters
===================

For details on the template parameters for Cumsum, see :ref:`DSP_API_REFERENCE`.

Access Functions
================

For details on the access functions for Cumsum, see :ref:`DSP_API_REFERENCE`.

Ports
=====

For details on the ports for Cumsum, see :ref:`DSP_API_REFERENCE`.

Note that the number and type of ports are determined by the configuration of template parameters.
The size of the input and output window is a product of the size of each element as defined by ``TT_DATA`` and ``TT_OUT_DATA``, the second dimension ``TP_DIM_B``, the number of frames ``TP_NUM_FRAMES``, and the ceiling function of the first dimension ``TP_DIM_A`` rounded up to an integer number of data memory accesses. Memory access is 256 bits wide on AIE and AIE-ML, but 512 bits on AIE-MLv2.

For example, with ``TP_DIM_A = 27``, ``TP_DIM_B = 8``, ``TP_NUM_FRAMES = 1``, and ``TT_DATA = float``, the window size for the dimensions statement in your supergraph will be:

.. code-block:: text

     WindowSize = TP_DIM_B * TP_NUM_FRAMES * CEIL(TP_DIM_A, kSamplesInMemAccess)

where ``kSamplesInMemAccess = kMemAccessWidthBytes / sizeof(TT_DATA)`` and ``kMemAccessWidthBytes = 32`` for AIE and AIE-ML, or ``64`` for AIE-MLv2.

Design Notes
============

Cumsum performs a sample-by-sample accumulation. For example, for input ``4, 5, 6, 7``, the output would be ``4, 9, 15, 22``.

The parameter ``TP_MODE`` selects the mode of operation. The following modes are supported:

``TP_MODE = 0``
---------------

This is conventional cumulative sum of element-wise accumulation along the first dimension. Note that this will operate on all samples in a row, even above ``TP_DIM_A`` up to ``CEIL(TP_DIM_A, kSamplesInMemAccess)``, as this is faster than padding with zeros using exception code.

``TP_MODE = 1``
---------------

Cumsum is performed along the second dimension. For example, with ``TP_DIM_A = 4``, ``TP_DIM_B = 4``, the input window may look like:

.. code-block:: text

     {{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15}}

This will result in an output of:

.. code-block:: text

     {{0, 1, 2, 3}, {4, 6, 8, 10}, {12, 15, 18, 21}, {24, 28, 32, 36}}

Note that ``TP_DIM_A`` will be rounded up as described earlier by the CEIL function to the width of memory access. ``TP_DIM_A`` itself does not need to be a multiple of the memory access width. The IP will ignore samples beyond ``TP_DIM_A`` up to this ceiling value and will output zeros for these samples.

``TP_MODE = 2``
---------------

This mode is conceptually identical to ``TP_MODE = 0`` with ``TP_SHIFT = 0``. It is a faster implementation, but the output is undetermined if the cumsum overflows the range of ``TT_OUT_DATA``.
If this occurs, the output sample in question and all following samples until the end of that vector will be undefined. No defensive check or detection of this error condition is implemented, as doing so would degrade performance.
The user must ensure that the cumsum does not overflow ``TT_OUT_DATA`` when using this mode.

Scaling
-------

Scaling in Cumsum is controlled by the ``TP_SHIFT`` parameter, which describes how many binary places to shift the result to the right (i.e., only power-of-2 scaling values are supported).
No scaling is applied when the data type is a floating point type. Setting ``TP_SHIFT`` to any value other than 0 when ``TT_DATA`` is a floating-point type will result in an error.

Rounding and Saturation
-----------------------

Rounding and saturation occur when scaling is applied, but only have effect if ``TP_SHIFT`` is non-zero.

Constraints
-----------

Cumsum does not contain any constraints.

Code Example
============

.. literalinclude:: ../../../../L2/examples/docs_examples/test_cumsum.hpp
     :language: cpp
     :lines: 17-

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
    :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
    :ltrim:
