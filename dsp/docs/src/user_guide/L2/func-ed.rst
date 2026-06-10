..
   Copyright © 2019–2022 Xilinx Inc.
   Copyright © 2022–2026 Advanced Micro Devices, Inc.

   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _DSP_EUCLIDEAN_DISTANCE:

===================
Euclidean Distance
===================

This library element computes the Euclidean Distance (ED) between two sets of points, P and Q. The Euclidean Distance is the straight-line (L2-norm) distance between a pair of points; for two points of dimension D it is ``ED = sqrt( (q0-p0)^2 + (q1-p1)^2 + ... + (q(D-1)-p(D-1))^2 )``. 

One distance value is produced per point pair, so for ``TP_LEN`` point pairs the output vector has ``TP_LEN`` samples. The Euclidean Distance IP has configurable data type (``float`` or ``bfloat16``), vector length (``TP_LEN``), point dimension (``TP_DIM``, 1D to 4D), output mode (distance or squared, via ``TP_IS_OUTPUT_SQUARED``), rounding, and saturation. It is supported on AIE, AIE-ML, and AIE-MLv2 devices and uses the IO-buffer interface.

Template parameters are used to configure the top-level graph of the ``euclidean_distance_graph`` class.

Entry Point
===========

The graph entry point is as follows:

.. code-block::

    xf::dsp::aie::euclidean_distance::euclidean_distance_graph

Device Support
==============

The ED supports **AIE**, **AIE-ML**, and **AIE-MLv2** devices for all features.

- The available round modes and their enumerated values differ between AIE and AIE-ML/AIE-MLv2 devices. See :ref:`DSP_COMPILING_AND_SIMULATING`.

Supported Input Data Types
==========================

The data types for input ports P and Q (``inP`` and ``inQ``) and the output port are controlled by ``TT_DATA``.
Supported data types are: ``float`` (on **AIE**, **AIE-ML**, and **AIE-MLv2**) and ``bfloat16`` (on **AIE-ML** and **AIE-MLv2**).

Template Parameters
===================

To see details on the template parameters for the Euclidean Distance, see :ref:`DSP_API_REFERENCE`.

Access Functions
================

To see details on the access functions for the Euclidean Distance, see :ref:`DSP_API_REFERENCE`.

Ports
=====

To see details on the ports for the Euclidean Distance, see :ref:`DSP_API_REFERENCE`.

Design Notes
============

The Euclidean Distance supports the IO-buffer interface (``TP_API`` = ``0``) only.

Length Constraints
------------------

``TP_LEN`` specifies the number of P/Q point pairs processed and the number of
output samples produced. ``TP_LEN`` must be a multiple of the vector load size
for the chosen device and data type. The minimum ``TP_LEN`` equals the applicable granularity,
and the maximum is bounded by the data memory available on the target device.

Dimension (TP_DIM)
------------------

``TP_DIM`` selects the spatial dimension of each point P and Q and must be in the range ``[1, 4]``. All of 1D (``TP_DIM`` = ``1``), 2D, 3D, and 4D are supported on **AIE**, **AIE-ML**, and **AIE-MLv2**.

Regardless of ``TP_DIM``, each point still occupies a fixed four-element group in memory (see :ref:`DSP_ED_INPUT_DATA` below); only the first ``TP_DIM`` elements of each group are used in the calculation, and the remaining ``4 - TP_DIM`` elements are ignored.

.. _DSP_ED_INPUT_DATA:

Input Data
----------

Input data must have length ``TP_LEN × 4`` samples for each vector (P and Q), regardless of ``TP_DIM`` value. The hardware processes data in 4-element groups, using only the first ``TP_DIM`` elements for calculations.

**Configuration Example:**

.. code-block::

   TP_LEN = 8, TP_DIM = 3, TP_IS_OUTPUT_SQUARED = 1

   in_P[32] = [1,2,3,4, 5,6,7,8, 9,10,11,12, ..., 29,30,31,32]
   in_Q[32] = [32,31,30,29, 28,27,26,25, 24,23,22,21, ..., 4,3,2,1]

**Data Organization:**

::

    4-Element Groups (TP_LEN=8):
    P0:{1,2,3,4}   P1:{5,6,7,8}   P2:{9,10,11,12}   ...   P7:{29,30,31,32}
    Q0:{32,31,30,29} Q1:{28,27,26,25} Q2:{24,23,22,21} ... Q7:{4,3,2,1}

    When TP_DIM=3, only first 3 elements are used:
    P0→{1,2,3} Q0→{32,31,30} (4th element ignored)

**Memory Layout and Access Pattern**

::

    Memory Layout (4-element groups):

    Vector P Memory:
    [1][2][3][4]   [5][6][7][8]   [9][10][11][12]   ...   [29][30][31][32]
        P0             P1              P2                        P7

    Vector Q Memory:
    [32][31][30][29]   [28][27][26][25]   [24][23][22][21]   ...   [4][3][2][1]
           Q0                 Q1                 Q2                      Q7

    Processing Flow:
    Iteration 1: P0{1,2,3,4} <--> Q0{32,31,30,29}
    Iteration 2: P1{5,6,7,8} <--> Q1{28,27,26,25}
    ...
    Iteration 8: P7{29,30,31,32} <--> Q7{4,3,2,1}

