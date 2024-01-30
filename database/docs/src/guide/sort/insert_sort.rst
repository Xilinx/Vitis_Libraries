.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Insert, sort, insertSort
   :description: Describes the structure and execution of the Insert sort.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _guide-insert_sort:

********************************************************
Internals of Insert Sort
********************************************************

.. toctree::
   :hidden:
   :maxdepth: 2

This document describes the structure and execution of Insert Sort, implemented as a :ref:`insertSort <cid-xf::database::insertSort>` function.

Principle
~~~~~~~~~

Insert sort is a simple sorting algorithm that builds the final sorted array (or list) one item at a time. It is much less efficient on large lists than more advanced algorithms such as quicksort, heapsort, or merge sort. However, insert sort provides several advantages:

1. Simple implementation

2. Efficient for (quite) small data sets, much like other quadratic sorting algorithms.

3. The time complexity is O(nk) when each element in the input is no more than k places away from its sorted position.

4. Only requires a constant amount O(1) of additional memory space.

5. Sort a list as it receives it.

For its FPGA implementation, a dedicated structure is designed as follows:

.. image:: /images/insert_sort_architecture.png
   :alt: Insert Sort Processing  Structure
   :align: center

The Insert Sort primitive has an internal shift register array to sort the input stream. It takes five steps to finish the sort processing of a stream with a limited max sort number. 

1. Build a group of shift registers, and the number of shift register is the maximum sort number.

2. Broadcasting the input value to every shift registers, then comparing size between the internal value of each shift register and the input value. For descending sort, run step 3. Otherwise, run step 4.

3. For descending sort, you should build a internal ascending array. If the input value is larger than array[i], then right shift array[i]. If the input value is less than array[i] and larger than array[i+1], insert the input value to array[i+1];

4. For ascending sort, you should build an internal descending array. If the input value is less than array[i], then right shift array[i]. If the input value is larger than array[i] and less than array[i+1], insert the input value to array[i+1];

5. If the input stream is not empty, output the last value of the array, and then return to step 2. Otherwise, right shift the whole register array and output the last array value until the array is empty.

Synthesis Results
~~~~~~~~~~~~~~~~~

For bitwidth=32, the resource consumption for different max sort number is listed in the following table:

+-----------------+----------+-----------+-----------+-----------+-----------+-----------+-----------+
| Max_sort_number | 8        | 16        | 32        | 64        | 128       | 256       | 512       |
+-----------------+----------+-----------+-----------+-----------+-----------+-----------+-----------+
| Interval        | 1        | 1         | 1         | 1         | 1         | 1         | 1         |
+-----------------+----------+-----------+-----------+-----------+-----------+-----------+-----------+
| LUT             | 343      | 607       | 1135      | 2144      | 4192      | 8285      | 16477     |
+-----------------+----------+-----------+-----------+-----------+-----------+-----------+-----------+
| Register        | 1007     | 1835      | 3451      | 6692      | 13156     | 26082     | 51885     |
+-----------------+----------+-----------+-----------+-----------+-----------+-----------+-----------+

.. image:: /images/insert_sort_resource.png
   :alt: Insert Sort Resource Consumption
   :align: center

Insert Sort primitive sets 1024 as the default maximum sort number. To achieve an arbitrary sort number, first the input stream will be sorted every 1024 number by the Insert Sort primitive, then use the Merge Sort primitive to merge the sorted stream (see reference :ref:`guide-merge_sort`). The following figure shows the synthesis result for the maximum sort number of 1024.

.. image:: /images/insert_sort_synthesis_resource.png
   :alt: Insert Sort Synthesis
   :align: center


.. image:: /images/insert_sort_loop_synthesis.png
   :alt: Insert Sort Loop
   :align: center


Implementation Results
~~~~~~~~~~~~~~~~~~~~~~

This is the implementation result of the Insert Sort primitive with Max_sort_number=1024:

.. image:: /images/insert_sort_implementation_resource.png
   :alt: Insert Sort Implementation
   :align: center

.. IMPORTANT::
   The max sort number should be less than 1024 because the array partition can only support an array size smaller then 1024. For an arbitary sort number, the Merge Sort primitive is required :ref:`guide-merge_sort`.

.. CAUTION::
   The size of input stream should be larger than the max sort number, otherwise the internal shift register is not fully initialized.

This ``insertSort`` primitive has one port for key input, one port for payload input, one port for key output, one port for payload output, and one boolean sign for indicating an ascending sort or descending sort.