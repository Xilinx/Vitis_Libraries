..
   Copyright © 2019–2022 Xilinx Inc
   Copyright © 2022–2025 Advanced Micro Devices, Inc
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _EUCLIDEAN_DISTANCE:

==================
Euclidean Distance
==================

The DSPLib contains a solution for calculating the Euclidean Distance (ED). This library element computes the Euclidean Distance (ED) operation between two input vectors, P and Q, in a vectorized manner for the specifed dimension (1D, 2D, 3D, or 4D).
This is achieved by leveraging hardware acceleration on AIE and AIE-ML devices. The operation supports only IO Buffer interface, depending on the configuration.
The vectorized implementation ensures high performance by processing multiple data points in parallel, utilizing the hardware's capabilities.

Template parameters are used to configure the top-level graph of the `euclidean_distance_graph` class.

Entry Point
===========

The graph entry point is the following:

.. code-block::

    xf::dsp::aie::euclidean_distance::euclidean_distance_graph

Device Support
==============

The ED supports both AIE and AIE-ML devices for all features.

- Round modes available and the enumerated values of round modes differ between AIE and AIE-ML devices. See :ref:`COMPILING_AND_SIMULATING`.

Supported Input Data Types
==========================

The data types for input ports P and Q (`inP` and `inQ`) are controlled by `TT_DATA`. 
Both **inputs** and the **output** must be same data type. Supported data types are: `float` ( for both **AIE** and **AIE-ML**) and `bfloat16` (for **AIE-ML**).

Please see the table :ref:`euclidean_distance_supported_data_types` for valid input/output data types.

.. _euclidean_distance_supported_data_types:

.. table:: IO-BUFFER INTERFACE: Supported Input/Output Data Types
   :align: center

   +------------------+------------------+------------------+------------------+------------------+
   | InputP Data Type | InputQ Data Type | Output Data Type | AIE Valid        | AIE-ML Valid     |
   +==================+==================+==================+==================+==================+
   | float            | float            | float            | yes              | yes              |
   +------------------+------------------+------------------+------------------+------------------+
   | bfloat16         | bfloat16         | bfloat16         | **no**           | yes              |
   +------------------+------------------+------------------+------------------+------------------+

Template Parameters
===================

To see details on the template parameters for the Euclidean Distance, see :ref:`API_REFERENCE`.

Access Functions
================

To see details on the access functions for the Euclidean Distance, see :ref:`API_REFERENCE`.

Ports
=====

To see details on the ports for the Euclidean Distance, see :ref:`API_REFERENCE`.

Design Notes
============
The performance of the IP depends on the chosen data type :ref:`euclidean_distance_supported_data_types`. The number of multiplications per clock cycle will be updated based on the data type.
The Euclidean Distance supports IO Buffer interface (`TP_API = 0`).

Input Data
----------
Users must ensure that the **input data is padded with zeros** based on the chosen dimension `TP_DIM` since processing happens on fixed dimension i.e., **4D** to utilize hardware capabilities of vectorization. 
Please refer to the table below for the padding of input data :ref:`LEN. OF PADDED InP AND InQ DATA` and the example.

Padded_input:
-------------

| ``Dimension:3``

Example Config.,
 * ``TT_DATA`` - data type, e.g. ``float``,
 * ``TT_DATA_OUT`` - data type, e.g. ``float``,
 * ``TP_LEN`` - length of vector., e.g. ``8``,
 * ``TP_DIM`` - dimension of space, e.g. ``3D``,
 * ``TP_API`` - IO Buffer interface, e.g. ``0``,
 * ``TP_IS_OUTPUT_SQUARED`` - output squared, e.g. ``1``

.. code-block::

   in_P[TP_LEN * TP_DIM] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24],
   in_Q[TP_LEN * TP_DIM] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24],
   LEN = 8,
   DIM = 3,
   API = 0,
   IS_OUTPUT_SQUARED = 1

Example:
--------

- ``input vector P:``

.. table:: inDataP
   :align: center

   +---+---+---+---+---+---+---+---+---+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
   | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 | 20 | 21 | 22 | 23 | 24 |
   +---+---+---+---+---+---+---+---+---+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+

- ``input vector Q:``

.. table:: inDataQ
   :align: center

   +---+---+---+---+---+---+---+---+---+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
   | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 | 20 | 21 | 22 | 23 | 24 |
   +---+---+---+---+---+---+---+---+---+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+

|  **Padding**:

      - TP_DIM - dimension, e.g. ``3D``
      - FixedDim - ``4D``

      * Zeros_Padded_to_InVec = (FixedDim - TP_DIM) -- (4-3) -- ``1``. means 1 zero should be padded to the given input at every 4th index

