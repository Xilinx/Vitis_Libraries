..
   Copyright © 2019–2024 Advanced Micro Devices, Inc
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _FFT_IFFT_AIE_ONLY:


============================
FFT/IFFT 1CH DIT (AIE-only)
============================

This is a single channel, decimation in time (DIT) implementation. It has a configurable point size, data type, forward/reverse direction, scaling (as a shift), cascade length, static/dynamic point size, window size, interface API (stream/window), and parallelism factor.


Entry Point
===========

The graph entry point is the following:

.. code-block::

    xf::dsp::aie::fft::fft_ifft_dit_1ch_graph

Device Support
==============

The fft_ifft_dit_1ch supports AIE and AIE-ML devices. All features are supported on these variants with the following differences:

- ``TT_DATA`` and ``TT_TWIDDLE``. AIE-ML does not support cfloat type.
- ``TT_TWIDDLE``: AIE supports cint32. AIE-ML does not.
- ``TP_RND``: Supported round modes differ between AIE and AIE-ML as for all library elements.
- Number of ports: When configured for ``TP_API=1`` (stream IO), AIE will require 2 input ports (sample interleaved - even samples on the first port) and 2 output ports similarly interleaved for each lane of processing. The number of lanes is ``2^TP_PARALLEL_FACTOR``. AIE-ML accepts one stream only per kernel.

Supported Types
===============

The data type to the FFT is controlled by the ``TT_DATA`` template parameter. This can take one of three choices: cint16, cint32, or cfloat. This selection applies to both input data and output data. The template parameter ``TT_TWIDDLE`` can take one of three values, cint32, cint16 or cfloat. However, ``TT_DATA`` and ``TT_TWIDDLE`` must both be integer types or must both be cfloat.

Template Parameters
===================

To see details on the template parameters for the FFT, see :ref:`API_REFERENCE`.

For guidance on configuration with some example scenarios, see :ref:`FFT_CONFIGURATION_NOTES`

See also :ref:`PARAMETER_LEGALITY_NOTES` regarding legality checking of parameters.

.. note::  Window interfaces are now referred to as IO-buffers. IO-buffers are conceptually the same as windows. Graph connections between windows and IO-buffers are supported. More details on IO-buffers can be found in  `UG1079 Input and Output Buffers <https://docs.xilinx.com/r/en-US/ug1079-ai-engine-kernel-coding/Input-and-Output-Buffers>`_. For backwards compatibility, template parameters which refer to windows, e.g., ``TP_WINDOW_VSIZE``, remain unchanged.

Access Functions
================

To see details on the access functions for the FFT, see :ref:`API_REFERENCE`.

Ports
=====

To see details on the ports for the FFT, see :ref:`API_REFERENCE`. Note that the number and type of ports are determined by the configuration of template parameters.

Design Notes
============

Dynamic Point Size
------------------

The FFT supports dynamic (runtime controlled) point sizes. This feature is available when the ``TP_DYN_PT_SIZE`` template parameter is set. When set to 0 (static point size), all data will be expected in frames of ``TP_POINT_SIZE`` data samples, though multiple frames can be input together using ``TP_WINDOW_VSIZE``. When set to 1 (dynamic point size), each window must be preceded by a 256-bit header to describe the runtime parameters of that window. Note that ``TP_WINDOW_VSIZE`` described the number of samples in a window so does not include this header. The format of the header is described in Table 5. When ``TP_DYN_PT_SIZE`` =1, ``TP_POINT_SIZE`` describes the maximum point size which may be input.

.. _FFT_IFFT_HEADER_FORMAT:

.. table:: Header Format
   :align: center

   +-------------------------------+----------------------+---------------------------------------------------------------------------------+
   |                               | Location (TT_DATA    |                                                                                 |
   | Field name                    | sample)              | Description                                                                     |
   +===============================+======================+=================================================================================+
   |                               |                      |                                                                                 |
   | Direction                     | 0 (real part)        | 0 (inverse FFT) 1 (forward FFT)                                                 |
   +-------------------------------+----------------------+---------------------------------------------------------------------------------+
   |                               |                      |                                                                                 |
   | Point size (radix2 stages)    | 1 (real part)        | Point size described as a power of 2. E.g. 5 described a   point size of 32.    |
   +-------------------------------+----------------------+---------------------------------------------------------------------------------+
   |                               |                      |                                                                                 |
   | Reserved                      | 2                    | reserved                                                                        |
   +-------------------------------+----------------------+---------------------------------------------------------------------------------+
   |                               | 3 (cint32 or cfloat) |                                                                                 |
   | Status (output only)          | 7 (cint16)           |                                                                                 |
   |                               | (real part)          | 0 = legal point size, 1 = illegal point size                                    |
   +-------------------------------+----------------------+---------------------------------------------------------------------------------+

