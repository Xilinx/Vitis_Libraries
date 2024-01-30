.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.
.. meta::
   :keywords: Hash-Join-V4, Hash-Build-Probe-v4, hashJoinV4, hashBuildProbeV4
   :description: Describes the structure and execution of Hash-Join-V4 and Hash-Build-Probe-v4.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _guide-hash_join_v4:

********************************************************
Internals of Hash-Join-v4 and Hash-Build-Probe-v4
********************************************************

.. toctree::
   :hidden:
   :maxdepth: 2

This document describes the structure and execution of Hash-Join-V4 and Hash-Build-Probe-V4, implemented as :ref:`hashJoinV4 <cid-xf::database::hashJoinV4>` and :ref:`hashBuildProbeV4 <cid-xf::database::hashBuildProbeV4>` functions.

.. image:: /images/hashJoinV4Structure.png
   :alt: Hash Join MPU Structure
   :align: center

The Hash-Join-v4 and Hash-Build-Probe-v4 are general primitives to accelerate the Hash-Join algorithm utilizing the advantage of high memory bandwidth in AMD FPGAs. Compared with :ref:`hashJoinV3 <cid-xf::database::hashJoinV3>` and :ref:`hashBuildProbeV3 <cid-xf::database::hashBuildProbeV3>`, the v4 primitive provides Bloom-Filter to avoid redundant access to HBMs which can further reduce the efficiency of the Hash-Join algorithm. Hash-Join-v4 performs Hash-Join in single-call mode which means the small table and the big table should be scanned one after another. Hash-Build-Probe-v4 provides a separative solution for build and probe in Hash-Join. In Hash-Build-Probe, incremental build is supported, and the workflow can be scheduled as build0 -> build1 -> probe0 -> build2 -> probe2...

Workload is distributed based on the most significant bits (MSBs) of the hash value of the join key to the processing unit (PU), so that each PU can work independently. The current design uses a maximum number of eight PUs, served by four input channels through each of which a pair of key and payload can be passed in each cycle. 
Additionally, overflow processing is provided in this design.

The number of PU is set to 8, as each PU requires two dedicated bank to temporarily store rows in the small table (one for base rows and another for overflow rows). Due to the DDR/HBM memory access delay, four channels can serve enough data to these PUs. Each PU performs HASH-JOIN in three phases.

1. Build: With the small table as the input, the number of keys falls into each hash values are counted. The value of the hash counters are stored in bit vector in URAM. Every hash value has a fixed depth of storage in HBM to store the rows of the small table. If the counter of hash value is larger than the fixed depth, it means that overflow happens. Another bit vector is used for counting overflow rows. In the v4 primitive, the overflow hash counter is stored in HBM to save URAM storage for the bloom filter bit vector. Also, the overflow of small table will be stored in another area of the HBM.

.. image:: /images/build_sc_v4.png
   :alt: Build
   :align: center

2. Merge: Accumulating the overflow hash counter to get the offsets of each hash value. Then, the overflow rows will be read in from one HBM and write out to another HBM. The output address of overflow rows is according to the offset of its hash value. By operation of Merge, you can put the overflow rows into its right position and wait for Probe. To provide high throughput in this operation, two dedicated HBM ports are required for each PU, which provides read and write accessibilities at the same time.
   
.. image:: /images/merge_sc_v4.png
   :alt: Merge
   :align: center

3. Probe: The big table is read in and filtered by the bit vector of the bloom filter. It will reduce the size of the big table stream. You can get the number of hash collisions in the hash counter, and then the possible matched key and payload pairs can be retrieved from DDR/HBM to join with the big table's payload after key comparison.

.. image:: /images/probe_sc_v4.png
   :alt: Probe
   :align: center

.. IMPORTANT::
   Make sure the size of small table is smaller than the size of the HBM buffers. The small table and big table should be fed only ONCE.

.. CAUTION::
   Currently, this primitive expects a unique key in the small table.

The primitive has only one port for key input and one port for payload input. If your tables are joined by multiple key columns or has multiple columns as payload, use :ref:`combineCol <cid-xf::database::combineCol>` to merge the column streams, and use :ref:`splitCol <cid-xf::database::splitCol>` to split the output to columns.

There is a deep relation in the template parameters of the primitive. In general, the maximum capacity of rows and depth of the hash entry is limited by the size of HTB. 
Each PU has one HTB in this design, and the size of one HTB is equal to the size of one pseudo-channel in HBM. Here is an example of row capacity when PU=8:

  +----------+----------+--------------+--------------+------------+--------------------------+-------------------+
  | HTB Size | Key Size | Payload Size | Row Capacity | Hash Entry | Max Depth for Hash Entry | Overflow Capacity |
  +----------+----------+--------------+--------------+------------+--------------------------+-------------------+
  | 8x256MB  |  32 bit  |   32 bit     |      64M     |     1M     | 63 (base rows take 63M)  |         1M        |
  +----------+----------+--------------+--------------+------------+--------------------------+-------------------+
  | 8x256MB  | 128 bit  |   128 bit    |      16M     |     1M     | 15 (base rows take 15M)  |         1M        |
  +----------+----------+--------------+--------------+------------+--------------------------+-------------------+

The number of hash entry is limited by the number of URAM in a single SLR. For example, there are 320 URAMs in a SLR of the AMD Alveo™ U280, and 1M hash entry will take 64 URAMs.
Because the number of hash entry must be the power of two, you have to control the size of the hash entry in the bloom filter and hash counter to avoid crossing the SLR logic.
This table shows an example for URAM utilization in different size of hash entries:

+---------------------+-------------------------+-----------------------+--------------+---------------------+-----------+
| Bloom Filter Number | Bloom Filter Hash Entry | URAM for Bloom Filter |   Hash Entry | URAM for Hash Entry | Total URAM|
+---------------------+-------------------------+-----------------------+--------------+---------------------+-----------+
|      1              |     16M(16Mx1bit)       | 1x64(3x64x64bitx4K)   | 2M(2Mx16bit) |   128(64x64bitx4K)  |     192   |
+---------------------+-------------------------+-----------------------+--------------+---------------------+-----------+
|      3              |     16M(16Mx1bit)       | 3x64(3x64x64bitx4K)   | 1M(1Mx16bit) |    64(64x64bitx4K)  |     256   |
+---------------------+-------------------------+-----------------------+--------------+---------------------+-----------+