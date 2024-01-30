.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: bloom-filter, bloomFilter, bv-update, bv-check
   :description: Describes the structure and execution of the bloom filter module.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


.. _guide-bloom_filter:

********************************************************
Internals of Bloom-Filter
********************************************************

.. toctree::
   :hidden:
   :maxdepth: 1

This document describes the structure and execution of the bloom filter module, implemented as a :ref:`bloomFilter <cid-xf::database::bfGen>` function.

The structure of ``bloomFilter`` is described as follows. The primitive has two functions, ``bv-update`` and ``bv-check``. It takes block RAM or URAM as internal storage for the bloom filter vectors. The input hash value is the addressing parameter for updating the bloom filter vector and checking the built vector. Chip select is based on the most significant bits (MSBs) of the hash value, and the width select is based on its least significant bits (LSBs). The total storage size is related to the value of ``1 << BV_W``. Make sure the storage size is less than the maximum storage size of a single SLR, otherwise, it will result in a placing failure.

.. image:: /images/bloom_filter.png
   :alt: Bloom Filter Top Structure
   :align: center

The primitive provides an efficient way to filter redundant data. It can be easily applied on the Hash Join primitive to eliminate false shoots in the Hash Join Probe, as shown below:

.. image:: /images/bloom_filter_performance.png
   :alt: Bloom Filter Performance
   :align: center