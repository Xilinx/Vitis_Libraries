..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _DSP_SAMPLE_DELAY:

======================
Sample Delay
======================

Sample Delay is a circular-buffer-based implementation of a delay filter for introducing a delay into a time series. See the Design Notes for insight into the implementation.

Entry Point
===========

The graph entry point is as follows:

.. code-block::

    xf::dsp::aie::sample_delay

Device Support
==============

The Sample Delay supports AIE, AIE-ML, and AIE-MLv2.

Supported Data Types
====================

The sample_delay supports ``uint8``, ``int8``, ``int16``, ``cint16``, ``int32``, ``cint32``, ``float``, and ``cfloat`` data types on the input.

Template Parameters
===================

To see details on the template parameters for the sample_delay, see :ref:`DSP_API_REFERENCE`.

Access Functions
================

To see details on the access functions for the sample_delay, see :ref:`DSP_API_REFERENCE`.

Ports
=====

To see details on the ports for the sample_delay, see :ref:`DSP_API_REFERENCE`.

Design Notes
============

Sample Delay introduces delay into input data (often a time series). The unit of delay is the number of samples, provided via the runtime-programmable (RTP) port ``sampleDelayValue``. The legal range of ``sampleDelayValue`` is [0, ``MAX_DELAY`` - 1].
As far as functionality is concerned, it is a delay filter; however, the implementation employs a vectorized circular buffer. The delay provided on the ``sampleDelayValue`` RTP port is introduced by converting it into two address offsets: a vector offset and an element offset.
The vector-offset portion of the delay adjusts the starting read address, whereas the remaining element-offset portion is applied via a shuffle operation carried out on each vector traversing the processor registers.



.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
