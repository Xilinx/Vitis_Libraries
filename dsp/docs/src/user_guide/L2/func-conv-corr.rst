..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _CONVOLUTION_CORRELATION:

=======================
Convolution/Correlation
=======================

This library element computes the convolution or correlation of two input vectors depending on the specified function type. The convolution and correlation library element has three modes: FULL, SAME, and VALID.
Template parameters are used to configure the top-level graph of the `conv_corr_graph` class.

Entry Point
===========

The graph entry point is as follows:

.. code-block::

    xf::dsp::aie::conv_corr::conv_corr_graph

Device Support
==============

The Convolution/Correlation library element supports AIE, AIE-ML and AIE-MLv2 devices for all features, with the following differences:

- The available round modes and the enumerated values of round modes are the same for AIE-ML and AIE-MLv2 devices, but differ from those for AIE devices. See :ref:`COMPILING_AND_SIMULATING`.

Supported Input Data Types
==========================

The data type for input ports F and G (`inF` and `inG`) is controlled by `T_DATA_F` and `T_DATA_G`, respectively.
Both **inputs** may take one of the following 8 choices: `int8`, `int16`, `int32`, `cint16`, `cint32`, `float`, `cfloat` and `bfloat16`.
The **output** may take one of the following 6 choices: `int16`, `int32`, `cint16`, `cint32`, `float`, and `cfloat`.
Please see the table :ref:`CONV_CORR_combos` for valid input/output data type combinations.

.. _CONV_CORR_combos:

.. table:: IO-BUFFER INTERFACE: Supported Combinations of Input/Output Data Types
   :align: center

   +------------------+------------------+------------------+------------------+------------------+------------------+
   | InputF Data Type | InputG Data Type | Output Data Type | AIE Valid        | AIE-ML Valid     | AIE-MLv2 Valid   |
   +==================+==================+==================+==================+==================+==================+
   | int8             | int8             | int16            | **no**           | yes              | yes              |
   +------------------+------------------+------------------+------------------+------------------+------------------+
   | int16            | int16            | int32            | yes              | yes              | yes              |
   +------------------+------------------+------------------+------------------+------------------+------------------+
   | int32            | int16            | int32            | yes              | yes              | yes              |
   +------------------+------------------+------------------+------------------+------------------+------------------+
   | cint16           | int16            | cint16           | yes              | yes              | yes              |
   +------------------+------------------+------------------+------------------+------------------+------------------+
   | cint16           | int16            | cint32           | yes              | yes              | yes              |
   +------------------+------------------+------------------+------------------+------------------+------------------+
   | cint16           | int32            | cint32           | yes              | yes              | yes              |
   +------------------+------------------+------------------+------------------+------------------+------------------+
   | cint16           | cint16           | cint32           | yes              | yes              | yes              |
   +------------------+------------------+------------------+------------------+------------------+------------------+
   | cint32           | int16            | cint32           | yes              | yes              | yes              |
   +------------------+------------------+------------------+------------------+------------------+------------------+
   | cint32           | cint16           | cint32           | yes              | yes              | yes              |
   +------------------+------------------+------------------+------------------+------------------+------------------+
   | float            | float            | float            | yes              | yes              | yes              |
   +------------------+------------------+------------------+------------------+------------------+------------------+
   | cfloat           | float            | cfloat           | yes              | **no**           | **no**           |
   +------------------+------------------+------------------+------------------+------------------+------------------+
   | cfloat           | cfloat           | cfloat           | yes              | **no**           | **no**           |
   +------------------+------------------+------------------+------------------+------------------+------------------+
   | bfloat16         | bfloat16         | float            | **no**           | yes              | yes              |
   +------------------+------------------+------------------+------------------+------------------+------------------+
   | 1. A mix of float and integer types is not supported.                                                           |
   | 2. A mix of float data type for the F signal and cfloat data type for the G signal is not supported.            |
   +-----------------------------------------------------------------------------------------------------------------+

