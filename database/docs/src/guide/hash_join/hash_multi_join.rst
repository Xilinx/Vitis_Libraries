.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Hash-Multi-Join, Hash-Join, hashMultiJoin
   :description: Describes the structure and execution of Hash-Multi-Join.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _guide-hash_multi_join:

********************************************************
Internals of Hash-Multi-Join
********************************************************

.. toctree::
   :hidden:
   :maxdepth: 2

This document describes the structure and execution of Hash-Multi-Join, implemented as a :ref:`hashMultiJoin <cid-xf::database::hashMultiJoin>` function. Hash-Multi-Join is a combination Join algorithm, which can perform Hash-Join, Hash-Semi-Join and Hash-Anti-Join with programmable ability. It is based on the framework of ``Hash-Join-v3`` as shown in the following figure:

.. image:: /images/hashJoinV3Structure.png
   :alt: Hash Join MPU Structure
   :align: center

The number of PU is set to 8, as each PU requires two dedicated banks to temporarily store rows in the small table (one for base rows and another for overflow rows). Because of the DDR/HBM memory access delay, four channels can serve enough data to these PUs. Each PU performs Hash-Join in three phases, and details are described in :ref:`guide-hash_join_v3`. The control of Multi-Join function is as follows:

+------------+----------------+
|   Flag     |      Type      |
+------------+----------------+
| JOIN_INNER | Hash-Join      |
+------------+----------------+
| JOIN_SEMI  | Hash-Semi-Join |
+------------+----------------+
| JOIN_ANTI  | Hash-Anti-Join |
+------------+----------------+

.. IMPORTANT::
   Make sure the size of the small table is smaller than the size of the HBM buffers. The small table and big table should be fed only ONCE.

.. CAUTION::
   Currently, this primitive expects unique key in small table.

The primitive has only one port for key input and one port for payload input. If your tables are joined by multiple key columns or has multiple columns as payload, use :ref:`combineCol <cid-xf::database::combineCol>` to merge the column streams, and use :ref:`splitCol <cid-xf::database::splitCol>` to split the output to columns.

There is a deep relation in the template parameters of the primitive. In general, the maximum capacity of rows and depth of hash entry is limited by the size of HTB. Each PU has one HTB in this design, and the size of one HTB is equal to the size of one pseudo-channel in HBM. Here is an example of the row capacity when PU=8:

  +----------+----------+--------------+--------------+--------------+--------------------------+-------------------+
  | HTB Size | Key Size | Payload Size | Row Capacity |  Hash Entry  | Max Depth for Hash Entry | Overflow Capacity |
  +----------+----------+--------------+--------------+--------------+--------------------------+-------------------+
  | 8x256MB  |  32 bit  |   32 bit     |      64M     |     1M       | 63 (base rows take 63M)  |         1M        |
  +----------+----------+--------------+--------------+--------------+--------------------------+-------------------+
  | 8x256MB  | 128 bit  |   128 bit    |      16M     |     1M       | 15 (base rows take 15M)  |         1M        |
  +----------+----------+--------------+--------------+--------------+--------------------------+-------------------+

The Number of hash entry is limited by the number of URAM in a single SLR. For example, there are 320 URAMs in a SLR of the AMD Alveo™ U280, and the 1M hash entry will take 192 URAMs (96 URAMs for the base hash counter + 96 URAMs for the overflow hash counter). Because the number of hash entry must be the power of two, 1M hash entry is the maximum for the Alveo U280 to avoid crossing the SLR logic which will lead to bad timing performance of the design.