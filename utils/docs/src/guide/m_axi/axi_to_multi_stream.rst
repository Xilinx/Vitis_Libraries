.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _guide-axi_to_multi_stream:

********************************
Internals of axiToMultiStream
********************************

.. toctree::
   :hidden:
   :maxdepth: 2

This document describes the structure and execution flow of axiToMultiStream,
implemented as :ref:`axiToMultiStream <cid-xf::common::utils_hw::axiToMultiStream>` function.

.. image:: /images/axi_to_multi_stream.png
   :alt: axi_to_multi_stream Structure
   :width: 80%
   :align: center

The axiToMultiStream primitive is non-blocking and uses round robin to load multiple categories of data from one AXI master into multiple streams.
For example, in the implementation of one AXI port loading three types of data, the data of each type could be tightly packed.
Each type of data's length should be multiple of 8 bits. The three types of data width could be unaligned or aligned. For example, three types of data compressed in one binary file. AXI port is assumed to have width as multiple of 8-bit char.

.. CAUTION::
   Applicable condition:

   This module only supports three categories of data.

   When input data ptr width is less than AXI port width, the AXI port bandwidth
   is not fully used. So, AXI port width should be minimized while meeting
   performance requirements of application.


This primitive has two modules working simultaneously.

1. ``read_to_vec_stream``: It reads AXI master to three BRAM buffers,
   and loads the buffers in non-blocking and round robin way to ``_WAxi`` width stream.

2. ``split_vec_to_aligned``: It takes the ``_WAxi`` width stream, aligns to the stream width, and splits the _WAxi width data to stream width and output the stream.