.. table:: STREAM INTERFACE : Supported Combinations of Input/Output data types
   :align: center

   +------------------+------------------+------------------+------------------+------------------+------------------+
   | InputF Data Type | InputG Data Type | Output Data Type | AIE Valid        | AIE-ML Valid     | AIE-MLv2 Valid   |
   +==================+==================+==================+==================+==================+==================+
   | cint16           | cint16           | cint16           | yes              | no               | no               |
   +------------------+------------------+------------------+------------------+------------------+------------------+
   | cint16           | int16            | cint16           | yes              | no               | no               |
   +------------------+------------------+------------------+------------------+------------------+------------------+
   | Note: Stream-based implementation does not support other data types.                                            |
   |                                                                                                                 |
   +-----------------------------------------------------------------------------------------------------------------+

Template Parameters
===================

To see details on the template parameters for Convolution/Correlation, see :ref:`API_REFERENCE`.

Access Functions
================

To see details on the access functions for Convolution/Correlation, see :ref:`API_REFERENCE`.

Ports
=====

To see details on the ports for Convolution/Correlation, see :ref:`API_REFERENCE`.

Design Notes
============
The performance of the IP depends on the chosen data type combination :ref:`CONV_CORR_combos`. The number of multiplications per clock cycle will be updated based on the data type combination.
The Convolution/Correlation operation can be processed by both IO Buffer (`TP_API = 0`) and Stream-Based (`TP_API = 1`) interfaces, which are controlled by the parameter named ``TP_API``.

Input Buffer Length
--------------------
The input buffer length of F and G is controlled by ``TP_F_LEN`` and  ``TP_G_LEN`` respectively.

Output Buffer Length
--------------------

The output buffer length is calculated using the formula shown in the **OUT_BUFFER_LEN** table: ``ceil((TP_F_LEN + TP_G_LEN - 1), LANES)`` for FULL mode, ``ceil(TP_F_LEN, LANES)`` for SAME mode, and ``ceil((TP_F_LEN - TP_G_LEN + 1), LANES)`` for VALID mode.
Here, ``LANES`` is the number of parallel data lanes available in the AIE hardware, ensuring the buffer is always sized to the next multiple of the hardware lanes for efficient vector processing.

Formula for **ceil**:

.. code-block::

    ceil(a,b) ==> (((a+b-1)/b) * b)

.. _Out_Buffer_Len_and_Lanes_info:

.. table:: OUT_BUFFER_LEN
   :align: center

   +-----------------+--------------+--------------------------------------------+
   | TP_COMPUTE_MODE | MODE NAME    |      OUT_BUFFER_LEN                        |
   +=================+==============+============================================+
   | 0               | FULL         |  ceil((TP_F_LEN + TP_G_LEN - 1), LANES)    |
   +-----------------+--------------+--------------------------------------------+
   | 1               | SAME         |  ceil(TP_F_LEN, LANES)                     |
   +-----------------+--------------+--------------------------------------------+
   | 2               | VALID        |  ceil((TP_F_LEN - TP_G_LEN + 1), LANES)    |
   +-----------------+--------------+--------------------------------------------+
   Where:
       * ``TP_F_LEN`` is the length of input F vector.
       * ``TP_G_LEN`` is the length of input G vector.
       * ``LANES`` is the number of parallel data lanes available in the AIE hardware, which depends on the data type combination used. See :ref:`LANES

.. table:: LANES
   :align: center

   +------------------+------------------+------------------+------------------+------------------+--------------------+
   | InputF Data Type | InputG Data Type | Output Data Type | **AIE-1** Lanes  | **AIE-ML** Lanes | **AIE-MLv2** Lanes |
   +==================+==================+==================+==================+==================+====================+
   | int8             | int8             | int16            | **0**            | 32               | 64                 |
   +------------------+------------------+------------------+------------------+------------------+--------------------+
   | int16            | int16            | int32            | 16               | 16               | 32                 |
   +------------------+------------------+------------------+------------------+------------------+--------------------+
   | int32            | int16            | int32            | 8                | 16               | 32                 |
   +------------------+------------------+------------------+------------------+------------------+--------------------+
   | cint16           | int16            | cint16           | 8                | 16               | 32                 |
   +------------------+------------------+------------------+------------------+------------------+--------------------+
   | cint16           | int16            | cint32           | 8                | 16               | 32                 |
   +------------------+------------------+------------------+------------------+------------------+--------------------+
   | cint16           | int32            | cint32           | 8                | 16               | 16                 |
   +------------------+------------------+------------------+------------------+------------------+--------------------+
   | cint16           | cint16           | cint32           | 8                | 16               | 16                 |
   +------------------+------------------+------------------+------------------+------------------+--------------------+
   | cint32           | int16            | cint32           | 4                | 16               | 16                 |
   +------------------+------------------+------------------+------------------+------------------+--------------------+
   | cint32           | cint16           | cint32           | 4                | 8                | 16                 |
   +------------------+------------------+------------------+------------------+------------------+--------------------+
   | float            | float            | float            | 8                | 32               | 16                 |
   +------------------+------------------+------------------+------------------+------------------+--------------------+
   | cfloat           | float            | cfloat           | 4                | **0**            | **0**              |
   +------------------+------------------+------------------+------------------+------------------+--------------------+
   | cfloat           | cfloat           | cfloat           | 4                | **0**            | **0**              |
   +------------------+------------------+------------------+------------------+------------------+--------------------+
   | bfloat16         | bfloat16         | float            | **0**            | 16               | 64                 |
   +------------------+------------------+------------------+------------------+------------------+--------------------+

.. note:: Please refer to `UG1603 Number of Lanes supported by Sliding Multiplication <https://docs.amd.com/r/en-US/ug1603-ai-engine-ml-kernel-graph/Sliding-Multiplication>`_.

