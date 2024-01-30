.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.
.. meta::
   :keywords: Merge, sort, mergeSort
   :description: Describes the structure and execution of the Merge sort.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _guide-merge_sort:

********************************************************
Internals of Merge Sort
********************************************************

.. toctree::
   :hidden:
   :maxdepth: 2

Principle
~~~~~~~~~

This document describes the structure and execution of Merge Sort, implemented as a :ref:`mergeSort <cid-xf::database::mergeSort>` function.

The algorithm works in software as follows:

1. Divide the unsorted list into N sublists with each containing one element (a list of one element is considered sorted).

2. Repeatedly merge the sublists to produce new sorted sublists until there is only one sublist remaining. This will be the sorted list.

For FPGA implementation, a hardware oriented design is realized in the Merge Sort primitive.

.. image:: /images/merge_sort_architecture.png
   :alt: Merge Sort Processing Structure
   :align: center

The Merge Sort primitive has an internal comparator to sort two input stream into one output stream.

Steps for descending (vice versa):

1. Read the first right value and the first left value.

2. Compare the two values and output the larger one.

3. If the output value in step 2 is from the right stream and the right stream is not empty, then keep the read value from the right stream. Otherwise, read from the left stream.

4. If both stream are empty, break the loop. Otherwise, return to step 2.

Synthesis Results
~~~~~~~~~~~~~~~~~

.. image:: /images/merge_sort_synthesis_resource.png
   :alt: Merge Sort Synthesis
   :align: center

Implementation Results
~~~~~~~~~~~~~~~~~~~~~~

.. image:: /images/merge_sort_implementation_resource.png
   :alt: Merge Sort Implementation
   :align: center

.. IMPORTANT::
   The end flag of the input stream should be initialized; otherwise, it can cause deadlock in the output stream. The input stream of the Merge Sort primitive should be a pre-sorted stream.

.. CAUTION::
   If the two input stream are both empty, the function output will be also empty.

This ``mergeSort`` primitive has two ports for key input, two ports for payload input, one port for merged key output, one port for merged payload output, and one boolean sign for indicating ascending sort or descending sort.