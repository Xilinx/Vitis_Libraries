..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _SAMPLE_DELAY:

======================
Sample Delay
======================

Sample Delay is a circular buffer based implementation of a delay filter for introducing a delay into a time series. See the design notes for insight into the implementation.

Entry Point
===========

The graph entry point is the following:

.. code-block::

    xf::dsp::aie::sample_delay

Device Support
==============

The Sample Delay supports AIE, AIE-ML and AIE-MLv2.

Supported Types
===============

The sample_delay supports unit8, int8, int16, cint16, int32, cint32, float, and cfloat data types on the input.

Template Parameters
===================

To see details on the template parameters for the sample_delay, see :ref:`API_REFERENCE`.

Access Functions
================

To see details on the access functions for the sample_delay, see :ref:`API_REFERENCE`.

Ports
=====

To see details on the ports for the sample_delay, see :ref:`API_REFERENCE`.

Design Notes
============

Sample Delay introduces delay into the input data which is often a time series. The unit of delay is 'number of samples' which is passed on Run Time Parameter (RTP) port: sampleDelayValue. The legal range of sampleDelayValue is [0, MAX_DELAY-1].
As far as the functionality is concerned, it is a delay filter, however, implementation employs a vectorized circular buffer. The delay passed on the sampleDelayValue RTP port is introduced by converting it into two address offsets: vector offset and element offset.
The application of vector offset part (of the delay) is used to adjust the starting read address, whereas the application of the remainder element offset part (of the delay) is a shuffle operation carried out on each vector traversing through the processor registers.



.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
