..
   Copyright © 2019–2024 Advanced Micro Devices, Inc
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _FFT_Window:

==========
FFT Window
==========

This library element implements an FFT window function such as a Hamming Window. It has configurable point size, data type, scaling (as a shift), static/dynamic point size, window size, interface API (stream/window), and parallelism factor.

Entry Point
===========

The graph entry point is the following:

.. code-block::

    xf::dsp::aie::fft::fft_window_graph

Device Support
==============

The fft_window supports AIE and AIE-ML devices with the following exception(s):

- ``TT_DATA`` supports cfloat on AIE, but not on AIE-ML.
- ``TT_COEFF`` supports float on AIE, but not on AIE-ML.
- Round modes available and the enumerated values of round modes differ between AIE and AIE-ML devices. See :ref:`COMPILING_AND_SIMULATING`.

Supported Types
===============

The data type to the FFT window is controlled by the ``TT_DATA`` template parameter . This can take one of three choices: cint16, cint32, or cfloat (AI Engine (AIE) only, not AIE-ML). This selection applies to both input data and output data.

The ``TT_COEFF`` template parameter  can take one of three values, int16, int32, or cfloat, but currently, this value is forced by the choice of ``TT_DATA``, so ``TT_COEFF`` must be set to:

- int16 when ``TT_DATA`` is set to cint16 or
- int32 when ``TT_DATA`` is set to cint32 or
- float for ``TT_DATA`` set to cfloat.

Template Parameters
===================

To see details on the template parameters for the FFT Window, see :ref:`API_REFERENCE`.

Access Functions
================

To see details on the access functions for the FFT Window, see :ref:`API_REFERENCE`.

Ports
=====

To see details on the ports for the FFT Window, see :ref:`API_REFERENCE`. Note that the number and type of ports are determined by the configuration of template parameters.

Design Notes
============

The FFT Window performs a sample by sample scaling of data prior to entry to a FFT. Because there is an open-ended list of window types in the literature, this unit takes the raw coefficient values as a constructor argument rather than a choice of window types from a finite supported library. For convenience, utility functions are provided for some of the common window types, i.e., Hamming, Hann, Blackman, and Kaiser. On construction, the array containing the window coefficients must be passed to the graph constructor.

Dynamic Point Size
------------------

The FFT Window supports dynamic (runtime controlled) point sizes. This feature is available when the ``TP_DYN_PT_SIZE`` template parameter is set to 1.

When set to 0 (static point size), all data will be expected in frames of ``TP_POINT_SIZE`` data samples, though multiple frames might be input together using ``TP_WINDOW_VSIZE`` which is an integer multiple of ``TP_POINT_SIZE``.

When set to 1 (dynamic point size), each _window_ must be preceded by a 256 bit header to describe the runtime parameters of that window. ``TP_WINDOW_VSIZE`` describes the number of samples in a window so it does not include this header.

The format of the header is described in the following table. When ``TP_DYN_PT_SIZE = 1``, ``TP_POINT_SIZE`` describes the maximum point size which can be input.

.. _FFT_Window_HEADER_FORMAT:

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
   | Reserved                      | 2  OR                | reserved                                                                        |
   |                               | 6 for TT_DATA=cint16 |                                                                                 |
   +-------------------------------+----------------------+---------------------------------------------------------------------------------+
   |                               |                      |                                                                                 |
   | Status (output only)          | 3 (real part)  OR    | 0 = legal point size, 1 = illegal point size                                    |
   |                               | 7 for TT_DATA=cint16 |                                                                                 |
   +-------------------------------+----------------------+---------------------------------------------------------------------------------+

The locations are set to suit the ``TT_DATA`` type. That is, for ``TT_DATA=cint16``, direction is described in the first cint16 (real part) of the 256 bit header and point size is described in the real part of the second cint16 value. Similarly, for ``TT_DATA=cint32``, the real part of the first cint32 value in the header holds the direction field and the real part of second cint32 value holds the Point size (radix2) field.

For ``TT_DATA=cfloat``, the values in the header are expected as cfloat and are value-cast (not reinterpret-cast) to integers internally. The output window also has a header. This is copied from the input header except for the status field, which is inserted. The status field is ignored on input. If an illegal point size is entered, the output header will have this field set to a non-zero value and the remainder of the output window is undefined.

For dynamic operation (``TP_DYN_PT_SIZE = 1``), the window will operate on frames of ``TP_POINT_SIZE`` or ``TP_POINT_SIZE/2^N`` down to 16. Because the window values cannot be faithfully determined by sampling or interpolating from the provided parent array, it is necessary when in this mode to provide an array holding the window values for each point size. Because this full array is ``TP_POINT_SIZE*(1+ 1/2 + 1/4 + ...)``, the overall table size provided must be ``TP_POINT_SIZE * 2``. For example, for ``TP_POINT_SIZE = 64``, the values of the window coefficients for point size 64 will occupy array indices 0 to 63. The coefficients for point size 32 will occupy indices 64 thru 95, and those for point size 16 will occupy 96 thru 111.

Super Sample Rate Operation
---------------------------

While the term Super Sample Rate strictly means the processing of more than one sample per clock cycle, in the AIE context, it is taken to mean an implementation using parallel kernels to improve performance at the expense of additional resource use. In the FFT window, the SSR operation is controlled by the ``TP_SSR`` template parameter. This parameter is intended to improve performance and also allow support of point sizes beyond the limitations of a single tile.

The ``TP_SSR`` parameter allows a trade of performance for resource use in the form of tiles used. The tile utilization is quite simply the same as the value of ``TP_SSR``.

Super Sample Rate Sample to Port Mapping
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When a Super Sample Rate operation is used, data is input and output using multiple ports. These multiple ports on input or output act as one channel. The mapping of samples to ports is that each successive sample should be passed to a different port in a round-robin fashion, e.g., with ``TP_SSR`` set to 4, samples 0, 4, 8, ... should be sent to input port 0, samples 1, 5, 9, ... to input port 1, samples 2, 6, 10, ... to input port 2, sample 3, 7, 11, ... to input port 3, and so on.

Scaling
-------

Scaling in the FFT window is controlled by the ``TP_SHIFT`` parameter which describes how many binary places by which to shift the result to the right, i.e., only power-of-2 scaling values are supported. For integer types, it is suggested that the window coefficient values approach the maximum positive value which can be expressed the type selected, to maximize the number of significant digits. e.g., the provided utility functions scale int16 windows to a maximum value of 2^14, and therefore a ``TP_SHIFT`` value of 14 will normalize output values and ensure saturation does not occur.

No scaling is applied at any point when the data type is cfloat. Setting ``TP_SHIFT`` to any value other than 0 when ``TT_DATA`` is cfloat will result in an error.

Saturation
----------

Distortion caused by saturation will be possible for certain configurations of the FFT window. For instance, with integer data types if any window coefficient values exceed ``2^TP_SHIFT``, saturation will occur. Because the window coefficient values are input at construction time, no compile-time error can be issued for this hazard, so it is for you to ensure that saturation does not occur.

Constraints
-----------

The FFT window does not contain any constraints. It is a single kernel design except when ``TP_SSR > 1`` in which case the port connections force placement of the tiles on separate tiles.

Code Example
============
.. literalinclude:: ../../../../L2/examples/docs_examples/test_fft_window.hpp
    :language: cpp
    :lines: 17-

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