.. table:: PaddedInDataP[TP_LEN*FixedDim] --> PaddedInDataP[32]
   :align: center

   +----+----+----+--------+----+----+----+--------+----+----+----+--------+----+----+----+--------+----+----+----+--------+----+----+----+--------+----+----+----+--------+----+----+----+--------+
   | 1D | 2D | 3D | **4D** | 1D | 2D | 3D | **4D** | 1D | 2D | 3D | **4D** | 1D | 2D | 3D | **4D** | 1D | 2D | 3D | **4D** | 1D | 2D | 3D | **4D** | 1D | 2D | 3D | **4D** | 1D | 2D | 3D | **4D** |
   +====+====+====+========+====+====+====+========+====+====+====+========+====+====+====+========+====+====+====+========+====+====+====+========+====+====+====+========+====+====+====+========+
   | 1  | 2  | 3  | **0**  | 4  | 5  | 6  |  **0** | 7  | 8  | 9  |  **0** | 10 | 11 | 12 |  **0** | 13 | 14 | 15 |  **0** | 16 | 17 | 18 |  **0** | 19 | 20 | 21 |  **0** | 22 | 23 | 24 |  **0** |
   +----+----+----+--------+----+----+----+--------+----+----+----+--------+----+----+----+--------+----+----+----+--------+----+----+----+--------+----+----+----+--------+----+----+----+--------+

.. table:: PaddedInDataQ[TP_LEN*FixedDim] --> PaddedInDataQ[32]
   :align: center

   +----+----+----+--------+----+----+----+--------+----+----+----+--------+----+----+----+--------+----+----+----+--------+----+----+----+--------+----+----+----+--------+----+----+----+--------+
   | 1D | 2D | 3D | **4D** | 1D | 2D | 3D | **4D** | 1D | 2D | 3D | **4D** | 1D | 2D | 3D | **4D** | 1D | 2D | 3D | **4D** | 1D | 2D | 3D | **4D** | 1D | 2D | 3D | **4D** | 1D | 2D | 3D | **4D** |
   +====+====+====+========+====+====+====+========+====+====+====+========+====+====+====+========+====+====+====+========+====+====+====+========+====+====+====+========+====+====+====+========+
   | 1  | 2  | 3  | **0**  | 4  | 5  | 6  |  **0** | 7  | 8  | 9  |  **0** | 10 | 11 | 12 |  **0** | 13 | 14 | 15 |  **0** | 16 | 17 | 18 |  **0** | 19 | 20 | 21 |  **0** | 22 | 23 | 24 |  **0** |
   +----+----+----+--------+----+----+----+--------+----+----+----+--------+----+----+----+--------+----+----+----+--------+----+----+----+--------+----+----+----+--------+----+----+----+--------+


| ``Dimension:2``

Example Config.,
 * ``TT_DATA`` - data type, e.g. ``float``,
 * ``TT_DATA_OUT`` - data type, e.g. ``float``,
 * ``TP_LEN`` - length of vector., e.g. ``8``,
 * ``TP_DIM`` - dimension of space, e.g. ``2D``,
 * ``TP_API`` - IO Buffer interface, e.g. ``0``,
 * ``TP_IS_OUTPUT_SQUARED`` - output squared, e.g. ``1``

.. code-block::

   in_P[TP_LEN * TP_DIM] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
   in_Q[TP_LEN * TP_DIM] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
   LEN = 8,
   DIM = 2,
   API = 0,
   IS_OUTPUT_SQUARED = 1


Example:
--------

- ``input vector P:``

.. table:: inDataP
   :align: center

   +---+---+---+---+---+---+---+---+---+----+----+----+----+----+----+----+
   | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 
   +---+---+---+---+---+---+---+---+---+----+----+----+----+----+----+----+

- ``input vector Q:``

.. table:: inDataQ
   :align: center

   +---+---+---+---+---+---+---+---+---+----+----+----+----+----+----+----+
   | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 
   +---+---+---+---+---+---+---+---+---+----+----+----+----+----+----+----+

| **Padding**:

      - TP_DIM - dimension, e.g. ``2D``
      - FixedDim - ``4D``

      * Zeros_Padded_to_InVec = (FixedDim - TP_DIM) -- (4-2) -- ``2``. means 2 zeros should be padded to the given input at every 3rd and 4th index

