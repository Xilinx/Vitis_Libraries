..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _Function_Approximation:

======================
Function Approximation
======================

The Function Approximation library element provides a vectorized linear approximation of a function, f(x), for a given input data, x, using a configured lookup table of slope and offset values that describe the function.
Utility functions which create the approximation lookup tables are provided for a number of frequently used functions such as sqrt(), invsqrt(), log(), exp() and inv().
The Function Approximation can also be provided with user-created lookup tables, provided they meet the requirements specified in the section below, :ref:`Configuring the Lookup Table <Configuring_the_Lookup_Table>`.

Entry Point
===========

The graph entry point is the following:

.. code-block::

    xf::dsp::aie::func_approx

Device Support
==============

The `func_approx` library element supports AIE, AIE-ML and AIE-MLv2.

- Round modes available and the enumerated values of round modes are the same for AIE-ML and AIE-MLv2 devices, but differ from those for AIE devices. See :ref:`COMPILING_AND_SIMULATING`.

Supported Types
===============

The input and output data type for the Function Approximation must be real-valued and is controlled by the ``TT_DATA`` template parameter.
For AIE devices, this parameter can be configured as `int16`, `int32`, or `float`. For AIE-ML and AIE-MLv2, this parameter can be configured as `int16`, `int32`, `float`, or `bfloat16`.

Template Parameters
===================

For further information on the template parameters for the Function Approximation, see :ref:`API_REFERENCE`.

Access Functions
================

To see details on the access functions for the Function Approximation, see :ref:`API_REFERENCE`.

Ports
=====

The Function Approximation library element supports `iobuffers` for input and output ports.

To see details on the ports for the Function Approximation, see :ref:`API_REFERENCE`.

Design Notes
============

Input Data
----------

There are three key parameters which describe the form of the expected input data necessary for approximation:

- `TP_COARSE_BITS`:     Specifies the number of bits in a data sample used to address the lookup table for coarse approximation.
- `TP_FINE_BITS`:       Specifies the number of least-significant bits in a data sample used for interpolation.
- `TP_DOMAIN_MODE`:     Sets the mode of the expected domain that the input is normalized to. For example, `TP_DOMAIN_MODE = 0` identifies that the domain of the input to the function for approximation is 0 to 1.

The required bit-field of a data sample from MSB to LSB is `[headroom, TP_COARSE_BITS, TP_FINE_BITS]`. Headroom is optional, but `TP_COARSE_BITS + TP_FINE_BITS` must be less than or equal to the number of bits in the specified `TT_DATA` type.

Therefore, for a given input sample, the corresponding `TP_COARSE_BITS` will provide the index to the supplied lookup table. At this index of the lookup table will be a slope and an offset value.
For integer types, the slope-offset values are in point-slope form. The offset value is the coarse approximation, and the slope value is multiplied with the `TP_FINE_BITS` of the input sample for linear interpolation.
The sum of these two values will provide the resulting approximation, `f(x)` for an input, `x`.

Shifting input down by `TP_FINE_BITS` will leave just the `TP_COARSE_BITS`. (If headroom exists, these bits must be zero).

.. code-block:: c

    index = input >> TP_FINE_BITS

Use index as an address to the lookup of slope-offset values. Multiply lower `TP_FINE_BITS` of input with looked-up slope and add to lookup offset values to find the resulting approximation.

.. code-block:: c

    output = offset[index] + slope[index] * input[TP_FINE_BITS-1:0]

For floating-point types, the slope-offset values are in slope-intercept form. The float input sample is cast to an integer, and the `TP_COARSE_BITS` of this cast integer sample is used as an index to the lookup tables.

.. code-block:: c

    index = int(input) >> TP_FINE_BITS
    output = offset[index] + slope[index] * input

Note, for floating-point data, all of the input is multiplied by the slope, whereas for integer data, only the lower `TP_FINE_BITS` is multiplied with the slope. This means that slope-offset values for the interpolated function lines are point-slope for integers and slope-intercept for floats. Further information can be found in the following section about configuring the lookup tables.

.. _Configuring_the_Lookup_Table:

Configuring the Lookup Table
----------------------------

There will be `2^TP_COARSE_BITS` in the lookup table. Each location will contain a slope value and an offset value which represent the linear approximation of the function at the corresponding location of the domain.
Lookup tables for integer data types require the slope/offset values to be obtained using the point-slope form, whereas lookup tables for floating-point types require the slope-intercept form.

For example, slope-offset values for integer types (point-slope):

.. code-block:: c

    slope[i] = y[i+1] - y[i]
    offset[i] = y[i]

Slope-offset values for floating-point types (slope-intercept):

.. code-block:: c

    slope[i] = (y[i + 1] - y[i]) / (x[i + 1] - x[i])
    offset[i] = y[i] - slope[i] * x[i]

The provided lookup table should be populated as below:

.. code-block:: c

    slope[0], offset[0], slope[1], offset[1], ... slope[2^TP_COARSE_BITS - 1], offset[2^TP_COARSE_BITS - 1]

A single lookup will require `sizeof(TT_DATA) * 2 * 2^TP_COARSE_BITS` bytes of memory. For performance reasons, a duplicate of the lookup is created by the `func_approx` graph.
Configurations for AIE-ML or AIE-MLv2 devices with a data type of `int16` or `bfloat16` will use the AI Engine API for improved parallel lookups. However, this requires an additional duplication within each lookup table.
This duplication will be done within the graph but must be accounted for when calculating the memory required for the provided lookup tables. Users must provide the lookup table, without any duplication, as a constructor argument to the graph.

Input Domain Modes
------------------

The template parameter `TP_DOMAIN_MODE` will control the domain of input data that is expected for the particular function's approximation.
Provided input data that is outside the chosen `TP_DOMAIN_MODE` domain will result in undefined behavior at the output. There are three possible `TP_DOMAIN_MODES` available:

+----------------+--------------------+-----------------------------------------------------------+-------------------------+----------------------------------------------------------------------------------------------------------------------------------+
| TP_DOMAIN_MODE | Input domain       | Integer input range                                       | Floating-point input    | Notes                                                                                                                            |
|                |                    |                                                           | range                   |                                                                                                                                  |
+================+====================+===========================================================+=========================+==================================================================================================================================+
| 0              | 0 <= x < 1         | 0 <= int(x) < 2 ^ (TP_COARSE_BITS + TP_FINE_BITS)         | 0 <= float(x) < 1       | Integer and floating-point samples in specified ranges correspond to a normalized input of 0 to 1. Must not be used for          |
|                |                    |                                                           |                         | functions where f(x) is infinity at x = 0.                                                                                       |
+----------------+--------------------+-----------------------------------------------------------+-------------------------+----------------------------------------------------------------------------------------------------------------------------------+
| 1              | 1 <= x < 2         | 0 <= int(x) < 2 ^ (TP_COARSE_BITS + TP_FINE_BITS - 1)     | 0 <= float(x) < 1       | The most significant bit in the TP_COARSE_BITS field of each data input sample must be set to zero. As such, the number of       |
|                |                    |                                                           |                         | locations in the lookup tables is half that of other TP_DOMAIN_MODES. This applies to floating-point values where float values   |
|                |                    |                                                           |                         | 0 to 1 will correspond to a domain of 1 to 2 for the function.                                                                   |
+----------------+--------------------+-----------------------------------------------------------+-------------------------+----------------------------------------------------------------------------------------------------------------------------------+
| 2              | 1 <= x < 4         | 2 ^ (TP_COARSE_BITS + TP_FINE_BITS - 2) <= int(x) < 2 ^   | 1 <= float(x) < 4       | Lookup tables values for the first quadrant of locations are created but are ignored. As such, the latter three quadrants of the |
|                |                    | (TP_COARSE_BITS + TP_FINE_BITS)                           |                         | lookup tables will cover an input domain of 1 to 4.                                                                              |
+----------------+--------------------+-----------------------------------------------------------+-------------------------+----------------------------------------------------------------------------------------------------------------------------------+

Lookup Utility Functions
------------------------

A number of utility functions are provided to create lookup tables for some common function approximations.
The functions, as well as their recommended domain modes, are documented in :ref:`API_REFERENCE`.

Code Example
============

.. literalinclude:: ../../../../L2/examples/docs_examples_2/test_func_approx.hpp
    :language: cpp
    :lines: 17-

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
