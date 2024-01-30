.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Hash-Join, hashJoinMPU, combineCol, splitCol
   :description: Describes the structure and execution of Hash-Join Multi-Process-Unit version.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _guide-hash_join_mpu:

********************************************************
Internals of Hash-Join (Multi-Process-Unit Version)
********************************************************

.. toctree::
   :hidden:
   :maxdepth: 2

This document describes the structure and execution of the Hash-Join Multi-Process-Unit Version, implemented as a :ref:`hashJoinMPU <cid-xf::database::hashJoinMPU>` function.

.. image:: /images/hash_join_mpu.png
   :alt: Hash Join MPU Structure
   :align: center

The Hash-Join primitive has a clustered design internally to use the advantage of high memory bandwidth in AMD FPGAs. Workload is distributed based on the most significant bits (MSBs) of the hash value of the join key to processing units (PUs), so that each PU can work independently. The current design uses eight PUs, served by four input channels through each of which a pair of key and payload can be passed in each cycle.

The number of PU is set to 8, as each PU requires a dedicated bank to avoid conflicts, and due to the DDR/HBM memory access delay, four channels can serve enough data to these PUs. Each PU performs Hash-Join in three phases.

1. build bitmap: With small table as the input, the number of keys falls into each of the hash values are counted. The number of counts are stored in a bit vector in URAM. After a full scan of the small table, the bit vector is walked once, accumulating the counts to offsets of each hash.

2. build unit: The small table is read in again and stored into DDR/HBM buffers of that PU. By referencing the bit vector in URAM created in previous phase, the kernel knows where to find empty slots for each key,and once a small table payload and key is written into DDR/URAM, the offset in bit vector is increased by 1, so that the next key of same hash value can be written into a different place. As the number of keys with each hash have been counted, such an offset increase will not step into another key's slot.

3. probe unit: Finally, the big table is read in, and again by referencing the bit vector with hash of key, you can know the offset of this hash and number of keys with the same hash. Then the possible matched key and payload pairs can be retrieved from DDR/HBM and joined with big table payload after key comparison.

.. IMPORTANT::
   To reduce the storage size of hash-table on the FPGA board, the small table has to be scanned in TWICE, followed by the big table ONCE.

.. CAUTION::
   Currently, this primitive expects unique key in the small table.

This ``hashJoinMPU`` primitive has only one port for key input and one port for payload input. If your tables are joined by multiple key columns or has multiple columns as payload, use :ref:`combineCol <cid-xf::database::combineCol>` to merge the column streams, and use :ref:`splitCol <cid-xf::database::splitCol>` to split the output to columns.

There are two versions of this primitive currently, with different number of slots for hash collision and key duplication. The version with more slots per hash entry has less total row capacity, summarized as follows:

  +--------------+----------------+
  | row capacity | hash slots     |
  +--------------+----------------+
  | 2M           | 262144 (0.25M) |
  +--------------+----------------+
  | 8M           | 512 (0.5K)     |
  +--------------+----------------+