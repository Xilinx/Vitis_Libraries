..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _BITONIC_SORT:

============
Bitonic Sort
============

This library element bitonically sorts the input list.
The bitonic sort has configurable data types and list sizes, along with a configurable number of frames, ascending or descending sort, and cascading support. Additionally, there is support for Super Sample Rate, which allows multiple sets of input data to be processed and sorted into a single sorted list.
Template parameters are used to configure the top level graph of the bitonic_sort_graph class.

Entry Point
===========

The graph entry point is the following:

.. code-block::

    xf::dsp::aie::bitonic_sort::bitonic_sort_graph

Device Support
==============

The Bitonic Sort Product library element supports AIE, AIE-ML and AIE-MLv2 devices.

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

Super Sample Rate
-----------------

The ``TP_SSR`` template parameter enables the use of multiple bitonic sort kernels to process larger datasets more efficiently. When ``TP_SSR > 1``, the bitonic sort graph will instantiate ``TP_SSR`` bitonic sort kernels, each responsible for sorting a subset of the input data. These sorted sublists are then passed through a tree of ``TP_SSR - 1`` merge sort kernels, which combine the sublists into a single sorted output stream.

The performance of a single bitonic sort kernel diminishes as the list size (``TP_DIM``) increases due to the increasing number of stages in the bitonic sort. For larger list sizes, it is more efficient to split the workload across multiple bitonic sort kernels, each sorting ``TP_DIM / TP_SSR`` samples. This approach improves performance, especially for larger datasets, but at the cost of resources. On the lower end of ``TP_DIM`` sizes, performance is generally better with a single kernel (``TP_SSR = 1``), as the overhead of merging sublists is avoided.

Using multiple bitonic sort kernels also increases the maximum possible list size (``TP_DIM``) that can be sorted. With a single kernel, the maximum ``TP_DIM`` is limited by the data memory of an AI Engine tile. By splitting the sort across ``TP_SSR`` kernels, the maximum ``TP_DIM`` can be increased by a factor of ``TP_SSR``.

The bitonic sort graph configured with ``TP_SSR > 1`` will have ``TP_SSR`` input IO buffers and a single output stream. Note that ``TP_SSR`` is only supported when the number of frames ``TP_NUM_FRAMES`` is set to 1.


Constraints
-----------
The Bitonic Sort input ``TP_DIM`` must be a power of 2. ``TP_DIM * size_of(TT_DATA) / TP_SSR`` must have a minimum value of 64 bytes (size of buffer on AI Engine * 2).

Code Example
============
.. literalinclude:: ../../../../L2/examples/docs_examples/test_bitonic_sort.hpp
    :language: cpp
    :lines: 17-

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:



