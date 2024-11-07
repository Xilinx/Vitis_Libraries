..
   Copyright © 2019–2024 Advanced Micro Devices, Inc
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _KRONECKER_MATRIX_PRODUCT:

========================
Kronecker Matrix Product
========================


Entry Point
===========

The graph entry point is the following:

.. code-block::

    xf::dsp::aie::kronecker::kronecker_graph

Device Support
==============

The Kronecker supports AIE and AIE-ML devices.

Supported Types
===============

Please see table :ref:`KRONECKER_output_type`: for allowed input data type combinations and corresponding output type.

.. _KRONECKER_output_type:

.. table:: Supported Combinations of Input/Output data types
   :align: center

   +------------------+------------------+------------------+-------------+
   | InputA Data Type | InputB Data Type | Output Data Type | Vector Size |
   +==================+==================+==================+=============+
   | int16            | int16            | int16            | 16          |
   +------------------+------------------+------------------+-------------+
   | int16            | int32            | int32            | 8           |
   +------------------+------------------+------------------+-------------+
   | int16            | cint16           | cint16           | 8           |
   +------------------+------------------+------------------+-------------+
   | int16            | cint32           | cint32           | 4           |
   +------------------+------------------+------------------+-------------+
   | int32            | int16            | int32            | 8           |
   +------------------+------------------+------------------+-------------+
   | int32            | int32            | int32            | 8           |
   +------------------+------------------+------------------+-------------+
   | int32            | cint16           | cint32           | 4           |
   +------------------+------------------+------------------+-------------+
   | int32            | cint32           | cint32           | 4           |
   +------------------+------------------+------------------+-------------+
   | cint16           | int16            | cint16           | 8           |
   +------------------+------------------+------------------+-------------+
   | cint16           | int32            | cint32           | 4           |
   +------------------+------------------+------------------+-------------+
   | cint16           | cint16           | cint16           | 8           |
   +------------------+------------------+------------------+-------------+
   | cint16           | cint32           | cint32           | 4           |
   +------------------+------------------+------------------+-------------+
   | cint32           | int16            | cint32           | 4           |
   +------------------+------------------+------------------+-------------+
   | cint32           | int32            | cint32           | 4           |
   +------------------+------------------+------------------+-------------+
   | cint32           | cint16           | cint32           | 4           |
   +------------------+------------------+------------------+-------------+
   | cint32           | cint32           | cint32           | 4           |
   +------------------+------------------+------------------+-------------+
   | float            | float            | float            | 8           |
   +------------------+------------------+------------------+-------------+
   | float            | cfloat           | cfloat           | 4           |
   +------------------+------------------+------------------+-------------+
   | cfloat           | float            | cfloat           | 4           |
   +------------------+------------------+------------------+-------------+
   | cfloat           | cfloat           | cfloat           | 4           |
   +------------------+------------------+------------------+-------------+

Template Parameters
===================

To see details on the template parameters for the Kronecker Matrix Product, see :ref:`API_REFERENCE`.

Ports
=====

To see details on the ports for the Kronecker Matrix Product, see :ref:`API_REFERENCE`. Note that the type of ports are determined by the configuration of template parameters.

Design Notes
============
The performance of the IP heavily depends on the chosen data types. The data type combination determines the number of multiplications per clock cycle.

Inputs
------

It accepts input matrices in COLUMN major order. The leading dimension of the input matrices have to be an integer multiple of vector size, where:

   #. The leading dimension in a COLUMN major order is ROWS.
   #. The vector size is data type dependent. Please see table :ref:`KRONECKER_output_type` for the vector size.

Super Sample Rate (SSR)
-----------------------

The SSR operation is controlled by parameter ``TP_SSR`` and SSR enables running multiple instances of a kernel in parallel where each instance runs on a separate tile. The input data is split and distributed to the parallel kernel instances.

   #. Input matrix A is split and distributed to parallel kernels. The split is based on the COLUMNS and thus ``TP_DIM_A_COLS`` must be divisible by ``TP_SSR``.
   #. Input matrix B is not split and a copy of it is passed to each parallel kernel.

Scaling
-------

Scaling is controlled by the ``TP_SHIFT`` parameter which describes the number of bits to shift the output to the right. Only power-of-2 scaling is supported. Float and cfloat implementations do not support scaling.

Constraints
-----------
The Kronecker Matrix Product does not contain any constraints. It is a single kernel design except when ``TP_SSR > 1`` in which case the port connections force placement of the kernels on separate tiles.

Code Example
============
.. literalinclude:: ../../../../L2/examples/docs_examples/test_kronecker.hpp
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