.. table:: PaddedInDataP[TP_LEN*FixedDim] --> PaddedInDataP[32]
   :align: center

   +----+----+--------+-------+----+----+--------+--------+----+----+--------+-------+----+----+--------+--------+----+----+--------+-------+----+----+--------+--------+----+----+--------+-------+----+----+--------+--------+
   | 1D | 2D | **3D** |**4D** | 1D | 2D | **3D** | **4D** | 1D | 2D | **3D** |**4D** | 1D | 2D | **3D** | **4D** | 1D | 2D | **3D** |**4D** | 1D | 2D | **3D** | **4D** | 1D | 2D | **3D** |**4D** | 1D | 2D | **3D** | **4D** |
   +====+====+========+=======+====+====+========+========+====+====+========+=======+====+====+========+========+====+====+========+=======+====+====+========+========+====+====+========+=======+====+====+========+========+
   | 1  | 2  | **0**  | **0** | 3  | 4  | **0**  |  **0** | 5  | 6  |  **0** | **0** | 7  | 8  |  **0** |  **0** | 9  | 10 |  **0** | **0** | 11 | 12 |  **0** |  **0** | 13 | 14 | **0**  | **0** | 15 | 16 |  **0** |  **0** |
   +----+----+--------+-------+----+----+--------+--------+----+----+--------+-------+----+----+--------+--------+----+----+--------+-------+----+----+--------+--------+----+----+--------+-------+----+----+--------+--------+

.. table:: PaddedInDataQ[TP_LEN*FixedDim] --> PaddedInDataQ[32]
   :align: center

   +----+----+--------+-------+----+----+--------+--------+----+----+--------+-------+----+----+--------+--------+----+----+--------+-------+----+----+--------+--------+----+----+--------+-------+----+----+--------+--------+
   | 1D | 2D | **3D** |**4D** | 1D | 2D | **3D** | **4D** | 1D | 2D | **3D** |**4D** | 1D | 2D | **3D** | **4D** | 1D | 2D | **3D** |**4D** | 1D | 2D | **3D** | **4D** | 1D | 2D | **3D** |**4D** | 1D | 2D | **3D** | **4D** |
   +====+====+========+=======+====+====+========+========+====+====+========+=======+====+====+========+========+====+====+========+=======+====+====+========+========+====+====+========+=======+====+====+========+========+
   | 1  | 2  | **0**  | **0** | 3  | 4  | **0**  |  **0** | 5  | 6  |  **0** | **0** | 7  | 8  |  **0** |  **0** | 9  | 10 |  **0** | **0** | 11 | 12 |  **0** |  **0** | 13 | 14 | **0**  | **0** | 15 | 16 |  **0** |  **0** |
   +----+----+--------+-------+----+----+--------+--------+----+----+--------+-------+----+----+--------+--------+----+----+--------+-------+----+----+--------+--------+----+----+--------+-------+----+----+--------+--------+

| ``Dimension:1``

Example Config.,
 * ``TT_DATA`` - data type, e.g. ``float``,
 * ``TT_DATA_OUT`` - data type, e.g. ``float``,
 * ``TP_LEN`` - length of vector., e.g. ``8``,
 * ``TP_DIM`` - dimension of space, e.g. ``1D``,
 * ``TP_API`` - IO Buffer interface, e.g. ``0``,
 * ``TP_IS_OUTPUT_SQUARED`` - output squared, e.g. ``1``

.. code-block::

   in_P[TP_LEN * TP_DIM] = [1, 2, 3, 4, 5, 6, 7, 8],
   in_Q[TP_LEN * TP_DIM] = [1, 2, 3, 4, 5, 6, 7, 8],
   LEN = 8,
   DIM = 1,
   API = 0,
   IS_OUTPUT_SQUARED = 1


Example:
--------

- ``input vector P:``

.. table:: inDataP
   :align: center

   +---+---+---+---+---+---+---+---+
   | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 
   +---+---+---+---+---+---+---+---+

- ``input vector Q:``

.. table:: inDataQ
   :align: center

   +---+---+---+---+---+---+---+---+
   | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 
   +---+---+---+---+---+---+---+---+

| **Padding**:

      - TP_DIM - dimension, e.g. ``1D``
      - FixedDim - ``4D``

      * Zeros_Padded_to_InVec = (FixedDim - TP_DIM) -- (4-1) -- ``3``. means 3 zeros should be padded to the given input at every 2nd, 3rd and 4th index.

