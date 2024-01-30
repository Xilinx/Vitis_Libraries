.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _guide-axi_to_stream:

********************************
Internals of axiToStream
********************************

.. toctree::
   :hidden:
   :maxdepth: 2

This document describes the structure and execution of axiToStream,
implemented as :ref:`axiToStream <cid-xf::common::utils_hw::axiToStream>` function.

.. image:: /images/axi_to_stream.png
   :alt: two types of axi_to_stream Structure
   :width: 80%
   :align: center

The axiToStream for aligned data implement is a lightweight primitive for aligned data, the width of AXI port
is positive integer multiple of alignment width and the stream's width just equals the aligned width. Both AXI port
and alignment width are assumed to be multiple of 8-bit char.

The axiToStream for general data is relatively universal compared to the axiToStream for aligned data,
so it causes more resource. The data length should be in number of 8-bit char. The data width could be unaligned or aligned. For exapmple, compressed binary files. AXI port is assumed to have width as multiple of 8-bit char.

.. CAUTION::
   Applicable conditions:

   When input data pointer width is less than AXI port width, the AXI port bandwidth
   is not fully used. So, AXI port width should be minimized while meeting
   performance requirements of application.

This primitive performs axiToStream in two modules working simultaneously.

1. ``read_to_vec``: It reads from AXI master to a ``_WAxi`` width stream.

2. ``split_vec_to_aligned``: It consumes the ``_WAxi`` width stream, splits and aligns the wide data to
   stream width, and writes the data into stream.

This ``axiToStream`` primitive has only one port for axi ptr and one port for stream output.