Example Config.,

 .. code-block::
   
   Data_F - ``int16``,
   Data_G - ``int16``,
   Data_Out - ``int32``,
   Func_Type = 1 (conv),
   compute_mode = 0 (FULL), 1 (SAME), 2 (VALID),
   F_LEN = 64,
   G_LEN = 32.

   in_F[``F_LEN``] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,............,64],
   in_G[``G_LEN``] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,......,32],
   
   **FULL Mode:**
      OUT_DATA_LEN = (TP_F_LEN + TP_G_LEN - 1) --> (64+32-1) --> **95**;
      LANES = ``16`` for ``int16xint16 data combo``
      Output_Buffer_len = ceil(95,16) --> (((95+16-1)/16)*16) --> ((110/16)*16) --> as (110/16) is integer divisible --> (6*16)--> **96**. So output_buffer has **95** valid output samples and **1** zero sample.

   **SAME Mode:**
      OUT_DATA_LEN = TP_F_LEN --> **64**;
      LANES = ``16`` for ``int16xint16 data combo``
      Output_Buffer_len = ceil(64,16) --> (((64+16-1)/16)*16) --> ((79/16)*16) --> as (79/16) is integer divisible --> (4*16)--> **64**. So output_buffer has **64** valid output samples.

   **VALID Mode:**
      OUT_DATA_LEN = (TP_F_LEN - TP_G_LEN + 1) --> (64-32+1) --> **33**;
      LANES = ``16`` for ``int16xint16 data combo``
      Output_Buffer_len = ceil(33,16) --> (((33+16-1)/16)*16) --> ((48/16)*16) --> as (48/16) is integer divisible --> (3*16)--> --> **48**. So output_buffer has **33** valid output samples and **15** zero samples.



IO Buffer Interface
-------------------
The Convolution/Correlation operation can be performed via the IO Buffer interface on **AIE**, **AIE-ML** and **AIE-MLv2** devices.

| ``TP_API`` - **0** indicates **IO-BUFFER** interface

The IO Buffer interface supports all data type combinations. See :ref:`CONV_CORR_combos` `table:: IO-BUFFER INTERFACE`.

Streaming Interface
-------------------
The Convolution/Correlation operation via streaming is supported only on AIE devices. Streaming support is **not available** on **AIE-ML** and **AIE-MLv2** devices.

| ``TP_API`` - **1** indicates **STREAM** interface.