The locations are set to suit the ``TT_DATA`` type. That is, for ``TT_DATA=cint16``, direction is described in the first cint16 (real part) of the 256 bit header, and point size is described in the real part of the second cint16 value. Similarly, for ``TT_DATA=cint32``, the real part of the first cint32 value in the header holds the direction field and the real part of the second cint32 value holds the Point size (radix2) field.

Note that for ``TT_DATA=cfloat``, the values in the header are expected as cfloat and are value-cast (not reinterpret-cast) to integers internally. The output window also has a header. This is copied from the input header except for the status field, which is inserted. The status field is ignored on input. If an illegal point size is entered, the output header will have this field set to a non-zero value and the remainder of the output window is undefined.

Super Sample Rate Operation
---------------------------

While the term Super Sample Rate strictly means the processing of more than one sample per clock cycle, in the AIE context, it is taken to mean an implementation using parallel kernels to improve performance at the expense of additional resource use. In the FFT, SSR operation is controlled by the ``TP_PARALLEL_POWER`` template parameter. This parameter is intended to improve performance and also allow support of point sizes beyond the limitations of a single tile. Diagram :ref:`FIGURE_FFT_CONSTRAINTS` shows an example graph with ``TP_PARALLEL_POWER`` set to 2. This results in four subframe processors in parallel each performing an FFT of ``N/2^TP_PARALLEL_POWER`` point size. These subframe outputs are then combined by ``TP_PARALLEL_POWER`` stages of radix2 to create the final result. The order of samples is described in the note for ``TP_API`` above.

The ``TP_PARALLEL_POWER`` parameter  allows a trade of performance for resource use in the form of tiles used. The following table shows the tile utilization versus ``TP_PARALLEL_POWER`` assuming that all widgets co-habit with FFT processing kernels.

.. table:: FFT Resource Usage
   :align: center

   +-------------------+------------------+
   | TP_PARALLEL_POWER | Number of Tiles  |
   +===================+==================+
   |         0         |        1         |
   +-------------------+------------------+
   |         1         |        4         |
   +-------------------+------------------+
   |         2         |       12         |
   +-------------------+------------------+
   |         3         |       32         |
   +-------------------+------------------+
   |         4         |       80         |
   +-------------------+------------------+

Super Sample Rate Sample to Port Mapping
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When the Super Sample Rate operation is used, data is input and output using multiple ports. These multiple ports on input or output act as one channel. The mapping of samples to ports is that each successive sample should be passed to a different port in a round-robin fashion, e.g., with ``TP_PARALLEL_POWER`` set to 1, sample 0 should be sent to input port 0, sample 1 to input port 1, sample 2 to input port 2, sample 3 to input port 0, and so on.
Super Sample Rate operation (``TP_PARALLEL_POWER``>0) supports both  IO-buffer IO (``TP_API`` == 0) or stream input (``TP_API`` == 1). For IO-buffer IO, there is one port per lane of processing (number of lanes= 2^ ``TP_PARALLEL_POWER``). For stream IO the number of ports is the number of lanes multiplied by the number of streams per tile (2 for AIE, 1 for AIE-ML).

.. table:: FFT Number of Ports Examples
   :align: center

   +-------------------+------------------+------------------+------------------+------------------+
   | TP_PARALLEL_POWER | Number of Ports                                                           |
   +-------------------+------------------+------------------+------------------+------------------+
   |                   | AIE                                 | AIE-ML                              |
   +-------------------+------------------+------------------+------------------+------------------+
   |                   | Stream(note 1)   |      Buffer      |      Stream      |      Buffer      |
   +===================+==================+==================+==================+==================+
   |         0         |        2         |        1         |        1         |        1         |
   +-------------------+------------------+------------------+------------------+------------------+
   |         1         |        4         |        2         |        2         |        2         |
   +-------------------+------------------+------------------+------------------+------------------+
   |         2         |        8         |        4         |        4         |        4         |
   +-------------------+------------------+------------------+------------------+------------------+
   |         3         |       16         |        8         |        8         |        8         |
   +-------------------+------------------+------------------+------------------+------------------+
   |         4         |       32         |       16         |       16         |       16         |
   +-------------------+------------------+------------------+------------------+------------------+
   | 1. AIE device has two streams available on each AIE tile.                                     |
   |    FFT utilizes both streams in Super Sample Rate mode.                                       |
   +-------------------+------------------+------------------+------------------+------------------+

