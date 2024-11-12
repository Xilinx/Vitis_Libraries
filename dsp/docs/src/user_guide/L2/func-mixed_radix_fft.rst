..
   Copyright © 2019–2024 Advanced Micro Devices, Inc
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _MIXED_RADIX_FFT:

===============
Mixed Radix FFT
===============

This library element implements an FFT or Inverse FFT of a point size which is not a power of 2 only but is a product of a power of 2, a power of 3, and a power of 5.

Entry Point
===========

The graph entry point is the following:

.. code-block::

    xf::dsp::aie::fft::mixed_radix_fft_graph

Device Support
==============
The mixed_radix_fft supports AIE and AIE-ML devices.

All features are supported on these variants with minor differences as follows:

- ``TP_POINT_SIZE``: The width of vectors used for calculations differs by AIE variant. The point size ``TP_POINT_SIZE`` must be a multiple of the number of samples processed in an atomic vectorized butterfly operation. This is 16 for AIE and 32 for AIE-ML.
- ``TP_RND``: Supported round modes differ between AIE and AIE-ML devices as for all library elements.
- Number of ports: When configured for ``TP_API=1`` (stream IO), AIE will require 2 input ports (sample interleaved - even samples on the first port) and 2 output ports similarly interleaved. AIE-ML accepts one stream only.
- ``TP_DYN_PT_SIZE``: the dynamic (run-time) point-size feature is supported on AIE only.

Supported Types
===============

The data type to the mixed radix FFT is controlled by the ``TT_DATA`` template parameter. This can take one of two choices: cint16 or cint32. ``TT_DATA`` determined the data type of both the input data and output data.
The ``TT_TWIDDLE`` template parameter is constrained by ``TT_DATA`` and so currently must be set to cint16.

Template Parameters
===================

To see details on the template parameters for the Mixed Radix FFT, see :ref:`API_REFERENCE`.

Access Functions
================

To see details on the access functions for the Mixed Radix FFT, see :ref:`API_REFERENCE`.

Ports
=====

To see details on the ports for the Mixed Radix FFT, see :ref:`API_REFERENCE`.

.. note:: The number and type of ports are determined by the configuration of template parameters.

Design Notes
============

The Mixed Radix FFT performs an FFT or inverse FFT on a frame of data of point size N where N = 2^A * 3^B * 5^C. The minimum value of A is 4, which means all supported point sizes are a multiple of 16. The maximum point size is determined by the amount of memory available to an AIE tile, so it will be in the region of 2048 for cint32. Point size support is currently limited to 3300.

Dynamic Point Size
------------------

Dynamic point size is selected using ``TP_DYN_PT_SIZE`` = 1. When set, the library unit has two input ports and two output ports. There is a header input port
and a data input port. Similarly there is an output header port and an output data port.
The header for both input and output is a fixed size of 256bits considered as 8 fields each of int32 type.
The fields hold the following information:

.. table:: Dynamic point size data header in Mixed Radix FFT
   :align: center

   +---------------+-------------------------------------------------------------------+
   | Field index   |    Description                                                    |
   +===============+===================================================================+
   |    0          |    Direction. Set this to 0 for inverse FFT and 1 for forward FFT |
   +---------------+-------------------------------------------------------------------+
   |    1          |    Not used                                                       |
   +---------------+-------------------------------------------------------------------+
   |    2          |    N, where the point size is 2^N * 3^P * 5^Q                     |
   +---------------+-------------------------------------------------------------------+
   |    3          |    P, where the point size is 2^N * 3^P * 5^Q                     |
   +---------------+-------------------------------------------------------------------+
   |    4          |    Q, where the point size is 2^N * 3^P * 5^Q                     |
   +---------------+-------------------------------------------------------------------+
   |    5          |    Not used                                                       |
   +---------------+-------------------------------------------------------------------+
   |    6          |    Not used                                                       |
   +---------------+-------------------------------------------------------------------+
   |    7          |    Error flag (set on output if the pointsize is invalid)         |
   +---------------+-------------------------------------------------------------------+



Super Sample Rate Operation
---------------------------

The Super Sample Rate is commonly understood as a sample rate greater than the system clock. In the context of AIE kernels, which are inherently SSR, the term SSR refers to execution using multiple kernels in parallel.

The Mixed Radix FFT does not currently support implementations using multiple kernels in parallel to execute the FFT.

Scaling
-------

Scaling in the Mixed Radix FFT is controlled by the ``TP_SHIFT`` parameter which describes how many binary places by which to shift the result to the right, i.e., only power-of-2 scaling values are supported. Scaling is applied after the last rank has been computed. Internal calculations are unscaled, so a data type of cint32 is used internally to allow for bit-growth from an input of cint16. No larger type than cint32 is available, so if ``TT_DATA`` is cint32, you must avoid arithmetic overflow by ensuring that the data value range on the input is restricted to prevent overflow.

Rounding and Saturation
-----------------------

In the final stage, the final values are converted to ``TT_DATA`` using ``TP_SHIFT``, ``TP_RND``, and ``TP_SAT``. ``TP_SHIFT`` performs the scaling as described previously. ``TP_RND`` and ``TP_SAT`` determine the form of rounding and saturation applied on the downshifted value. The following tables describe the form of rounding and of saturation performed.

.. _mixed_radix_fft_rnd_and_sat:

