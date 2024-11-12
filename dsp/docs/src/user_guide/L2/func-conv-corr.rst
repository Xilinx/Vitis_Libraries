..
   Copyright © 2019–2024 Advanced Micro Devices, Inc
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _CONVOLUTION_CORRELATION:

=======================
Convolution/Correlation
=======================

This library element computes the Convolution or Correlation of two input vectors depending on the specified function type. There are 3 compute modes which affect the size of the output - full, valid and same - which respectively compute the full convolution/correlation of F and G, convolution/correlation that do not rely on zero padding, and the convolution/correlation which has the same dimension size as F, centered with respect to the 'full' output.
The library element has configurable data types and vector dimensions for inputs F and G and the output, along with bit shifting, rounding and saturation.
Template parameters are used to configure the top level graph of the conv_corr_graph class.

Entry Point
===========

The graph entry point is the following:

.. code-block::

    xf::dsp::aie::conv_corr::conv_corr_graph

Device Support
==============

The Convolution/Correlation library element supports AIE and AIE-ML devices for all features with the following differences.

- Round modes available and the enumerated values of round modes differ between AIE and AIE-ML devices. See :ref:`COMPILING_AND_SIMULATING`.

Supported Types
===============

The data type for input port F and G (inF and inG) is controlled by T_DATA_F and T_DATA_G respectively.
Both inputs may take one of the 7 choices: int8, int16, int32, cint16, cint32, float and bfloat16.
The output may take one of 5 choices: int16, int32, cint16, cint32, float.
Please see table :ref:`CONV_CORR_combos` for valid input/output data type combinations.

.. _CONV_CORR_combos:

.. table:: IO-BUFFER INTERFACE : Supported Combinations of Input/Output data types
   :align: center

   +------------------+------------------+------------------+------------------+------------------+
   | InputF Data Type | InputG Data Type | Output Data Type | AIE Valid        | AIE-ML Valid     |
   +==================+==================+==================+==================+==================+
   | int8             | int8             | int16            | yes              | yes              |
   +------------------+------------------+------------------+------------------+------------------+
   | int16            | int8             | int16            | yes              | yes              |
   +------------------+------------------+------------------+------------------+------------------+
   | int16            | int16            | int32            | yes              | yes              |
   +------------------+------------------+------------------+------------------+------------------+
   | int32            | int16            | int32            | yes              | yes              |
   +------------------+------------------+------------------+------------------+------------------+
   | cint16           | int16            | cint16           | yes              | yes              |
   +------------------+------------------+------------------+------------------+------------------+
   | cint16           | int16            | cint32           | yes              | yes              |
   +------------------+------------------+------------------+------------------+------------------+
   | cint16           | int32            | cint32           | yes              | yes              |
   +------------------+------------------+------------------+------------------+------------------+
   | cint16           | cint16           | cint32           | yes              | yes              |
   +------------------+------------------+------------------+------------------+------------------+
   | cint32           | int16            | cint32           | yes              | yes              |
   +------------------+------------------+------------------+------------------+------------------+
   | cint32           | cint16           | cint32           | yes              | yes              |
   +------------------+------------------+------------------+------------------+------------------+
   | float            | float            | float            | yes              | yes              |
   +------------------+------------------+------------------+------------------+------------------+
   | bfloat16         | bfloat16         | float            | no               | yes              |
   +------------------+------------------+------------------+------------------+------------------+

.. table:: STREAM INTERFACE : Supported Combinations of Input/Output data types
   :align: center

   +------------------+------------------+------------------+------------------+------------------+
   | InputF Data Type | InputG Data Type | Output Data Type | AIE Valid        | AIE-ML Valid     |
   +==================+==================+==================+==================+==================+
   | cint16           | cint16           | cint16           | yes              | no               |
   +------------------+------------------+------------------+------------------+------------------+


Template Parameters
===================

To see details on the template parameters for Convolution / Correlation, see :ref:`API_REFERENCE`.


Access Functions
================

To see details on the access functions for Convolution / Correlation, see :ref:`API_REFERENCE`.

Ports
=====

To see details on the ports for Convolution / Correlation, see :ref:`API_REFERENCE`.

Design Notes
============
The performance of the IP depends on the chosen data types combo :ref:`CONV_CORR_combos` . The number of multiplications per clock cycle will updated based on data type combo.
The Convolution/Correlation operation can be processed by both IO Buffer (TP_API = 0) and Stream Based (TP_API =1) which can controlled by the parameter named as ``TP_API``

IO Buffer Interface
-------------------
The Convolution/Correlation operation via IO Buffer by both AIE and AIE-ML.

IO Buffer interface can support all data type combos, see :ref:`CONV_CORR_combos` `table:: IO-BUFFER INTERFACE`

Streaming Interface
-------------------
The Convolution/Correlation operation via streaming supported by AIE Engine only and there is no support in AIE-ML
Input F Sig. is assumed to be streaming on AXI-Stream interface. As F is consumed on streams, selection of Length(F) can be higher to meet higher throughput.
Input G Sig. is assumed to be window on I/O buffer interface and length(G) can be smaller.

Streaming interface can support only one data type combo, see :ref:`CONV_CORR_combos` `table:: STREAM INTERFACE`


Scaling
-------
Scaling in Convolution / Correlation is controlled by the ``TP_SHIFT`` parameter which describes the number of bits to shift the output to the right.
Float, cfloat and bfloat16 implementations do not support scaling. ``TP_SHIFT`` must be set to '0'.

Saturation
----------
Distortion caused by saturation will be possible for Convolution / Correlation. Since the input values are input at construction time, no compile-time error can be issued for this hazard, so it is for the user to ensure that saturation does not occur.


Code Example
============


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