Scaling
-------

Scaling in the FFT is controlled by the ``TP_SHIFT`` parameter which describes how many binary places by which to shift the result to the right, i.e., only power-of-2 scaling values are supported. The FFT implementation does not implement the 1/N scaling of an IFFT directly, but this can be configured via ``TP_SHIFT``. Internal to the FFT, for cint16 and cint32 data, a data type of cint32 is used for temporary value. After each rank, the values are scaled by only enough to normalize the bit growth caused by the twiddle multiplication (i.e., 15 bits), but there is no compensation for the bit growth of the adder in the butterfly operation. No scaling is applied at any point when the data type is cfloat. Setting ``TP_SHIFT`` to any value other than 0 when ``TT_DATA`` is cfloat will result in an error. In the case of ``TP_PARALLEL_POWER > 0`` for cint16, the streams carrying data between subframe processors, and the combiner stages carry cint16 data so as to allow for high performance. In this case, the scaling value applied to each subframe processor is (``TP_SHIFT-TP_PARALLEL_POWER``) (if positive and 0 if not). Each combiner stage will have a shift of 1 is applied, to compensate for the bit growth of 1 in the stage's butterfly, if there is adequate ``TP_SHIFT`` to allow for this, or 0 if there is not. For example, with an FFT configured to be ``POINT_SIZE=1024``, ``DATA_TYPE=cint16``, ``PARALLEL_POWER=2`` and ``TP_SHIFT=10``, there will be four subframe processors and two further ranks of four combiners. The four subframe processors will all have a local ``TP_SHIFT`` of 10-2 = 8 applied, and each of the combiners will have a local ``TP_SHIFT`` of 1 applied. This scheme is designed to preserve as much accuracy as possible without compromising performance. If better accuracy or noise performance is required, this can be achieved at the expense of throughput by using ``TT_DATA=cint32``.

Rounding and Saturation
-----------------------

In the final stage, the final values are converted to ``TT_DATA`` using ``TP_SHIFT``, ``TP_RND``, and ``TP_SAT``. ``TP_SHIFT`` performs the scaling as described elsewhere. ``TP_RND`` and ``TP_SAT`` determine the form of rounding and saturation applied on the downshifted value. The following tables describe the form of rounding and of saturation performed.

.. _fft_rnd_and_sat:

