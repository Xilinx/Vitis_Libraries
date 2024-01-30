.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Bitonic, sort, bitonicSort
   :description: Describes the structure and execution of the Bitonic sort.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _guide-bitonic_sort:

********************************************************
Internals of Bitonic Sort
********************************************************

.. toctree::
   :hidden:
   :maxdepth: 2


This document describes the structure and execution of Bitonic Sort, implemented as a :ref:`bitonicSort <cid-xf::database::bitonicSort>` function. Bitonic Sort is a special kind of sorting network, where the sequence of comparisons is not data dependent. This makes sorting networks suitable for implementation in hardware or in parallel processor arrays. The computing complexity of bitonic sort is O(n*log(n)2).


.. image:: /images/bitonic_sort_architecture.png
   :alt: Bitonic Sort Processing Structure
   :align: center


Bitonic Sort has giant data throughput, and it demands large resource same time. It is well-fitted for the application with a high band of data input. The table shows the resource consumption for an instance of bitonic sort with input bitwidth=32.

                        +-------------------+----------+-----------+-----------+-----------+
                        | BitonicSortNumber | 8        | 16        | 32        | 64        |
                        +-------------------+----------+-----------+-----------+-----------+
                        | Lantency          | 22       | 42        | 79        | 149       |
                        +-------------------+----------+-----------+-----------+-----------+
                        | Interval          | 9        | 17        | 33        | 65        |
                        +-------------------+----------+-----------+-----------+-----------+
                        | LUT               | 2647     | 7912      | 21584     | 58064     |
                        +-------------------+----------+-----------+-----------+-----------+
                        | Register          | 3136     | 9291      | 26011     | 69160     |
                        +-------------------+----------+-----------+-----------+-----------+

If the Bitonic Sort number grows twice, the resource consumption of Bitonic Sort will grow around four times, theoretically.


.. image:: /images/bitonic_sort_resource_consumption.png
   :alt: Bitonic Sort Resource Consumption in FPGA
   :align: center


.. IMPORTANT::
   The current version of Bitonic Sort is stream in and stream out. The bitonic sort number must be a power of two because of the algorithm restriction. Combine it with the Merge Sort primitive can achieve an arbitrary sort number; see the reference :ref:`guide-merge_sort`.

.. CAUTION::
   The size of the Bitonic Sort number should be set with the consideration of the FPGA resources to pass place and route.

This ``bitonicSort`` primitive has one port for key input, one port for payload input, one port for key output, one port for payload output, and one boolean sign for indicating an ascending sort or descending sort.