* The input F signal is assumed to be streaming on the AXI-Stream interface. As F is consumed on streams, the selection of Length(F) can be higher to achieve higher throughput.
* The input G signal is assumed to be a window on the I/O buffer interface.
* `TP_COMPUTE_MODE` is fixed to VALID since padding is not required for the F signal, as F is always a continuous stream for both convolution and correlation.
* Typical use cases that can benefit from the stream interface kernel involve having a continuous stream of F, for which a convolution or correlation with a known pattern (e.g., G) must be computed.

   **Cascaded Kernels**
    - Number of kernels to be cascaded together to distribute the computation of convolution or corrleation is controlled by ``TP_CASC_LEN`` template parmaeter.
      The library provides access functions to determine the value of ``TP_CASC_LEN`` that gives you the optimum performance, i.e., the minimum number of kernels that can provide the maximum performance. More details can be found in :ref:`API_REFERENCE`.

   **Parallel Input/Output paths**
   - Input/Output samples are distributed over multiple parallel computation paths, which is controlled by ``TP_PHASES`` template parameter.

| Stream Processing: Num of Phases ``TP_PHASES`` can be increased only when each cascade stream has maximum data rate i.e with maximum cascade length (4)
|                    To achieve data rate less than 1GSPS, Cascade length parameter can be decreased in a single phase design.
|                    To achieve data rate more than 1GSPS, NUM_PHASES parameter should be increased by keeping
|                        the ``TP_CASC_LEN`` parameter to its maximum possible value (i.e. the value required to achieve 1GSPS when ``TP_PHASES`` is equal to 1)
|


   **STREAM OUTPUT:**

   The stream output operates on instantaneous samples from the continuous input stream without data memory movements or intermediate storage. 
   This approach prevents selective sample discarding while maintaining uninterrupted stream flow and continuous throughput performance. 
   :ref:`FIGURE_STREAM_IMPL_CONV_CORR` shows the transpose-form implementation of the convolution and correlation operations.

.. _FIGURE_STREAM_IMPL_CONV_CORR:
.. figure:: ./media/transpose_form_of_filter_implementation.png

- The stream output length is independent of the compute mode (VALID).
- When comparing the output with reference data, ensure that the first **M** samples are discarded from the stream output, where **M** is an integer that can be computed based on configuration parameters.
This discarding of M samples is necessary because the transpose-form implementation of convolution and correlation produces initial transient samples that do not represent valid results until the computation reaches steady state.

Computation of **M** samples to discard from the stream output:
---------------------------------------------------------------

   **M** denotes the number of samples to be discarded from the output stream.

   **For AIE Output:**
      * M = (TP_G_LEN - Delay + Offset)

   **For REF Output:**
      * M = (Offset + 1)

   where:
     * Delay = ( ((8 * **MacsPerCore** * **Points** / 2 - 3) * floor((``TP_CASC_LEN`` - 1) / **PhaseIncrement**) - (3 * (floor((**TP_CASC_LEN** - 1) / **PhaseIncrement** ) - 1)) * (**PhaseIncrement** - 1) - (3 * ((``TP_CASC_LEN`` - 1) % **PhaseIncrement**))) * **PhaseIncrement** - 1)

     * Offset = (3 * ``TP_PHASES`` * (**PhaseIncrement** - 1))

   where:
      * **MacsPerCore** = ceil(``TP_G_LEN`` / (**Lanes** * **Points** * ``TP_CASC_LEN`` * ``TP_PHASES``)).
      * **PhaseIncrement** = ``TP_PHASES`` / **StreamsPerCore**.

      where:
         * **StreamsPerCore** = 1 when **(TP_G_LEN > ((TP_PHASES * Lanes * Points) / 2))**, otherwise **StreamsPerCore** = 2.
         * **Lanes** and **Points** are parameters related to the computation of convolution/correlation of two vectors F and G. Please refer to the table below for lanes and points for supported data type combinations. See `table:: LANES and POINTS`.