.. table:: Rounding and Saturation in FFT
   :align: center

   +------------------------+----------------+-----------------+----------------------------------+
   | Parameter Name         |    Type        |  Description    |    Range                         |
   +========================+================+=================+==================================+
   |    TP_RND              |    Unsigned    | Round mode      |    0 to 3 are not supported.     |
   |                        |    int         |                 |    These modes perform floor or  |
   |                        |                |                 |    ceiling operations which lead |
   |                        |                |                 |    to large errors on output     |
   |                        |                |                 |                                  |
   |                        |                |                 |    4 = ``rnd_sym_inf``           |
   |                        |                |                 |    symmetrical                   |
   |                        |                |                 |    to infinity (AIE)             |
   |                        |                |                 |                                  |
   |                        |                |                 |    5 = ``rnd_sym_zero``          |
   |                        |                |                 |    symmetrical                   |
   |                        |                |                 |    to zero   (AIE)               |
   |                        |                |                 |                                  |
   |                        |                |                 |    6 = ``rnd_conv_even``         |
   |                        |                |                 |    convergent                    |
   |                        |                |                 |    to even (AIE)                 |
   |                        |                |                 |                                  |
   |                        |                |                 |    12 = ``rnd_conv_even``        |
   |                        |                |                 |    convergent                    |
   |                        |                |                 |    to even (AIE-ML)              |
   |                        |                |                 |                                  |
   |                        |                |                 |    7 = ``rnd_conv_odd``          |
   |                        |                |                 |    convergent                    |
   |                        |                |                 |    to odd (AIE)                  |
   |                        |                |                 |                                  |
   |                        |                |                 |    13 = ``rnd_conv_odd``         |
   |                        |                |                 |    convergent                    |
   |                        |                |                 |    to odd      (AIE)             |
   |                        |                |                 |                                  |
   |                        |                |                 |    8= ``rnd_neg_inf``            |
   |                        |                |                 |    to negative                   |
   |                        |                |                 |    infinity    (AIE-ML)          |
   |                        |                |                 |                                  |
   |                        |                |                 |    9 = ``rnd_pos_inf``           |
   |                        |                |                 |    to positive                   |
   |                        |                |                 |    infinity    (AIE-ML)          |
   |                        |                |                 |                                  |
   |                        |                |                 |    10 = ``rnd_sym_zero``         |
   |                        |                |                 |    to zero     (AIE-ML)          |
   |                        |                |                 |                                  |
   |                        |                |                 |    11 = ``rnd_sym_inf``          |
   |                        |                |                 |    to infinity (AIE-ML)          |
   +------------------------+----------------+-----------------+----------------------------------+
   |    TP_SAT              |    Unsigned    | Saturation mode |    0 = ``unsaturated``           |
   |                        |    int         |                 |                                  |
   |                        |                |                 |    1 = ``asymmetric saturation`` |
   |                        |                |                 |    i.e +2^(N-1)-1 to -2^(N-1)    |
   |                        |                |                 |    e.g. +32767 to -32768         |
   |                        |                |                 |                                  |
   |                        |                |                 |    3 = ``symmetric saturation``  |
   |                        |                |                 |    i.e +2^(N-1)-1 to -2^(N-1)+1  |
   |                        |                |                 |    e.g., +32767 to -32767        |
   |                        |                |                 |                                  |
   +------------------------+----------------+-----------------+----------------------------------+

Distortion caused by saturation will be possible for certain configurations of the FFT. For instance, with ``TT_DATA=cint32``, it is possible for the sample values within the FFT to grow beyond the range of int32 values due to bit growth in the FFT algorithm. Saturation is applied at each stage (rank). In the final stage when ``TP_SHIFT`` is applied, saturation is also applied according to ``TP_SAT``. Similarly, if the FFT is configured for ``TT_DATA=cint16``, but insufficient scaling (TP_SHIFT) is applied, then sample values can exceed the range of int16 and so these too will be saturated in the final stage. For ``TT_DATA=cfloat``, the FFT performs no scaling, nor saturation. Any saturation effects will be due to the atomic float operations returning positive infinity, negative infinity, or NaN.

Cascade Feature
---------------

The FFT is configured using the ``TP_CASC_LEN`` template parameter. This determines the number of kernels over which the FFT function (or subframe FFT in the case of ``TP_PARALLEL_POWER>0``) is split. To be clear, this feature does not use the cascade ports of kernels to convey any data. IO-buffers are used to convey data from one kernel to the next in the chain. The term cascade is used simply in the sense that the function is split into a series of operations which are executed by a series of kernels, each on a separate tile. The FFT function is only split at stage boundaries, so the ``TP_CASC_LEN`` value cannot exceed the number of stages for that FFT.

Twiddle mode
------------

For integer types of ``TT_TWIDDLE``, the twiddle values are stored internally in lookup tables as their true value shifted right until the sign bit (2s complement) is in the most significant position of the data type (int16 or int32). However, 2s complement is an asymmetric range, which means the twiddles close to point 1+j0 are saturated. This can lead to spectral leakage. The library unit therefore offers ``TP_TWIDDLE_MODE``, which, when set to 0 will have twiddle values as described above, but when set to 1 will have twiddle values of half this magnitude, so that saturation does not occur. This will have higher noise than for the other mode, but does not exhibit the spectral leakage.

Constraints
-----------

The FFT design has large memory requirements for data buffering and twiddle storage. Constraints might be necessary to fit a design or to achieve high performance, such as ensuring FFT kernels do not share tiles with other FFT kernels or user kernels. To apply constraints, you must know the instance names of the internal graph hierarchy of the FFT. See :ref:`FIGURE_FFT_CONSTRAINTS`.

.. _FIGURE_FFT_CONSTRAINTS:

.. figure:: ./media/X25897.png

   **Applying Design Constraints**

Location and other constraints can be applied in the parent graph which instances the FFT graph class. To apply a constraint, you will need to know the name of the kernel, which will include the hierarchial path to that kernel. The simplest way to derive names, including the hierarchial part, is to compile a design and open it in AMD Vitis |trade| , using the graph view. The names of all kernels and memory buffers can be obtained from there. These names can then be back-annotated to the parent graph to apply the necessary constraint.

The FFT graph class is implemented as a recursion of the top level to implement the parallelism. The instance names of each pair of subgraphs in the recursion are FFTsubframe(0) and FFTsubframe(1). In the final level of recursion, the FFT graph will contain an instance of either FFTwinproc (for ``TP_API = 0`` ) or FFTstrproc (when ``TP_API=1`` ). Within this level there is an array of kernels called m_fftKernels which will have ``TP_CASC_LEN`` members. In the above diagram, widgets are shown in green and red. The widgets either receive two streams and interlace these streams to form an IO-buffer of data on which the FFT operates, or take the IO-buffer output from the FFT and deinterlace this into two streams. The widgets might be expressed as separate kernels (``TP_USE_WIDGETS`` = 1), and hence then placed on separate tiles, or might be expressed as functions internal to the FFT kernel (or combiner kernel) (``TP_USE_WIDGETS`` = 0) for improved performance compared to one tile hosting both the FFT kernel and associated widgets. See also :ref:`FFT_CONFIGURATION_NOTES`.

In release 2023.1, widget kernels which converted from dual streams to IO-buffers and vice versa were blended with the parent FFT or combiner kernels they supported. This eliminated kernel-switch overheads and so improved performance versus the case where widgets were co-located with their parent FFT or combiner. However, this prevented you from splitting each trio of kernels over multiple tiles for even higher performance albeit at a trebling of the resource cost. In 2023.2, the choice of whether to express the widgets as standalone kernels, or to blend them with the FFT or combiner they serve, has been added as ``TP_USE_WIDGETS``. The following description applies to the configuration with widget kernels, but the principles of the recursive decomposition and the names of the FFT and FFT combiner kernels remain and apply in either case.
The stream to window conversion kernels on input and output to the FFT subframes are at the same level as m_fftKernels and are called m_inWidgetKernel and m_outWidgetKernel respectively. Each level of recursion will also contain an array of radix2 combiner kernels and associated stream to window conversion kernels. These are seen as a column of kernels in the above figure. Their instance names are m_r2Comb[] for the radix2 combiners and m_combInKernel[] and m_combOutKernel[] for the input and output widget kernels respectively.

Examples of constraints: For ``TP_PARALLEL_POWER=2``, to set the runtime ratio of the third of four subframe FFTs, the constraint could look like this:

.. code-block::

  runtime<ratio>(myFFT.FFTsubframe[1].FFTsubframe[0].FFTstrproc.m_kernels[0]) = 0.9; //where myFFT is the instance name of the FFT in your design.

For the same example, to ensure that the second radix2 combiner kernel in the first column of combiners and its input widget do not share a tile, the constraint could look like this:

.. code-block::

	not_equal(location<kernel>(myFFT.FFTsubframe[0].m_combInKernel[1]),location<kernel>( myFFT.FFTsubframe[0].m_r2Comb[1]));

For large point sizes, e.g., 65536, the design is large, requiring 80 tiles. With such a large design, the Vitis AIE mapper might time out due to there being too many possibilities of placement, so placement constraints are recommended to reduce the solution space. Reduce the time spent by the Vitis AIE mapper tool to find a solution. Example constraints have been provided in the ``test.hpp`` file for the fft_ifft_dit_1ch, i.e, in: `L2/tests/aie/fft_ifft_dit_1ch/test.hpp`.

Use of single_buffer
--------------------

When configured for ``TP_API=0``, i.e., IO-buffer API, the FFT will default to use ping-pong buffers for performance. However, for the FFT, the resulting buffers can be very large and can limit the point size achievable by a single kernel. It is possible to apply the single_buffer constraint to the input and/or output buffers to reduce the memory cost of the FFT, though this will come at the cost of reduced performance. By this means an FFT with ``TT_DATA=cint16`` of ``TP_POINT_SIZE=4096`` can be made to fit in a single kernel. The following code shows how such a constraint can be applied.

