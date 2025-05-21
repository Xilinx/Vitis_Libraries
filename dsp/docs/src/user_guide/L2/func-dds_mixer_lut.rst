..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _DDS_MIXER_LUT:

======================
DDS / Mixer Using LUTs
======================

Entry Point
============

The graph entry point is the following:

.. code-block::

    xf::dsp::aie::mixer::dds_mixer::dds_mixer_lut_graph

Device Support
==============

The DDS/Mixer LUT library element supports AIE, AIE-ML and AIE-MLv2 with the following differences:

- Round modes available and the enumerated values of round modes are the same for AIE-ML and AIE-MLv2 devices, but differ from those for AIE devices. See :ref:`COMPILING_AND_SIMULATING`.

Supported Types
===============

On AI Engine (AIE) devices, the dds_mixer_lut supports cint16, cint32, and cfloat as ``TT_DATA`` type that specifies the type of input and output data. Input is only required when ``TP_MIXER_MODE`` is
set to 1 (simple mixer) or 2 (dual conjugate mixer).

The ``cfloat`` data type is not supported on AIE-ML device.

Template Parameters
===================

To see details on the template parameters for the DDS/Mixer, see :ref:`API_REFERENCE`.

Access Functions
================

To see details on the access functions for the DDS/Mixer, see :ref:`API_REFERENCE`.

Ports
=====

To see details on the ports for the DDS/Mixer, see :ref:`API_REFERENCE`. On AIE-ML devices, outputs of the streaming and windowed modes are not bitwise identical due to some implementation choices made for higher performance.

Design Notes
============

Scaling
-------

When configured as a DDS (``TP_MIXER_MODE=0``), the output of the DDS is intended to be the components of a unit vector. For ``TT_DATA = cfloat``, this means that the outputs will be in the range -1.0 to +1.0. For ``TT_DATA = cint16`` and ``TT_DATA = cint32`` the output is scaled by two to the power 15 and 31 respectively such that the binary point follows the most significant bit of the output. Therefore, if the DDS output is used to multiply/mix, you must account for this bit shift.

SFDR
----

Spurious Free Dynamic Range is a parameter used to characterize signal generators. It measures the ratio between the amplitude of the fundamental frequency of the signal generator to the amplitude of the strongest spur. The dds_mixer_lut generates slightly different designs based on the SFDR you request. The different implementations offer tradeoffs between the SFDR of the waveform generated and the throughput. Three different implementations are available, for SFDR less than 60 dB, less than 120 dB, and for less than 180 dB. With the cint16 data type, the maximum SFDR is restricted to 96 dB by the bit-width of the data type. So, only two different implementations are available.

Super Sample Rate Operation
---------------------------

See :ref:`DDS_SSR`.

In this implementation of the DDS, the output data for various values of ``TP_SSR`` are not bitwise identical. The SFDR values might also be slightly different for different values of ``TP_SSR``.

Phase offset and Phase increment
--------------------------------

In the basic configuration, phase offset and phase increment are input to the DDS graph as constructor arguments. As such they are set at power-on and remain constant for all time.
The template parameter ``TP_USE_PHASE_OFFSET`` allows for phase offset to be modified at runtime. Set this parameter to 1 to allow run-time changes to phase offset. The template parameter ``TP_PHASE_OFFSET_API`` selects the form of port for this update. When set to 0 the graph will expose an RTP port. This is asynchronous, which means that it is not necessary to supply a new value for phase offset on each iteration of the kernels(s). When set to 1 the graph will expose an iobuffer port. Iobuffers have a minimum size of 32 bytes, but the phase offset is described in the first 4 bytes as a uint32 just as for an RTP. Since the iobuffer port is blocking, a new value for phase offset must be supplied for each iteration of the kernel(s).
Phase increment can be configured for run-time update. To do this, set ``TP_USE_PHASE_INC_RELOAD`` to 1. This will result in the exposure of an RTP port for phase increment in the form of a uint32. This port is asynchronous, so a new value of phase increment need not be supplied for every iteration of the kernel(s).
Note that the value supplied is used to calculate lookup tables necessary for the parallel operation. The function to do this takes approx 128 cycles on AIE, which has built-in sincos lookup, versus approx 220 cycle on AIE-ML. Therefore, with each RTP requiring so many cycles, frequent RTP updates will have a marked effect on performance.

Implementation Notes
====================

In a conventional DDS (sometimes known as an Numerically Controlled Oscillator), a phase increment value is added to a phase accumulator on each cycle. The value of the phase accumulator is effectively the phase part of a unit vector in polar form. This unit vector is then converted to cartesian form by the lookup of sin and cos values from a table of precomputed values. These cartesian values are then output.

It should be noted that, in the dds_mixer_lut the sin/cos values are not scaled to the full range of the bit-width to avoid saturation effects that arise due to 2s complement representation of numbers. The maximum positive value representable by an n-bit 2s complement number is 1 less than the magnitude of the largest negative value. So, the sin/cos values are scaled by the magnitude of the maximum positive value only. So, for cint16 type, +1 scales to +32767 and -1 scales to -32767. Also, following the runtime multiplication of the looked-up cartesian value for a cycle by the precomputed vector, scaling down and rounding will lead to other small reductions in the maximum magnitude of the waveform produced.

Code Example
============

The following code example shows how the DDS/Mixer graph class can be used within a user super-graph to use an instance configured as a mixer.

.. literalinclude:: ../../../../L2/examples/docs_examples/test_dds_lut.hpp
    :language: cpp
    :lines: 17-

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
