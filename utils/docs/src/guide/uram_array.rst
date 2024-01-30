.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _guide-class-uram_array:

********************************
Internals of UramArray
********************************

.. toctree::
   :hidden:
   :maxdepth: 2

The :ref:`UramArray <cid-xf::common::utils_hw::UramArray>` class aims to
help users to achive faster update rate to data stored in URAM blocks.

Work Flow
=========

.. figure:: /images/uram_array.png
    :alt: bit field
    :align: center

This module enables fast data update by creating a small history cache of
recently written data in register beside the URAM blocks.
Upon data read, it looks up the address in recent writes, and forwards
the result if the match is found.

It also provides a handy interface for initializing multiple URAM blocks
used as an array in parallel.

Storage Layout
==============

URAM blocks have fixed width of 72-bit, so our storage layout depends on the width of the element.

When the data element has no more than 72 bits, the helper class tries to
store as many as possible within 72 bits and pad zeros when some space is left.
For example, to store 20k 16-bit elements, two URAMs are used,
as each line can store four elements and each URAM has a fixed depth of 4k.

When the data element has more than 72 bits, the helper class uses a line of
multiple URAM blocks to store each element. This ensures that that we can initiate an element access during each cycle.
Hence, six URAM blocks are required to store 10k 128-bit elements.


Resources
=========

The hardware resources for 10k elements in the post- AMD Vivado |trade|  report are listed in the following
table:

.. table:: Hardware resources for URAM
    :align: center

    +-------------+----------+----------+-----------+-----------+-------------+
    |    _WData   |   URAM   |    FF    |    LUT    |  Latency  |  clock(ns)  |
    +-------------+----------+----------+-----------+-----------+-------------+
    |      64     |     3    |   3215   |   1868    |   10243   |    2.046    |
    +-------------+----------+----------+-----------+-----------+-------------+
    |      128    |     6    |   4000   |   2457    |   10243   |    2.046    |
    +-------------+----------+----------+-----------+-----------+-------------+


.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:

