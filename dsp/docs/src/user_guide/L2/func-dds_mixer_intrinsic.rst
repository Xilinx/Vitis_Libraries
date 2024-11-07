..
   Copyright © 2019–2024 Advanced Micro Devices, Inc
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _DDS_MIXER_INTRINSIC:

==========
DDS/Mixer
==========

Entry Point
============

The graph entry point is the following:

.. code-block::

    xf::dsp::aie::mixer::dds_mixer::dds_mixer_graph

Device Support
==============

The DDS/Mixer is supported on AIE only. For other devices, please use the DDS/Mixer LUT library element.

Supported Types
===============

The DDS/Mixer supports input types of cint16, cint32, and cfloat as selected by ``TT_DATA``. Input is only required when ``TP_MIXER_MODE`` is set to 1 (simple mixer) or 2 (dual conjugate mixer). The output type is also set by ``TT_DATA``. When ``TP_MIXER_MODE`` is set to 0 (DDS mode), ``TT_DATA`` types of cint16 or cfloat only are supported.

Template Parameters
===================

To see details on the template parameters for the DDS/Mixer, see :ref:`API_REFERENCE`.

Access Functions
================

To see details on the access functions for the DDS/Mixer, see :ref:`API_REFERENCE`.

Ports
=====

To see details on the ports for the DDS/Mixer, see :ref:`API_REFERENCE`.

Design Notes
============

Scaling
-------

When configured as a DDS (``TP_MIXER_MODE=0``), the output of the DDS is intended to be the components of a unit vector. For ``TT_DATA = cfloat``, this means that the outputs will be in the range -1.0 to +1.0. For ``TT_DATA = cint16``, the output is scaled by 2 to the power 15 such that the binary point follows the most significant bit of the output. Therefore, if the DDS output is used to multiply/mix, you must account for this 15 bit shift.

.. _DDS_SSR:

Super Sample Rate Operation
---------------------------

While the term Super Sample Rate strictly means the processing of more than one sample per clock cycle, in the AI Engine (AIE) context it is taken to mean an implementation using parallel kernels to improve performance at the expense of additional resource use. In the DDS/Mixer, SSR operation is controlled by the ``TP_SSR`` template parameter. Quite simply, the number of kernels used is set by ``TP_SSR`` and the performance increases proportionately.

Super Sample Rate Sample to Port Mapping
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When Super Sample Rate operation is used, data is input (where applicable) and output using multiple ports. These multiple ports on input or output act as one channel. The mapping of samples to ports is that each successive sample should be passed to a different port in a round-robin fashion, e.g., with ``TP_SSR`` set to 3, sample 0 should be sent to input port 0, sample 1 to input port 1, sample 2 to input port 2, sample 3 to input port 0, and so on.

Implementation Notes
====================

In a conventional DDS (sometimes known as an Numerically Controlled Oscillator), a phase increment value is added to a phase accumulator on each cycle. The value of the phase accumulator is effectively the phase part of a unit vector in polar form. This unit vector is then converted to cartesian form by the lookup of sin and cos values from a table of precomputed values. These cartesian values are then output.

The AIE is a vector processor where multiple data samples are operated upon each cycle. This is often referred to as Super Sample Rate, where the data rate exceeds the clock rate. The AIE DSP Library DDS is Super Sample Rate for maximal performance. The operation to convert phase to sin/cos values is a scalar operation, not a vector operation. The performance per kernel is limited by the number of data samples which can be written out per cycle. This number of samples differs according to data type size. Call this value N. Therefore, the implementation is for the phase accumulator to have N*phase_increment per cycle. This phase value is then converted to sin/cos values as described earlier. To convert this into N output samples, this scalar cartesian value is then multiplied by a vector of precomputed cartesian offset values to give the output values for samples ``0, 1, 2, ..., N-1``.

The precomputation occurs at construction time. The vector of offset values is created by a series of polar to cartesian lookups using ``0, phase_increment*1, phase_increment*2, ..., phase_increment*(N-1)``.

It should be noted that the cartesian values for lookup in hardware are scaled to use the full range of int16, so -1 becomes -32768, but +1 is saturated to +32767. Also, following the runtime multiplication of the looked-up cartesian value for a cycle by the precomputed vector, scaling down and rounding will lead to slightly different values than if the lookup had been used directly for each output value. In other words, the DDS output is not bit-accurate to the sin/cos lookup intrinsic.

:ref:`FIGURE_DDS_IMPL` shows the construction-time creation of a vector of offsets, then the runtime use of this vector to create multiple outputs from a single sin/cos lookup each cycle.

.. _FIGURE_DDS_IMPL:
.. figure:: ./media/DDS\ implementation.png

    **DDS Implementation**

Code Example
============

The following code example shows how the DDS/Mixer graph class can be used within a user super-graph to use an instance configured as a mixer.

.. literalinclude:: ../../../../L2/examples/docs_examples/test_dds.hpp
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
