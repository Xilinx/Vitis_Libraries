..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _WIDGET_REAL2COMPLEX:

======================
Widget Real to Complex
======================

The DSPLib contains a Widget Real to Complex solution, which provides a utility to convert real data to complex or vice versa.

Entry Point
===========

The graph entry point is the following:

.. code-block::

    xf::dsp::aie::widget::real2complex::widget_real2complex_graph

Device Support
==============

The widget real2complex supports AIE and AIE-ML devices.

Supported Types
===============

The widget_real2complex supports int16, cint16, int32, cint32, float, and cfloat on input. The corresponding ``TT_OUT_DATA`` must be set to the real or complex partner of the input type, e.g., if ``TT_DATA`` = int16, ``TT_OUT_DATA`` must be set to cint16.

Template Parameters
===================

To see details on the template parameters for the Widget Real to Complex, see :ref:`API_REFERENCE`.

Access Functions
================

To see details on the access functions for the Widget Real to Complex, see :ref:`API_REFERENCE`.

Ports
=====

To see details on the ports for the Widget Real to Complex, see :ref:`API_REFERENCE`.

Design Notes
============

The widget_real2complex library element converts real to complex or complex to real. An example of its use is that it can be used to enable real-only FFT operations despite the fact that the FFT currently supports only complex data types.

Code Example
============

The following code example shows how the widget_real2complex graph class can be used within a user super-graph, including how to set the runtime<ratio> of internal kernels. This example shows the widget configured to convert a window of int16 samples to cint16 samples.

.. literalinclude:: ../../../../L2/examples/docs_examples/test_widg_r2c.hpp
    :language: cpp
    :lines: 17-


.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
