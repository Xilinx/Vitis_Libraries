.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Hash-Join-V3, Hash-Build-Probe-v3, hashJoinV3, hashBuildProbeV3
   :description: Describes the structure and execution of Hash-Join-V3 and Hash-Build-Probe-v3.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _guide-hash_join_v3:

********************************************************
Internals of Hash-Join-v3 and Hash-Build-Probe-v3
********************************************************

.. toctree::
   :hidden:
   :maxdepth: 2

This document describes the structure and execution of Hash-Join-V3 and Hash-Build-Probe-v3, implemented as :ref:`hashJoinV3 <cid-xf::database::hashJoinV3>` and :ref:`hashBuildProbeV3 <cid-xf::database::hashBuildProbeV3>` functions.

.. image:: /images/hashJoinV3Structure.png
   :alt: Hash Join MPU Structure
   :align: center

The Hash-Join-v3 and Hash-Build-Probe-v3 are general primitives to accelerate the Hash-Join algorithm utilizing the advantage of high memory bandwidth in an AMD FPGA. Hash-Join-v3 performs Hash-Join in single-call mode which means the small table and the big table should be scanned one after another. Hash-Build-Probe-v3 provides a separative solution for build and probe in Hash-Join. In Hash-Build-Probe, an incremental build is supported, and the workflow can be scheduled as build0 -> build1 -> probe0 -> build2 -> probe2...

Workload is distributed based on the most significant bits (MSBs) of the hash value of join key to the processing unit (PU), so that each PU can work independently. The current design uses a maximum number of PUs, served by four input channels through each of which a pair of key and payload can be passed in each cycle. Overflow processing is provided in this design.

The number of PU is set to 8, as each PU requires two dedicated bank to temporarily store rows in the small table (one for base rows and another for overflow rows). Due to the DDR/HBM memory access delay, four channels can serve enough data to these PUs. Each PU performs HASH-JOIN in three phases.

1. Build: With small table as input, the number of keys falls into each hash values are counted. The value of hash counters are stored in bit vector in URAM. Every hash value has a fixed depth of storage in HBM to store rows of small table. If the counter of hash value is larger than the fixed depth, it means that overflow happens. Another bit vector is used for counting overflow rows. Also, the overflow of small table will be stored in another area in the HBM.

.. image:: /images/build_sc_v3.png
   :alt: Build
   :align: center

2. Merge: Accumulating the overflow hash counter to get the offsets of each hash value. Then, the overflow rows will be read in form one HBM and write out to another HBM. The output address of overflow rows is according to the offset of its hash value. By operation of Merge, you can put the overflow rows into its right position and wait for Probe. To provide high throughput in this operation, two dedicated HBM ports are required for each PU, which provides read and write accessibilities at the same time.
   
.. image:: /images/merge_sc_v3.png
   :alt: Merge
   :align: center

3. Probe: Finally, the big table is read in, and you can know the number of hash collision and offset of overflow rows with the same hash by referencing the hash counter in URAM. Then the possible matched key and payload pairs can be retrieved from HBM and joined with big table payload after key comparison.

.. image:: /images/probe_sc_v3.png
   :alt: Probe
   :align: center

.. IMPORTANT::
   Make sure the size of small table is smaller than the size of HBM buffers. Small table and big table should be fed only ONCE.

.. CAUTION::
   Currently, this primitive expects a unique key in the small table.

The primitive has only one port for key input and one port for payload input. If your tables are joined by multiple key columns or have multiple columns as payload, use :ref:`combineCol <cid-xf::database::combineCol>` to merge the column streams, and use :ref:`splitCol <cid-xf::database::splitCol>` to split the output to columns.

There is a deep relation in the template parameters of the primitive. In general, the maximum capacity of rows and the depth of the hash entry is limited by the size of HTB. Each PU has one HTB in this design, and the size of one HTB is equal to the size of one pseudo-channel in HBM. Here is an example of row capacity when PU=8:

  +----------+----------+--------------+--------------+--------------+--------------------------+-------------------+
  | HTB Size | Key Size | Payload Size | Row Capacity |  Hash Entry  | Max Depth for Hash Entry | Overflow Capacity |
  +----------+----------+--------------+--------------+--------------+--------------------------+-------------------+
  | 8x256MB  |  32 bit  |   32 bit     |      64M     |     1M       | 63 (base rows take 63M)  |         1M        |
  +----------+----------+--------------+--------------+--------------+--------------------------+-------------------+
  | 8x256MB  | 128 bit  |   128 bit    |      16M     |     1M       | 15 (base rows take 15M)  |         1M        |
  +----------+----------+--------------+--------------+--------------+--------------------------+-------------------+

The number of hash entry is limited by the number of URAM in a single SLR. For example, there are 320 URAMs in a SLR of the AMD Alveo™ U280, and 1M hash entry will take 192 URAMs (96 URAMs for base hash counter + 96 URAMs for overflow hash counter). Because the number of hash entry must be the power of 2, 1M hash entry is the maximum for the Alveo U280 to avoid crossing SLR logic, which will lead to a bad timing performance of the design.