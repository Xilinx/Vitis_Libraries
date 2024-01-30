.. 
   
.. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _guide-stream_reorder:

*****************************************
Internals of streamReorder
*****************************************

.. toctree::
   :hidden:
   :maxdepth: 3

This document describes the structure and execution of streamReorder,
implemented as :ref:`streamReorder <cid-xf::common::utils_hw::streamReorder>` function.

.. image:: /images/stream_reorder.png
   :alt: stream reoder Structure
   :width: 80%
   :align: center

The streamReorder adjusts the output order within a fix size group. Suppose the fix size is Wn, each Wn input data is reordered to output following the pattern from input configuration.

For example, Wn = 4, reorder config is 2,1,0,3.

Input order is 1,2,3,4,5,6,7 (1 is first input and 7 is last).

Each group of 4 data elements is reordered: 0 1 2 3 --> 2 1 0 3 and 4 5 6 7 --> 6 5 4 7.

Output is 2,1,0,3,6,5,4,7.

The design of this primitive applies ping-pong arrays to obtain a high performance. One is storing the input data while output data from another one.

.. CAUTION::
    Applicable conditions.

    1. The length of input stream is a multiple (>=1) of the length of reorder config stream set by the  ``_WindowSize`` template parameter.

    2. The configuration is loaded once in one invocation, and reused until the end.

    3. The types of input stream and output stream are same.