.. table:: PaddedInDataP[TP_LEN*FixedDim] --> PaddedInDataP[32]
   :align: center

   +----+--------+--------+--------+----+--------+--------+--------+----+--------+--------+--------+----+--------+--------+--------+----+--------+--------+-------+----+--------+--------+--------+----+--------+--------+-------+----+--------+--------+--------+
   | 1D | **2D** | **3D** | **4D** | 1D | **2D** | **3D** | **4D** | 1D | **2D** | **3D** | **4D** | 1D | **2D** | **3D** | **4D** | 1D | **2D** | **3D** |**4D** | 1D | **2D** | **3D** | **4D** | 1D | **2D** | **3D** |**4D** | 1D | **2D** | **3D** | **4D** | 
   +====+========+========+========+====+========+========+========+====+========+========+========+====+========+========+========+====+========+========+=======+====+========+========+========+====+========+========+=======+====+========+========+========+
   | 1  | **0**  | **0**  | **0**  | 2  | **0**  | **0**  | **0**  | 3  |  **0** |  **0** | **0**  | 4  | **0**  | **0**  | **0**  | 5  | **0**  | **0**  | **0** | 6  |  **0** |  **0** | **0**  |  7 |  **0** |  **0** | **0** | 8  | **0**  | **0**  | **0**  |
   +----+--------+--------+--------+----+--------+--------+--------+----+--------+--------+--------+----+--------+--------+--------+----+--------+--------+-------+----+--------+--------+--------+----+--------+--------+-------+----+--------+--------+--------+

.. table:: PaddedInDataQ[TP_LEN*FixedDim] --> PaddedInDataQ[32]
   :align: center

   +----+--------+--------+--------+----+--------+--------+--------+----+--------+--------+--------+----+--------+--------+--------+----+--------+--------+-------+----+--------+--------+--------+----+--------+--------+-------+----+--------+--------+--------+
   | 1D | **2D** | **3D** | **4D** | 1D | **2D** | **3D** | **4D** | 1D | **2D** | **3D** | **4D** | 1D | **2D** | **3D** | **4D** | 1D | **2D** | **3D** |**4D** | 1D | **2D** | **3D** | **4D** | 1D | **2D** | **3D** |**4D** | 1D | **2D** | **3D** | **4D** | 
   +====+========+========+========+====+========+========+========+====+========+========+========+====+========+========+========+====+========+========+=======+====+========+========+========+====+========+========+=======+====+========+========+========+
   | 1  | **0**  | **0**  | **0**  | 2  | **0**  | **0**  | **0**  | 3  |  **0** |  **0** | **0**  | 4  | **0**  | **0**  | **0**  | 5  | **0**  | **0**  | **0** | 6  |  **0** |  **0** | **0**  |  7 |  **0** |  **0** | **0** | 8  | **0**  | **0**  | **0**  |
   +----+--------+--------+--------+----+--------+--------+--------+----+--------+--------+--------+----+--------+--------+--------+----+--------+--------+-------+----+--------+--------+--------+----+--------+--------+-------+----+--------+--------+--------+

.. note:: Length of output is nothing but ``TP_LEN``

.. note:: **At present, ``ED`` expects input from the user to be padded, as demonstrated in the examples above. Automatic padding of input will be handled in the next release, "2025.2."**

IO Buffer Interface
-------------------
The Euclidean Distance operation can be performed via the IO Buffer interface on **AIE**, **AIE-ML** devices.

| ``TP_API`` - **0** indicates **IO-BUFFER** interface

The IO Buffer interface supports all data types. See :ref:`euclidean_distance_supported_data_types` `table:: IO-BUFFER INTERFACE`.

Saturation
----------
Distortion caused by saturation will be possible for Euclidean Distance computation. Since the input values are input at run-time, no compile-time error can be issued for this hazard, so it is for the user to ensure that saturation does not occur.

ED results: output is squared or standard 
-----------------------------------------

The template parameter ``TP_IS_OUTPUT_SQUARED`` determines whether the output is squared i.e. ``ED^2`` or standard ``ED``.,

* ``TP_IS_OUTPUT_SQUARED`` = **0** indicates that the output is standard ED.
* ``TP_IS_OUTPUT_SQUARED`` = **1** indicates that the output is **squared** i.e., ``ED^2``.

.. note:: ``TP_IS_OUTPUT_SQUARED`` is 0 by default, meaning the output is always standard ED.

   +----------------------------+----------------------------------------+
   | TP_IS_OUTPUT_SQUARED       |      Output                            |
   +============================+========================================+
   |  0                         | e.g. **ED** = sqrt(Dx^2 + Dy^2 + Dz^2) |
   +----------------------------+----------------------------------------+
   |  1                         | e.g. **ED^2** = (Dx^2 + Dy^2 + Dz^2)   |
   +----------------------------+----------------------------------------+

Code Example
============

The following code example shows how the euclidean_distance_graph class can be used within a user super-graph.

.. literalinclude:: ../../../../L2/examples/docs_examples_2/test_euclidean_distance.hpp
    :language: cpp
    :lines: 17-

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim: