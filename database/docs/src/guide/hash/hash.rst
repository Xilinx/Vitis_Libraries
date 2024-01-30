.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Hash, hashLookup3, hashMurmur3, Lookup3, Murmur3, 
   :description: Describes the structure and execution of hash functions, implemented as hashLookup3 and hashMurmur3.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


.. _guide-hash:

********************************************************
Internals of Lookup3 and Murmur3 Hash
********************************************************

.. toctree::
   :hidden:
   :maxdepth: 1

This document describes the structure and execution of hash functions, implemented as ``hashLookup3`` and ``hashMurmur3``.

-------------------------------------
Murmur3 and Lookup3 Hash Introduction
-------------------------------------

For different types of data, some are highly random and the others are high-latitude graph structures. These make it difficult to find a common hash function. Even for a specific type of data, a better hash function is not an easy one. The hash function can be selected from two perspectives:

1. Data distribution: One kind of measurement is to consider whether a hash function can distribute the set of data very well. To perform this kind of analysis, you need to know the number of hash values for the collision. If a linked list is used to process the collision, the average length of the linked list and the number of groups are the features to show the performance of the hash function.

2. The efficiency of the hash function: The other criterion to measure is the efficiency on which the hash function gets a hash value. In general, the computation complexity of the hash function is assumed to be O(1), that is why the time complexity of searching data in the hash table is considered to be "comparable with an average of O(1)". In other data structures, such as graphs (usually implemented as red-black trees), are considered as O(log n) complexity.

The Murmur3 and Lookup3 hash functions are called simple hash functions, and they are usually used for hashing string data. Unlike sha256, sha256, which are encrypted and not password-safe, they can directly generate a key for an associative container, such as a hash table.

----------------------------------------
Acceleration of Murmur3 and Lookup3 Hash
----------------------------------------

Murmur3 and Lookup3 have 32 bit, 64 bit, 128 bit, and other bit width algorithm cores for X86 and X64. Murmur3 uses cyclic shift and multiplication operations, while Lookup3 uses circular shift and addition. On the FPGA, it is suitable for concurrent processing in large amounts of data and maintain high throughput as well. Therefore, within the ensurement of consistency (limited by the working frequency, initiation interval), Murmur3 and Lookup3 hash can process a larger bit width of input.

Therefore, the results of the development with different input bit widths are shown in the following tables. The 512-bit input is optimal when the operating frequency of 300 MHz is satisfied. After running the AMD Vivado™ synthesis/implementation, the comparison tables are as follows:

1. Murmur3:

   +----------+-----------+----------+----------+----------+---------+---------+
   |          | 1024i-32o | 512i-32o | 256i-32o | 128i-32o | 64i-32o | 32i-32o |
   +----------+-----------+----------+----------+----------+---------+---------+
   |   CLB    | 1070      | 654      | 285      | 139      | 78      | 45      |
   +----------+-----------+----------+----------+----------+---------+---------+
   |   LUT    | 3824      | 1887     | 917      | 461      | 263     | 156     |
   +----------+-----------+----------+----------+----------+---------+---------+
   |   FF     | 5472      | 2832     | 1512     | 788      | 458     | 308     |
   +----------+-----------+----------+----------+----------+---------+---------+
   |   DSP    | 294       | 150      | 78       | 42       | 24      | 15      |
   +----------+-----------+----------+----------+----------+---------+---------+
   |   II     | 1         | 1        | 1        | 1        | 1       | 1       |
   +----------+-----------+----------+----------+----------+---------+---------+
   | Lantency | 31        | 23       | 19       | 17       | 16      | 15      |
   +----------+-----------+----------+----------+----------+---------+---------+

2. Lookup3:

   +----------+-----------+----------+----------+----------+---------+---------+
   |          | 1024i-32o | 512i-32o | 256i-32o | 128i-32o | 64i-32o | 32i-32o |
   +----------+-----------+----------+----------+----------+---------+---------+
   |   CLB    | 1080      | 515      | 227      | 128      | 56      | 33      |
   +----------+-----------+----------+----------+----------+---------+---------+
   |   LUT    | 7351      | 3501     | 1536     | 824      | 267     | 206     |
   +----------+-----------+----------+----------+----------+---------+---------+
   |   FF     | 6136      | 3152     | 1464     | 816      | 328     | 232     |
   +----------+-----------+----------+----------+----------+---------+---------+
   |   DSP    | 0         | 0        | 0        | 0        | 0       | 0       |
   +----------+-----------+----------+----------+----------+---------+---------+
   |   II     | 1         | 1        | 1        | 1        | 1       | 1       |
   +----------+-----------+----------+----------+----------+---------+---------+
   | Lantency | 44        | 24       | 12       | 8        | 4       | 4       |
   +----------+-----------+----------+----------+----------+---------+---------+