.. table:: Rounding and Saturation in Mixed Radix FFT
   :align: center

   +------------------------+----------------+-----------------+---------------------------------+
   | Parameter Name         |    Type        |  Description    |    Range                        |
   +========================+================+=================+=================================+
   |    TP_RND              |    Unsigned    | Round mode      |    0 to 3 are not supported.    |
   |                        |    int         |                 |    These modes perform floor or |
   |                        |                |                 |    ceiling operations which lead|
   |                        |                |                 |    to large errors on output    |
   |                        |                |                 |                                 |
   |                        |                |                 |    4 = ``rnd_sym_inf``          |
   |                        |                |                 |    symmetrical                  |
   |                        |                |                 |    to infinity                  |
   |                        |                |                 |                                 |
   |                        |                |                 |    5 = ``rnd_sym_zero``         |
   |                        |                |                 |    symmetrical                  |
   |                        |                |                 |    to zero                      |
   |                        |                |                 |                                 |
   |                        |                |                 |    6 = ``rnd_conv_even``        |
   |                        |                |                 |    convergent                   |
   |                        |                |                 |    to even                      |
   |                        |                |                 |                                 |
   |                        |                |                 |    7 = ``rnd_conv_odd``         |
   |                        |                |                 |    convergent                   |
   |                        |                |                 |    to odd                       |
   +------------------------+----------------+-----------------+---------------------------------+
   |    TP_SAT              |    Unsigned    | Saturation mode |    0 = ``unsaturated``          |
   |                        |    int         |                 |                                 |
   |                        |                |                 |    1 = ``asymmetric saturation``|
   |                        |                |                 |    i.e +2^(N-1)-1 to -2^(N-1)   |
   |                        |                |                 |    e.g. +32767 to -32768        |
   |                        |                |                 |                                 |
   |                        |                |                 |    3 = ``symmetric saturation`` |
   |                        |                |                 |    i.e +2^(N-1)-1 to -2^(N-1)+1 |
   |                        |                |                 |    e.g., +32767 to -32767       |
   |                        |                |                 |                                 |
   +------------------------+----------------+-----------------+---------------------------------+

Distortion caused by saturation will be possible for certain configurations of the FFT.
For instance, with ``TT_DATA = cint32``, it is possible for the sample values within the FFT to grow beyond the range of int32 values due to bit growth in the FFT algorithm. Saturation is applied at each stage (rank).
In the final stage when ``TP_SHIFT`` is applied, saturation is also applied. Similarly, if the FFT is configured for ``TT_DATA = cint16``, but insufficient scaling (``TP_SHIFT``) is applied, then sample values might exceed the range of int16 and so these too will be saturated in the final stage.
For ``TT_DATA = cfloat``, the FFT performs no scaling or saturation. Any saturation effects will be due to the atomic float operations returning positive infinity, negative infinity, or NaN.

Cascade Feature
---------------

The mixed radix FFT cascade feature is configured using the ``TP_CASC_LEN`` template parameter. This determines the number of kernels over which the FFT function is split. To be clear, this feature does not use the cascade ports of kernels to convey any data. IO-buffers are used to convey data from one kernel to the next in the chain. The term cascade is used simply in the sense that the function is split into a series of operations which are executed by a series of kernels, each on a separate tile. The FFT function is only split at stage boundaries, so the ``TP_CASC_LEN`` value cannot exceed the number of stages for that FFT.

Use of the cascade feature to split the FFT operation over multiple tiles will improve the throughput, because each tile in question will have fewer ranks of processing to execute and so it is ready for a new frame to process earlier.

The cascade feature is not supported for dynamic point size operation.

API Type
--------

The mixed radix FFT can be configured using ``TP_API`` to use IO buffer ports (0) or streams (1).

When configured for streams, additional kernels are added on the input and output to convert from streams to IO-buffers and vice versa, because internally the kernel performing the FFT itself uses IO buffers.

When configured to use streams, two streams are used. Even samples are to be supplied on the first stream input, and odd samples are to be supplied on the second input. In a similar fashion, even samples out appear on the first port out and odd samples out on the second port out.

Streaming configuration is not supported for dynamic point size operation.

Constraints
-----------

The Mixed Radix FFT does not contain any tool constraints such as location constraints.

Applying Design Constraints
---------------------------

Location and other constraints might be applied in the parent graph which instances the FFT graph class. To apply a constraint, you will need to know the name of the kernel, which will include the hierarchial path to that kernel. The simplest way to derive names, including the hierarchial part, is to compile a design and open it in AMD Vitis |trade| , using the graph view. The names of all kernels and memory buffers can be obtained from there. These names may then be back-annotated to the parent graph to apply the necessary constraint.

Code Example
============

.. literalinclude:: ../../../../L2/examples/docs_examples/test_mixed_radix_fft.hpp
    :language: cpp
    :lines: 17-

Configuration Notes
===================

This section is intended to provide guidance on how best to configure the FFT in some typical scenarios, or when designing with one particular metric in mind, such as resource use or performance.

Configuration for Performance Versus Resource
---------------------------------------------

At present, only one configuration parameter ``TP_CASC_LEN`` will affect this design tradeoff. ``TP_CASC_LEN`` can be set from 1 up to the number of stages required. The number of stages required will be determined by the ``TP_POINT_SIZE`` parameter. The number of stages is A+B+C+D where A is the power of 3, B is the power of 5, C is the power of 4, and D is the power of 2 required to decompose the point size. e.g., a point size of 432 = 3*3*3*2*2*2*2. Radix4 stages are used in preference to radix2 stages, so this will result in three stages of radix3 and two of radix4. The total number of stages in this example is therefore 5. When the point size is a multiple of 8, but not 16, radix2 stages are used, so a point size of 216 would decompose to 3*3*3*2*2*2 resulting in six stages total.

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