.. code-block::

    xf::dsp::aie::fft::dit_1ch::fft_ifft_dit_1ch_graph<DATA_TYPE, TWIDDLE_TYPE, POINT_SIZE, FFT_NIFFT, SHIFT, CASC_LEN,
                                                       DYN_PT_SIZE, WINDOW_VSIZE, API_IO, PARALLEL_POWER, TWIDDLE_MODE>
                                                       fftGraph;
    single_buffer(fftGraph.FFTwinproc.m_fftKernels[0].in[0]);


Code Example
============

The following code block shows example code of how to include an instance of the fft_ifft_dit_1ch graph in a super-graph and also how the constraints might be applied to kernels within the FFT graph. In this example, not all kernels within the fft_ifft_dit_1ch graph are subject to location constraints. It is sufficient for the mapper to find a solution in this case by constraining only the r2comb kernels.

.. literalinclude:: ../../../../L2/examples/docs_examples/test_fft.hpp
    :language: cpp
    :lines: 17-

.. _FFT_CONFIGURATION_NOTES:

Configuration Notes
===================

This section is intended to provide guidance on how best to configure the FFT in some typical scenarios or when designing with one particular metric in mind, such as resource use or performance.

Configuration for Performance Versus Resource
---------------------------------------------

Simple configurations of the FFT use a single kernel. Multiple kernels will be used when either ``TP_PARALLEL_POWER > 0`` or ``TP_CASC_LEN > 1`` or ``TP_USE_WIDGETS = 1`` (with stream IO or ``TP_PARALLEL_POWER>0``). All of these parameters exist to allow higher throughput, though ``TP_PARALLEL_POWER`` also allows larger point sizes than can be implemented in a single kernel. If a higher throughput is required than what can be achieved with a single kernel, then ``TP_CASC_LEN`` should be increased in preference to ``TP_PARALLEL_POWER``. This is because resource (number of kernels) will match ``TP_CASC_LEN``, whereas for ``TP_PARALLEL_POWER``, resource increases quadratically. It is recommended that ``TP_PARALLEL_POWER`` is only increased after ``TP_CASC_LEN`` has been increased but where throughput still needs to be increased.
Of course, ``TP_PARALLEL_POWER`` might be required if the point size required is greater than a single kernel can be achieved. In this case, to keep resource minimized, increase ``TP_PARALLEL_POWER`` as required to support the point size in question, then increase ``TP_CASC_LEN`` to achieve the required throughput, before again increasing ``TP_PARALLEL_POWER`` if higher throughput is still required. ``TP_USE_WIDGETS`` can be used when either ``TP_API= 1`` (stream IO to the FFT) or ``TP_PARALLEL_POWER>0`` because for this, widgets are used with streams for the internal trellis connections. By default, ``TP_USE_WIDGETS=0`` which means that the FFT with stream input will convert the incoming stream(s) to an IO-buffer as a function of the FFT kernel and similarly for the output IO-buffer to streams conversion. Setting ``TP_USE_WIDGETS=1`` will mean that these conversion functions are separate kernels. The use of runtime<ratio> or location constraints can then be used to force these widget kernels to be placed on different tiles to their parent FFT kernel and so boost performance. The resource cost will rise accordingly. If the performance achieved using ``TP_CASC_LEN`` alone is close to that required (e.g., within about 20%), then ``TP_USE_WIDGETS`` might help reach the required performance with only a modest increase in resources and so should be used in preference to ``TP_PARALLEL_POWER``. Using ``TP_USE_WIDGETS`` in conjunction with ``TP_PARALLEL_POWER>0`` can lead to a significant increase in tile use (up to 3x). The maximum point size supported by a single kernel can be increased by use of the single_buffer constraint. This only applies when ``TP_API=0`` (windows) as the streaming implementation always uses single buffering.

Scenarios
---------

**Scenario 1:**

 512 point forward FFT with cint16 data requires >500 MSa/sec with a window interface and minimal latency. With ``TP_CASC_LEN=1`` and ``TP_PARALLEL_POWER=0``, this is seen to achieve approx 419 Msa/sec. With ``TP_CASC_LEN=2``, this increases to 590 Msa/s. The configuration will be as follows:

.. code-block::

   xf::dsp::aie::fft::dit_1ch::fft_ifft_dit_1ch_graph<cint16, cint16, 512, 1, 9, 2, 0, 512, 0, 0> myFFT;