.. table:: LANES and POINTS: Used by the stream-based conv_corr kernel
   :align: center

   +-------------------------+-----------+------------+
   | Data Type Combo         | **Lanes** | **Points** |
   +=========================+===========+============+
   | cint16 (F) x int16 (G)  | 4         |  4         |
   +-------------------------+-----------+------------+
   | cint16 (F) x cint16 (G) | 4         |  2         |
   +-------------------------+-----------+------------+

The streaming interface supports only two data type combinations. See :ref:`CONV_CORR_combos` `table:: STREAM INTERFACE`.

Scaling
-------
Scaling in Convolution/Correlation is controlled by the ``TP_SHIFT`` parameter, which describes the number of bits to shift the output to the right.
Float, cfloat, and bfloat16 implementations do not support scaling. ``TP_SHIFT`` must be set to '0'.

Saturation
----------
Distortion caused by saturation is possible in Convolution/Correlation. Since the input values are provided at construction time, no compile-time error can be issued for this hazard. It is the user's responsibility to ensure that saturation does not occur.

Run Time Parameter (RTP) for Vector Lengths
---------------------------------------------
Vector length of **G** (``G_LEN``) can be configured at runtime through a Runtime Programmable (RTP) asynchronous input port, which is programmed by the processor subsystem (PS) during execution.

RTP Port Structure
~~~~~~~~~~~~~~~~~~
The RTP port consists of **8 bytes total** containing two 4-byte integer values:

* **Bytes 0-3**: Reserved.
* **Bytes 4-7**: Contains ``G_LEN`` - **Active parameter** when dynamic G_LEN is enabled

.. table:: RTP Port Layout
   :align: center

   +--------------------+------------------+
   |  G_LEN Bytes (4-7) |    Bytes (0-3)   |
   +====================+==================+
   |    [ACTIVE]        |    [RESERVED]    |
   +--------------------+------------------+


RTP Configuration
~~~~~~~~~~~~~~~~~

.. important::
   **RTP Interface Availability:**
   
   - **IO-BUFFER Interface** (``TP_API = 0``): **RTP support is available** - Runtime parameter updates can be used to modify G_LEN dynamically
   - **STREAM Interface** (``TP_API = 1``): **RTP support is NOT available** - All parameters must be configured at compile-time via template parameters
   
   The RTP port for dynamic vector length configuration is only functional when using the IO-BUFFER interface. Stream-based implementations do not support runtime parameter modification.

The conv_corr implementation supports dynamic (i.e. run-time variable) ``G_LEN``. This design choice supports efficient computation of sample loading for AIE buffer operations.

Dynamic ``G_LEN`` behavior is controlled by the ``TP_USE_RTP_VECTOR_LENGTHS`` template parameter:

* ``TP_USE_RTP_VECTOR_LENGTHS = 0``: Both ``F_LEN`` and ``G_LEN`` are static (RTP port unused)
* ``TP_USE_RTP_VECTOR_LENGTHS = 1``: ``G_LEN`` becomes dynamic (RTP port active)

.. table:: RTP Configuration Summary
   :align: center

   +----------------------------+-----------+---------------+--------------------+
   | TP_USE_RTP_VECTOR_LENGTHS  | **F_LEN** | **G_LEN**     | **RTP Usage**      |
   +============================+===========+===============+====================+
   | 0                          | Static    | Static        |      Unused        |
   +----------------------------+-----------+---------------+--------------------+
   | 1                          | Static    | **Dynamic**   | Bytes 4-7 Active   |
   +----------------------------+-----------+---------------+--------------------+

.. note:: 
   **Important RTP Usage Guidelines:**
   
   - Bytes 0-3 of the RTP port are reserved for future use
   - Only bytes 4-7 (``G_LEN``) are actively processed when ``TP_USE_RTP_VECTOR_LENGTHS = 1``
   - When writing to the RTP port, all 8 bytes must be provided (reserved bytes may be set to zero)
   - The RTP port is only active when ``TP_USE_RTP_VECTOR_LENGTHS = 1``; otherwise it remains unused

Runtime Length Constraints
~~~~~~~~~~~~~~~~~~~~~~~~~~~
When using dynamic RTP parameters, users must ensure that runtime G length is provided within the supported range based on AIE architecture load capabilities:

.. important::
   **RTP Parameter Bounds**: Runtime G_LEN value is always bounded by its template parameter i.e., ``TP_G_LEN``.

   - Runtime G_LEN ≤ ``TP_G_LEN`` (template parameter)

   **Minimum Length Requirements**: The minimum values for F and G lengths are derived from the number of lanes that the AIE vector processor operates on in parallel. Each AIE architecture has different vector processing capabilities:
   
   - **AIE vector processor lanes** determine the minimum data load requirement for efficient parallel processing
   - Data lengths below these minimums cannot fully utilize the vector processing capabilities, leading to suboptimal performance
   - The minimum requirement ensures that at least one complete vector load can be processed per operation
   - Different data types have different lane counts (see :ref:`LANES` table), which directly influences the minimum length constraints

**Range<min max> of F and G Lengths:**

.. table:: Data Type Combinations with Architecture-Specific Runtime Length Constraints
   :align: center

   +------------------+------------------------+------------------------+------------------------+
   | Input Data Type  | **AIE Constraints**    | **AIE-ML**             | **AIE-MLv2**           |
   |                  |                        | **Constraints**        | **Constraints**        |
   +==================+===========+============+===========+============+===========+============+
   |                  | **Min**   | **Max**    | **Min**   | **Max**    | **Min**   | **Max**    |
   +------------------+-----------+------------+-----------+------------+-----------+------------+
   | int8             |    NA     |  NA        |  **64**   | **16384**  |  **128**  | **16384**  |
   +------------------+-----------+------------+-----------+------------+-----------+------------+
   | int16            |  **32**   | **4096**   |  **32**   | **8192**   |  **64**   | **8192**   |
   +------------------+-----------+------------+-----------+------------+-----------+------------+
   | int32            |  **16**   | **2048**   |  **16**   | **4096**   |  **32**   | **4096**   |
   +------------------+-----------+------------+-----------+------------+-----------+------------+
   | cint16           |  **16**   | **2048**   |  **16**   | **4096**   |  **32**   | **4096**   |
   +------------------+-----------+------------+-----------+------------+-----------+------------+
   | cint32           |  **8**    | **1024**   |  **8**    | **2048**   |  **16**   | **2048**   |
   +------------------+-----------+------------+-----------+------------+-----------+------------+
   | float            |  **16**   | **2048**   |  **16**   | **4096**   |  **32**   | **4096**   |
   +------------------+-----------+------------+-----------+------------+-----------+------------+
   | cfloat           |  **8**    | **1024**   |    NA     |  NA        |    NA     |  NA        |
   +------------------+-----------+------------+-----------+------------+-----------+------------+
   | bfloat16         |    NA     |  NA        |  **32**   | **8192**   |  **64**   | **8192**   |
   +------------------+-----------+------------+-----------+------------+-----------+------------+

.. note::
   **Performance Guidelines:**

   * **Minimum values** are determined by the number of parallel lanes the AIE vector processor can operate on simultaneously for each data type combination (see :ref:`LANES` table)
   * **Maximum values** are constrained by AIE architecture buffer size limitations  
   * Values below minimums may result in suboptimal performance as they cannot fully utilize the vector processing lanes
   * Values above maximums will exceed buffer capacity and cause compilation errors
   * The minimum requirement ensures efficient vector load operations aligned with the AIE's parallel processing capabilities

.. warning::
   **Template Parameter Bounds**: The runtime lengths provided via RTP must never exceed the compile-time template parameters:
   
   - If runtime G_LEN > ``TP_G_LEN``: **Buffer overflow and undefined behavior**
   
   Always ensure: ``runtime_length ≤ template_parameter`` for both F and G vectors.

Code Example
============

The following code examples shows how conv_corr graph class might be used within a user super-graph.

Convolution
-----------

.. literalinclude:: ../../../../L2/examples/docs_examples/test_conv.hpp
    :language: cpp
    :lines: 17-

Correlation
-----------

.. literalinclude:: ../../../../L2/examples/docs_examples/test_corr.hpp
    :language: cpp
    :lines: 17-

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:

