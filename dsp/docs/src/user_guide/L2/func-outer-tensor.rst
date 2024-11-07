..
   Copyright © 2019–2024 Advanced Micro Devices, Inc
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _OUTER_TENSOR_PRODUCT:

====================
Outer Tensor Product
====================

This library element computes the Outer Tensor Product of two input vectors. If two vectors have dimensions n and m, then their outer tensor product is an n x m matrix.
Element-wise multiplication of each element of input A by each element of input B is performed and assigned to the output.  If it is considered that input A is the column vector and input B is the row vector, then the matrix is output in a row-major fashion.
The outer tensor product has configurable data types and vector dimensions for inputs A and B, along with a configurable number of frames, scaling, interfaces (stream/window), parallelism factors, rounding and saturation.
Template parameters are used to configure the top level graph of the outer_tensor_graph class.

Entry Point
===========

The graph entry point is the following:

.. code-block::

    xf::dsp::aie::outer_tensor::outer_tensor_graph

Device Support
==============

The Outer Tensor Product library element supports AIE and AIE-ML devices.

Supported Types
===============
The data type for input port A and B (inA and inB) is controlled by ``TT_DATA_A and TT_DATA_B`` respectively.
Both inputs may take one of the 6 choices: int16, int32, cint16, cint32, float and cfloat. It must be kept in mind that depending on the input type combinations, output type will be determined by the tool.
Please see table :ref:`OUTER_TENSOR_output_type` : for allowed input data type combinations and regarding output type.

.. _OUTER_TENSOR_output_type:

.. table:: Supported Combinations of Input/Output data types
   :align: center

   +------------------+------------------+------------------+
   | InputA Data Type | InputB Data Type | Output Data Type |
   +==================+==================+==================+
   | int16            | int16            | int16            |
   +------------------+------------------+------------------+
   | int16            | int32            | int32            |
   +------------------+------------------+------------------+
   | int16            | cint16           | cint16           |
   +------------------+------------------+------------------+
   | int16            | cint32           | cint32           |
   +------------------+------------------+------------------+
   | int32            | int16            | int32            |
   +------------------+------------------+------------------+
   | int32            | int32            | int32            |
   +------------------+------------------+------------------+
   | int32            | cint16           | cint32           |
   +------------------+------------------+------------------+
   | int32            | cint32           | cint32           |
   +------------------+------------------+------------------+
   | cint16           | int16            | cint16           |
   +------------------+------------------+------------------+
   | cint16           | int32            | cint32           |
   +------------------+------------------+------------------+
   | cint16           | cint16           | cint16           |
   +------------------+------------------+------------------+
   | cint16           | cint32           | cint32           |
   +------------------+------------------+------------------+
   | cint32           | int16            | cint32           |
   +------------------+------------------+------------------+
   | cint32           | int32            | cint32           |
   +------------------+------------------+------------------+
   | cint32           | cint16           | cint32           |
   +------------------+------------------+------------------+
   | cint32           | cint32           | cint32           |
   +------------------+------------------+------------------+
   | float            | float            | float            |
   +------------------+------------------+------------------+
   | float            | cfloat           | cfloat           |
   +------------------+------------------+------------------+
   | cfloat           | float            | cfloat           |
   +------------------+------------------+------------------+
   | cfloat           | cfloat           | cfloat           |
   +------------------+------------------+------------------+


Template Parameters
===================

To see details on the template parameters for the Outer Tensor Product, see :ref:`API_REFERENCE`.


Access Functions
================

To see details on the access functions for the Outer Tensor Product, see :ref:`API_REFERENCE`.

Ports
=====

To see details on the ports for the Outer Tensor Product, see :ref:`API_REFERENCE`. Note that the type of ports are determined by the configuration of template parameters.

Design Notes
============

Super Sample Rate Operation
---------------------------

While the term Super Sample Rate strictly means the processing of more than one sample per clock cycle, in the AIE context it is taken to mean an implementation using parallel kernels to improve performance at the expense of additional resource use.
In the Outer Tensor Product, SSR operation is controlled by the template parameter ``TP_SSR``. The number of tiles utilized to divide up workload is the value of ``TP_SSR``.

Scaling
-------
Scaling in the Outer Tensor Product is controlled by the ``TP_SHIFT`` parameter which describes the number of bits to shift the output to the right. Only power-of-2 scaling is supported.
Float and cfloat implementations do not support scaling.

Saturation
----------
Distortion caused by saturation will be possible for the Outer Tensor Product. It is for the user to ensure that saturation does not occur.

Constraints
-----------
The Outer Tensor Product inputs for ``TP_DIM_A``, ``TP_DIM_B``, ``TP_NUM_FRAMES`` and ``TP_SSR`` must be powers of 2. ``TP_DIM_X * size_of(TT_DATA_X)`` must have a minimum value of 32 bytes (size of buffer on AIE). It is a single kernel design except when ``TP_SSR>1`` in which case the port connections force placement of the tiles on separate tiles.

Code Example
============
.. literalinclude:: ../../../../L2/examples/docs_examples/test_outer_tensor.hpp
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