**Processing Algorithm (TP_DIM = 3 Example)**

For each iteration, only the first ``TP_DIM`` elements from each 4-element group are used in calculations. When TP_DIM = 3, the 4th element in each group is ignored. The process repeats for all TP_LEN vector pairs (8 iterations in this example).

**Calculation Process:**

::

    For each iteration i (0 to 7):

    1. Extract TP_DIM elements:     Pi[0:2] and Qi[0:2]
    2. Compute differences:         Diff = Qi - Pi  (element-wise)
    3. Square differences:          Squared = Diff²  (element-wise)
    4. Sum squared differences:     Sum = Σ(Squared)
    5. Apply square root:           Distance = √Sum  (if TP_IS_OUTPUT_SQUARED = 0)

**Concrete Examples:**

::

    D0: P0{1,2,3} vs Q0{32,31,30}  →  {31²+29²+27²}  →  2531  →  √2531 = 50.31
    D1: P1{5,6,7} vs Q1{28,27,26}  →  {23²+21²+19²}  →  1331  →  √1331 = 36.48
    D2: P2{9,10,11} vs Q2{24,23,22} → {15²+13²+11²}  →  515   →  √515  = 22.69
    ...
    D7: P7{29,30,31} vs Q7{4,3,2}  →  {-25²,-27²,-29²} → 2195  →  √2195 = 46.85

**Key Points:**

- **Element Selection:** Only first TP_DIM elements used (4th element ignored when TP_DIM = 3)
- **Vectorized Operations:** Differences, squaring, and summation use hardware vectorization
- **Output Control:** **TP_IS_OUTPUT_SQUARED** determines whether to apply square root

.. important::
   **Critical Memory Requirement:** ED expects input data of **P** and **Q** to have **TP_LEN × 4** samples, regardless of the user's dimension (i.e., ``TP_DIM``).

   This means:

   * Always allocate memory for 4 elements per vector group
   * Even if TP_DIM = 1, 2, or 3, you still need 4 elements per group
   * Total memory required: **TP_LEN × 4** samples for each input vector
   * Unused elements (when TP_DIM < 4) are ignored during calculation

Output Data
-----------

The **output data** has a length of ``TP_LEN``.

The output vector for the above input vectors P and Q is:

- ``TP_IS_OUTPUT_SQUARED = 1 (SQUARED OUTPUT):``

.. table:: outData_Squared
   :align: center

   +------+------+-----+-----+-----+-----+------+------+
   | ED0  | ED1  | ED2 | ED3 | ED4 | ED5 | ED6  | ED7  |
   +======+======+=====+=====+=====+=====+======+======+
   | 2531 | 1331 | 515 | 83  | 35  | 371 | 1091 | 2195 |
   +------+------+-----+-----+-----+-----+------+------+

- ``TP_IS_OUTPUT_SQUARED = 0 (ED OUTPUT):``

.. table:: outData_ED
   :align: center

   +---------+---------+---------+---------+---------+----------+---------+---------+
   |  ED0    | ED1     | ED2     | ED3     |  ED4    |  ED5     | ED6     | ED7     |
   +=========+=========+=========+=========+=========+==========+=========+=========+
   | 50.3099 | 36.4829 | 22.6936 | 9.11043 | 5.91608 | 19.26136 | 33.0303 | 46.8508 |
   +---------+---------+---------+---------+---------+----------+---------+---------+

IO Buffer Interface
-------------------

The Euclidean Distance operation can be performed via the IO Buffer interface on **AIE**, **AIE-ML**, and **AIE-MLv2** devices.

- ``TP_API = 0`` indicates an IO-buffer interface

Saturation
----------

Distortion caused by saturation is possible for Euclidean Distance computation. Since the input values are provided at runtime, no compile-time error can be issued for this hazard, so the user must ensure that saturation does not occur.

ED results: output is squared or standard
-----------------------------------------

The template parameter ``TP_IS_OUTPUT_SQUARED`` determines whether the output is squared (i.e., ``ED^2``) or standard ``ED``.

* ``TP_IS_OUTPUT_SQUARED`` = **0** indicates that the output is standard ED.
* ``TP_IS_OUTPUT_SQUARED`` = **1** indicates that the output is **squared** (i.e., ``ED^2``).

.. note::
   ``TP_IS_OUTPUT_SQUARED`` is 0 by default, meaning the output is always standard ED.

   +----------------------------+----------------------------------------+
   | TP_IS_OUTPUT_SQUARED       |      Output                            |
   +============================+========================================+
   |  0                         | e.g. **ED** = sqrt(Dx^2 + Dy^2 + Dz^2) |
   +----------------------------+----------------------------------------+
   |  1                         | e.g. **ED^2** = (Dx^2 + Dy^2 + Dz^2)   |
   +----------------------------+----------------------------------------+

Code Example
============

The following code example shows how the ``euclidean_distance_graph`` class can be used within a user super-graph.

.. literalinclude:: ../../../../L2/examples/docs_examples_2/test_euclidean_distance.hpp
    :language: cpp
    :lines: 17-

.. |trade| unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg| unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
