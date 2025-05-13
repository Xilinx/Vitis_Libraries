..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _WIDGET_API_CAST:

===============
Widget API Cast
===============

The DSPLib contains a Widget API Cast solution, which provides flexibility when connecting other kernels. This component is able to change from the stream interface to window interface and vice-versa. It may be configured to read two input stream interfaces and interleave data onto an output window interface. In addition, multiple copies of output window may be configured to allow extra flexibility when connecting to further kernels.

Entry Point
===========

The graph entry point is the following:

.. code-block::

    xf::dsp::aie::widget::api_cast::widget_api_cast_graph

Device Support
==============

The widget API cast supports AIE and AIE-ML devices.

Supported Types
===============

The widget API cast supports int16, cint16, int32, cint32, float, and cfloat types as selected by the ``TT_DATA`` template parameter. This data type is used by both input and output ports.

Template Parameters
===================

To see details on the template parameters for the Widget API Cast, see :ref:`API_REFERENCE`.

Access Functions
================

To see details on the access functions for the Widget API Cast, see :ref:`API_REFERENCE`.

Ports
=====

To see details on the ports for the Widget API Cast, see :ref:`API_REFERENCE`.

Design Notes
============

The widget_api_cast library element serves multiple purposes. First, it can convert from window to stream or vice versa. Second, it can perform a limited broadcast of windows. Third, it can perform various patterns of interlace when there are two streams in or out.

Code Example
============

The following code example shows how the widget_api_cast graph class can be used within a user super-graph, including how to set the runtime<ratio> of internal kernels. This example shows the widget configured to interlace two input streams on a sample-by-sample basis, with the output written to a window.

.. literalinclude:: ../../../../L2/examples/docs_examples/test_widget_api_cast.hpp
    :language: cpp
    :lines: 17-




.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