.. note:: ``TP_SHIFT`` is set to 9 for nominal 1/N scaling. ``TP_WINDOW_VSIZE`` has been set to ``TP_POINT_SIZE`` to minimize latency.

**Scenario 2:**

4096 point inverse FFT with cint32 data is required with 100 Msa/sec. This cannot be accommodated in a single kernel due to memory limits. These memory limits apply to cascaded implementations too, so the recommended configuration is as follows:

.. code-block::

   xf::dsp::aie::fft::dit_1ch::fft_ifft_dit_1ch_graph<cint32, cint16, 4096, 0, 12, 1, 0, 4096, 1, 1> myFFT;

.. note:: ``TP_SHIFT`` is set to 12 for nominal 1/N scaling. ``TP_WINDOW_VSIZE`` has been set to ``TP_POINT_SIZE`` because to attempt any multiple of ``TP_POINT_SIZE`` would exceed memory limits.

.. _PARAMETER_LEGALITY_NOTES:

Parameter Legality Notes
========================

Where possible, illegal values for template parameters, or illegal combinations of values for template parameters are detected at compilation time. Where an illegal configuration is detected, compilation will fail with an error message indicating the constraint in question. However, no attempt has been made to detect and error upon configurations which are simply too large for the resource available, as the library element cannot know how much of the device is used by the user code and also because the resource limits vary by device which the library unit cannot deduce. In these cases, compilation will likely fail, but due to the over-use of a resource detected by the AIE tools. For example, an FFT of ``TT_DATA = cint16`` can be supported up to ``TP_POINT_SIZE=65536`` using ``TP_PARALLEL_POWER=4``. A similarly configured FFT with ``TT_DATA=cint32`` will not compile because the per-tile memory use, which is constant and predictable, is exceeded. This condition is detected and an error would be issued. A FFT with ``TT_DATA=cint32`` and ``TP_PARALLEL_POWER=5`` should, in theory, be possible to implement, but this will use 192 tiles directly and will use the memory of many other tiles, so is likely to exceed the capacity of the AIE array. However, the available capacity cannot easily be determined, so no error check is applied here.

The largest point size which can be supported in a single kernel is limited by data memory availability. Since IO-buffer connections default to double buffering for maximal throughput, the choice of ``TP_API`` (IO-buffer or streams) affects the maximum point size, because the limit will be reached for IO-buffers for a lower ``TP_POINT_SIZE`` than for streams. The following table indicates the maximum point size possible for a single kernel for various values of ``TT_DATA`` and ``TP_API``.

.. table:: Maximum Point Size in a Single Kernel
   :align: center

   +-------------------+-------------------------+-----------------------+
   | TT_DATA           | Max Point Size                                  |
   |                   +-------------------------+-----------------------+
   |                   | TP_API=0 (buffer I/O)   | TP_API=1 (stream I/O) |
   +===================+=========================+=======================+
   |    cint16         |       2048              |        4096           |
   +-------------------+-------------------------+-----------------------+
   |    cint32         |       2048              |        4096           |
   +-------------------+-------------------------+-----------------------+
   |    cfloat         |       2048              |        2048           |
   +-------------------+-------------------------+-----------------------+

The maximum point size supported per kernel puts a practical limit on the maximum point size supported when using ``TP_PARALLEL_POWER>1``. This is because the largest devices available currently support a maximum ``TP_PARALLEL_POWER`` of 4. The largest possible FFT can be found by multiplying the values in the table by 2^4. For example, the largest practical FFT with stream IO and ``cint16`` data is 4096 << 4 = 65536. However, the extensive use of neighboring tile RAM makes placement a challenge the the mapper, so 32768 may be a practical upper limit for ``cint32``.

.. |image1| image:: ./media/image1.png
.. |image2| image:: ./media/image2.png
.. |image3| image:: ./media/image4.png
.. |image4| image:: ./media/image2.png
.. |image6| image:: ./media/image2.png
.. |image7| image:: ./media/image5.png
.. |image8| image:: ./media/image6.png
.. |image9| image:: ./media/image7.png
.. |image10| image:: ./media/image2.png
.. |image11| image:: ./media/image2.png
.. |image12| image:: ./media/image2.png
.. |image13| image:: ./media/image2.png
.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim: