..
   Copyright © 2019–2024 Advanced Micro Devices, Inc
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _HADAMARD_PRODUCT:

================
Hadamard Product
================

This library element implements Hadamard Product of two input vectors. Element-wise multiplication of input ports (inA and inB) is performed. Therefore input and output vectors are required to be the same dimension.
The Hadamard Product IP has configurable vector dimension, data type, window size, scaling (as a shift), interface api (stream/window) and parallelism factor.
Template parameters are used to configure the top level graph of the hadamard_graph class.

Entry Point
===========

The graph entry point is the following:

.. code-block::

    xf::dsp::aie::hadamard::hadamard_graph

Device Support
==============

The Hadamard supports AIE and AIE-ML devices.

- Round modes available and the enumerated values of round modes differ between AIE and AIE-ML devices. See :ref:`COMPILING_AND_SIMULATING`.


Supported Types
===============
The data types of input ports A and B (inA and inB) are controlled by ``TT_DATA_A`` and ``TT_DATA_B`` respectively.
Both inputs may take one of the 6 choices: int16, int32, cint16, cint32, float and cfloat. It must be kept in mind that depending on the input type combinations, output type will be determined by the IP.
Please see table :ref:`HADAMARD_output_type`: for allowed input data type combinations and resultant output type. In addition, the vector size granularity is given for the data type combinations. ``TP_DIM`` should be a multiple of the listed vector size.

.. _HADAMARD_output_type:

.. table:: Hadamard Supported Combinations of Input/Output data types
   :align: center

   +-------------------+-------------------+------------------+----------------------------+
   | Input Data Type A | Input Data Type B | Output Data Type | Vector Size (window/stream)|
   +===================+===================+==================+============================+
   | int16             | int16             | int16            |         16 / 8             |
   +-------------------+-------------------+------------------+----------------------------+
   | int16             | int32             | int32            |          8 / 8             |
   +-------------------+-------------------+------------------+----------------------------+
   | int16             | cint16            | cint16           |          8 / 8             |
   +-------------------+-------------------+------------------+----------------------------+
   | int16             | cint32            | cint32           |          8 / 8             |
   +-------------------+-------------------+------------------+----------------------------+
   | int32             | int16             | int32            |          8 / 8             |
   +-------------------+-------------------+------------------+----------------------------+
   | int32             | int32             | int32            |          8 / 4             |
   +-------------------+-------------------+------------------+----------------------------+
   | int32             | cint16            | cint32 *         |          4 / 4             |
   +-------------------+-------------------+------------------+----------------------------+
   | int32             | cint32            | cint32           |          4 / 4             |
   +-------------------+-------------------+------------------+----------------------------+
   | cint16            | int16             | cint16           |          8 / 8             |
   +-------------------+-------------------+------------------+----------------------------+
   | cint16            | int32             | cint32 *         |          4 / 4             |
   +-------------------+-------------------+------------------+----------------------------+
   | cint16            | cint16            | cint16           |          8 / 4             |
   +-------------------+-------------------+------------------+----------------------------+
   | cint16            | cint32            | cint32           |          4 / 4             |
   +-------------------+-------------------+------------------+----------------------------+
   | cint32            | int16             | cint32           |          8 / 8             |
   +-------------------+-------------------+------------------+----------------------------+
   | cint32            | int32             | cint32           |          4 / 4             |
   +-------------------+-------------------+------------------+----------------------------+
   | cint32            | cint16            | cint32           |          4 / 4             |
   +-------------------+-------------------+------------------+----------------------------+
   | cint32            | cint32            | cint32           |          4 / 2             |
   +-------------------+-------------------+------------------+----------------------------+
   | float             | float             | float            |          8 / 4             |
   +-------------------+-------------------+------------------+----------------------------+
   | float             | cfloat            | cfloat           |          4 / 4             |
   +-------------------+-------------------+------------------+----------------------------+
   | cfloat            | float             | cfloat           |          4 / 4             |
   +-------------------+-------------------+------------------+----------------------------+
   | cfloat            | cfloat            | cfloat           |          4 / 2             |
   +-------------------+-------------------+------------------+----------------------------+

.. note:: * Type combination is not supported by AIE-ML device.

Template Parameters
===================

To see details on the template parameters for the Hadamard Product, see :ref:`API_REFERENCE`.


Access Functions
================

To see details on the access functions for the Hadamard Product, see :ref:`API_REFERENCE`.

Ports
=====

To see details on the ports for the Hadamard Product, see :ref:`API_REFERENCE`.

Design Notes
============
The performance of the IP heavily depends on the chosen data types. The data type combination determines the number of multiplications per clock cycle.

The IP performs a ceiling operation on the ``TP_DIM`` value to the nearest multiple of the vector size listed in :ref:`HADAMARD_output_type`:.


Super Sample Rate Operation
---------------------------

While the term Super Sample Rate strictly means the processing of more than one sample per clock cycle, in the AIE context it is taken to mean an implementation using parallel kernels to improve performance at the expense of additional resource use.
In the Hadamard Product, SSR operation is controlled by the template parameter ``TP_SSR``. This parameter is intended to improve performance and also allow support of point sizes beyond the limitations of a single tile.

The parameter ``TP_SSR`` allows a trade of performance for resource use in the form of tiles used.

The user may allocate samples to ports in an SSR configuration as desired, so long as the same convention is applied on both input ports. The output samples will simply reflect this pattern. For example, a Hadamard of size 64 and SSR =2 will have 2 input A ports of 32 and 2 input B ports of 32 and two output ports of size 32. If the user chooses to allocate even samples to the first port in each case then the first output port will hold even index sample outputs. If the user chooses to allocate the first 32 samples of A to the first A port and similarly for B, then the first output port will hold the first 32 index samples out.

It should be kept in mind that, individual Hadamard kernels will be processing a vector size of ``TP_DIM/TP_SSR``. The IP performs a ceiling operation on the ``TP_DIM/TP_SSR`` to nearest multiple of 32 bytes or 16 bytes depending on the implementation parameters.

Scaling
-------
Scaling in the Hadamard Product is controlled by the ``TP_SHIFT`` parameter which describes how many binary places by which to shift the result to the right, i.e. only power-of-2 scaling values are supported.
No scaling is applied at any point when the input data types is float or cfloat. Setting ``TP_SHIFT`` to any value other than 0 when ``TT_DATA_A/TT_DATA_B`` is float or cfloat will result in an error.

Saturation
----------
Distortion caused by saturation will be possible for Hadamard Product. Since the input values are input at run-time, no compile-time error can be issued for this hazard, so it is for the user to ensure that saturation does not occur.

Constraints
-----------
The Hadamard Product does not contain any constraints. It is a single kernel design except when ``TP_SSR>1`` in which case the port connections force placement of the tiles on separate tiles.

Code Example
============
.. literalinclude:: ../../../../L2/examples/docs_examples/test_hadamard.hpp
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



