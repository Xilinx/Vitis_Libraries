..
   Copyright © 2019–2024 Advanced Micro Devices, Inc
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _BITONIC_SORT:

============
Bitonic Sort
============

This library element bitonically sorts the input list.
The bitonic sort has configurable data types and list sizes, along with a configurable number of frames, ascending or descending sort, and cascading support.
Template parameters are used to configure the top level graph of the bitonic_sort_graph class.

Entry Point
===========

The graph entry point is the following:

.. code-block::

    xf::dsp::aie::bitonic_sort::bitonic_sort_graph

Device Support
==============

The Bitonic Sort Product library element supports AIE and AIE-ML devices.

Supported Types
===============
The data type is controlled by ``TT_DATA``, and can be one of 4 choices: int16, uint16 (not available on AIE), int32 or float.


Template Parameters
===================

To see details on the template parameters for the Bitonic Sort, see :ref:`API_REFERENCE`.


Access Functions
================

To see details on the access functions for the Bitonic Sort, see :ref:`API_REFERENCE`.

Ports
=====

To see details on the ports for the Bitonic Sort, see :ref:`API_REFERENCE`. Note that the type of ports are determined by the configuration of template parameters.

Design Notes
============

Cascade Feature
---------------

The Bitonic Sort is configured using the ``TP_CASC_LEN`` template parameter. This determines the number of kernels over which the Bitonic Sort is split. To be clear, this feature does not use the cascade ports of kernels to convey any data. IO buffers are used to convey data from one kernel to the next in the chain. The term cascade is used simply in the sense that the function is split into a series of operations which are executed by a series of kernels, each on a separate tile. The Bitonic Sort is split at stage boundaries, so the ``TP_CASC_LEN`` cannot exceed the number of bitonic stages log2(n)*(log2(n)+1)/2.

Constraints
-----------
The Bitonic Sort input ``TP_DIM`` must be a power of 2. ``TP_DIM * size_of(TT_DATA)`` must have a minimum value of 64 bytes (size of buffer on AIE * 2).

Code Example
============
.. literalinclude:: ../../../../L2/examples/docs_examples/test_bitonic_sort.hpp
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



