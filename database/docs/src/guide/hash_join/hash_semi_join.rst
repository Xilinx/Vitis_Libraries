.. Copyright © YEAR–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Hash-Semi-Join, Hash-Join, hashSemiJoin, Hash-Join-MPU
   :description: Describes the structure and execution of Hash-Semi-Join.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _guide-hash_semi_join:

*************************************************************
Internals of Hash-Semi-Join (Multi-Process-Unit Version)
*************************************************************

.. toctree::
   :hidden:
   :maxdepth: 2

This document describes the structure and execution of Hash-Semi-Join, implemented as a :ref:`hashSemiJoin<cid-xf::database::hashSemiJoin>` function. Its implementation is based on :ref:`Hash-Join-MPU <guide-hash_join_mpu>`.

Hash-Semi-Join returns rows from the outer table where a field of the outer table matches with the one of the inner table. Even if a record of the outer table matches with many rows in the inner table, the one of outer table is output only once.

.. image:: /images/hash_semi_join_operation.png
   :alt: the operation of hash semi join
   :align: center

Hash-Semi-Join is written using the EXISTS or IN constructs. For example:
     select * from S where S.key in ( select key from T )

---------
Principle
---------

There are two stages in Hash-Semi-Join:
   
1. build: The inner table is used for the hash table for rapid searching matching rows. 
   
2. probe: The outer table is used for the probe table. Each record of the probe table is applied the same hash function on the joining column and will be hit the corresponding entry in the hash table. If a record of probe table first matches with a row in the hash table, it will be output and never output again even if it matches again.
 
.. image:: /images/hash_semi_join_principle.png
   :alt: Hash Semi Join princilpe
   :align: center

---------
Structure
---------

The structure of Hash-Semi-Join is same as that of :ref:`Hash-Join-MPU <guide-hash_join_mpu>`.

.. image:: /images/hash_semi_join_structure.png
   :alt: Hash Semi Join MPU Structure
   :align: center

The Hash-Semi-Join primitive has a multi-PU design internally to utilize the advantage of high memory bandwidth in AMD FPGAs. Workload is distributed based on the most signficant bits (MSBs) of the hash value of join key to the PU, so that each PU can work independently. The current design use eight PUs as the default and is served by four input channels. The input of key and payload can be processed as a pair in each cycle.

There are several kinds of modules in the design, and the detailed functionality of each module is described as follows:

1. scan: Outer table are input first and converted to the stream twice continually, then the inner table are input and converted to the stream. Key and payload are pre-stored in the DDR/HBM of the FPGA which can be scanned as input streams here.

2. Dispatcher: All records either in the outer or inner table are input by one or more (only 1 , 2, or 4 is supported) channels after scan. The dispatcher computes each key's hash value and chooses the MSBs for dispatching, so that the input stream with same hash value are processed by the same PU. Therefore, the input stream will be divided into multiple PUs (only 1,2,4, or 8 is supported). 

3. Switcher: Switcher merges the multi-channel output of dispatchers into one channel and distributes them to each PU according the MSB value of hash. 

4. Bitmap: Bitmap counts hash collisions and builds a bitmap (bit_vector).

5. Build: The rows of key and payload in the same PU are mapped to the bitmap and stored in a buffer. 

6. Probe: The keys in inner table are matched with that in the outer table.   

7. Collecter: Merges each PU's output into one output stream.

The work stages of these modules are show in the following table:

+-------------------+------------+----------+--------+-------+-------+-----------+
|      Input        | Dispatcher | Switcher | Bitmap | Build | Probe | Collecter |
+-------------------+------------+----------+--------+-------+-------+-----------+
| small/inner table |   work     |   work   |  work  |   -   |   -   |    -      |
+-------------------+------------+----------+--------+-------+-------+-----------+
| small/inner table |   work     |   work   |   -    | work  |   -   |    -      |
+-------------------+------------+----------+--------+-------+-------+-----------+
|  big/outer table  |   work     |   work   |   -    |   -   | work  |   work    |
+-------------------+------------+----------+--------+-------+-------+-----------+

The default number of the PU is set to 8, as each PU requires a dedicated bank to avoid conflicts, and because of the DDR/HBM memory access delay, four channels can serve enough data to these PUs. Each PU performs Hash-Semi-Join in three phases.

1. build bitmap: With the inner table as input, the number of keys falls into each hash values are counted. The number of counts are stored in a bit vector in URAM. After a full scan of the inner table, the bit vector is walked once, accumulating the counts to offset of each hash.

2. build unit: The inner table is read in again and stored into DDR/HBM buffers of that PU. By referencing the bit vector in URAM created in the previous phase, the kernel knows where to find empty slots for each key, and once a inner table payload and the key is written into the DDR/URAM, the offset in the bit vector is increased by 1, so that the next key of same hash value can be written into a different place. As the number of keys with each hash have been counted, such an offset increase will not step into another key's slot.

3. probe unit: Finally, the outer table is read inm, and again by referencing the bit vector with hash of key, you can know the offset of this hash and the number of keys with the same hash. Then the possible matched key and payload pairs can be retrieved from the DDR/HBM and joined with the outer table payload after key comparison.

.. IMPORTANT::
   To reduce the storage size of hash-table on a FPGA board, the inner table has to be scanned in TWICE and followed by the outer table ONCE.

.. CAUTION::
   Currently, this primitive expects a unique key in the inner table.

This ``hashSemiJoinMPU`` primitive only has one port for key input and one port for payload input. If your tables are joined by multiple key columns or has multiple columns as payload, use :ref:`combineCol <cid-xf::database::combineCol>` to merge the column streams, and use :ref:`splitCol <cid-xf::database::splitCol>` to split the output to columns.

There are two versions of this primitive currently, with different number of slots or hash collision and key duplication. The version with more slots per hash entry has less total row capacity, summarized as follows:

  +--------------+----------------+
  | Row Capacity | Hash Slots     |
  +--------------+----------------+
  | 2M           | 262144 (0.25M) |
  +--------------+----------------+