.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis Database Library, primitive, stream-based, scan, hash, filter, join, Hash-Join, Hash-Semi-Join, Hash-Anti-Join, Hash-Multi-Join, Merge-Join, Merge-Left-Join, Nested-Loop-Join, Group-Aggregate, Off-chip, Bitonic-Sort, Insert-Sort, Merge-Sort, Glue Logic
   :description: The algorithm library provides a range of primitives for implementing SQL queries in C++ for Vitis.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _hw_sql_guide:

***************************************************
Primitive Overview
***************************************************

The algorithm library provides a range of primitives for implementing SQL queries in C++ for AMD Vitis™. Headers for these hardware APIs can be found in the ``include`` directory of the package.

.. toctree::
   :maxdepth: 1

1. Stream-based Interface
-------------------------

The interfaces of primitives in this library are mostly HLS streams, with a single-bit stream along with the main data stream throughout the dataflow.

.. code:: cpp

        hls::stream<ap_uint<W> >& data_strm,
        hls::stream<bool>&        e_data_strm,

The benefits of this interface are:

* Within a HLS dataflow region, all primitives connected via HLS streams can work in parallel, and this is the key to FPGA acceleration.

* Using the single-bit stream to mark *end of operation* can trigger the stream consumer as soon as the first row data becomes available, without knowing how many rows will be generated later. Moreover, it can represent an empty table.

Some primitives work with separated columns, so their interfaces are naturally designed to receive multiple data streams, one for each column. As these columns are from the same table and always pass through the same number of items, they would share one end signal stream in the interface.

Other primitives' semantic might treat multiple-columns as one concept. For example, Hash-Join only cares about the key and the payload. For such primitives, :ref:`cid-xf::database::combineCol` and
:ref:`cid-xf::database::splitCol` helpers can be used to combine or split columns. For more details on helper utilities, see :ref:`guide-glue`.

2. Implementing Scan
--------------------

The Scan primitive is a function to transfer data from HBM/double-data rate (DDR) to the internal FPGA logic. A set of ``scanCol`` function is provided to support different purposes.

See :ref:`guide-scan`.

3. Implementing Hash
--------------------

The Lookup3 and Murmur3 hash functions are implemented now, and some specifications of different APIs are provided in the guide.

See :ref:`guide-hash`.

4. Implementing Filter
----------------------

.. tip::
   The filter works with boolean conditions, so to build an expression-based filter, the :ref:`guide-dynamic_eval` primitive can be used to implement the evaluation part. With the evaluated expression value pipelined to the filter, the payload can be conditionally kept or dropped.

This primitive allows the filter condition to be specified via some configuration bits at runtime, so that the xclbin does not need to be recompiled to implement a different filter logic.

See :ref:`guide-dynamic_filter`.

5. Implementing Evaluation
--------------------------

The Dynamic Evaluation primitive can calculate different expressions using data columns, based on post-bitstream configurations set during runtime.

See :ref:`guide-dynamic_eval`.

6. Implementing Bloom Filter
----------------------------

The Bloom Filter primitive is a block RAM/URAM based module to achieve high performance in its functionality. It can filter redundant data before Join processing and also saves time because of reducing unnecessary memory accessing.

See :ref:`guide-bloom_filter`.

7. Implementing Join
--------------------

7-1. Join Implementation Summary
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Join primitives are summarized in the following table:

+------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Hash-Join-MPU    | It performs hash join with a multi-process-unit design, and stores only the hash table in URAM. The small table is cached in DDR/HBM banks during execution.   |
+------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Hash-Join-v3     | The storage of the small table is optimized and provides an efficient way to handle hash overflow. Overall, it has better performance than Hash-Join-v2.       |
+------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Hash-Join-v4     | Implements a bloom filter in hash join to further improve the efficiency in accessing the small table which is cached in DDR/HBM.                              |
+------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Hash-Semi-Join   | It performs hash semi join based on the framework of ``Hash-Join-MPU``, in which the Join function is replaced by Semi-Join function.                          |
+------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Hash-Anti-Join   | It performs hash anti join based on the framework of ``Hash-Join-v3``, in which the Probe and Join function is redesigned to realize the logic of Anti-Join.   |
+------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Hash-Multi-Join  | It can use a parameter as control flag to perform hash join, hash semi-join, and hash anti-join. It also shares the most of the logic in ``Hash-Join-v3``.     |
+------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Merge-Join       | It performs inner join with two tables both presorted with the join key.                                                                                       |
+------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Merge-Left-Join  | It performs left join with two tables both presorted with the join key.                                                                                        |
+------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Nested-Loop-Join | On the FPGA, hash-based joins are typically preferred with unique keys, and a nested loop is often used to join a table with duplications with another table.  |
+------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+

7-2. Hash-Join
~~~~~~~~~~~~~~

The Hash-Join primitive has three versions now. When the requirement for performance is not strict, or the resource is limited, use the default ``Hash-Join-MPU`` version, which can process 8M distinct key or 2M input with key duplication. It can tolerate hash-collision up to 512 times. When the key is unique, different rows falling into the same hash entry is significantly boosted, and it allows up to 2^18 rows to be hashed in the same hash-table slot, including both hash collision and key duplication.

The ``Hash-Join-v3`` primitive can handle hash overflow and even the size of overflow is relatively large. It separates the storage of overflow rows from normal rows and takes twice number of DDR/HBM ports than ``Hash-Join-MPU``. Also, it ensures a better performance including larger size of a small table, higher throughput, and more compatible for possibly large overflow.

The ``Hash-Join-v4`` primitive implements a built-in bloom filter to reduce the redundant memory access. Bloom filter provides at least 64M 1-bit hash entries on URAM of a single SLR in AMD Alveo|trade| platforms, 
which is 16-32x larger than the hash index. So the combination of the bloom filter and the hash index is a better solution to improve the performance of hash join. 

For internal structure and execution flow, see:

        - :ref:`guide-hash_join_mpu`
        - :ref:`guide-hash_join_v3`
        - :ref:`guide-hash_join_v4`

7-3. Hash-Semi-Join
~~~~~~~~~~~~~~~~~~~

The Hash-Semi-Join primitive is based on the framework of ``Hash-Join-MPU``. The Join function is replaced by Semi-Join function which returns only the first row of the small table while processing Probing.

See :ref:`guide-hash_semi_join`.

7-4. Hash-Anti-Join
~~~~~~~~~~~~~~~~~~~

The Hash-Anti-Join returns rows from the first table which do not exist in the second table. It is implemented based on the ``Hash-Join-v3``.

See :ref:`guide-hash_anti_join`.

7-5. Hash-Multi-Join
~~~~~~~~~~~~~~~~~~~~

The Hash-Multi-Join primitive is a combined set of Hash-Join, Hash-Semi-Join, and Hash-Anti-Join. A control flag is used to determine which kind of Join to perform. The three Join algorithms share the most part of control logic and first in first out (FIFO) resource, so that the resource consumption is acceptable and well suited in a SLR.

See :ref:`guide-hash_multi_join`.

7-6. Merge-Join and Merge-Left-Join
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The merge-join performs inner-join of two tables, and the merge-left-join performs left join of two tables. The limitation includes:

        - The left table should not contain duplicated keys.
        - Both input tables should be sorted.

See :ref:`guide-merge_join`.

7-7. Nested-Loop-Join
~~~~~~~~~~~~~~~~~~~~~

The Nested Loop Join primitive performs a double loop to compare all keys in both small and big tables. To balance performance and resource consumptions in the FPGA, the size of the small table is limited to 120 for a single call of the primitive.

See :ref:`guide-nested_loop_join`.

8. Implementing Group-by Aggregation
------------------------------------

8-1. Sorted Rows Group-Aggregate
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This library provides a group-aggregate operator for sorted data. It assumes the input has already been sorted by group-keys and simply emits the aggregation result for the continuous same group key.

See :ref:`guide-group_aggregate`.

8-2. On-Chip Group-Aggregate
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When the group key is of limited width, it can be used directly as an address for on-chip storage in which the group aggregation can be implemented. This scenario is described as a "direct group aggregate". Although the retirement on group key limits its use case, this algorithm is light on LUTs, and the URAM usage could be optimized to fit the expected key width.

See :ref:`guide-direct_aggregate`

8-3. Off-chip Group-Aggregate
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This primitive performs multi-round Hash-Aggregate, flushing the overflow entries to the HBM/DDR temporarily. Thus, it enforces no limitation on the number of unique keys to be grouped.

See :ref:`guide-hash_aggr_general`

9. Implementing Hash Partition
------------------------------

The Hash Partition primitive can distribute a giant table into multiple sub-tables based on the most significant bits (MSBs) hash value of the partition key. It is also a multi-channel and multi-PU design to achieve high performance.

See :ref:`guide-hash_partition`.

10. Implementing Sort
---------------------

10-1. Sort Implementation Summary
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Sort primitives are summarized as follows:

:Bitonic-Sort: It performs sort by a bitonic sort algorithm.

:Insert-Sort: It performs sort by an insert sort algorithm.

:Merge-Sort: It merges two sorted streams into one sorted stream.

10-2. Bitonic-Sort
~~~~~~~~~~~~~~~~~~

This algorithm can provide very high throughput, but it is not resource-efficient when setting a large sort number.

See :ref:`guide-bitonic_sort`

10-3. Insert-Sort
~~~~~~~~~~~~~~~~~

This algorithm fits the scenarios where the I/O rate is low and uses the FPGA resources efficiently. However, the resource is linear to the max number of elements allowed to be sorted once, so to scale to large input, it should be used together with merge-sort.

See :ref:`guide-insert_sort`

10-4. Merge-Sort
~~~~~~~~~~~~~~~~

This algorithm simply merges two sorted streams into one sorted stream.

See :ref:`guide-merge_sort`

.. _guide-glue:

11. Glue Logic
--------------

11-1. Combine and Split Columns
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

See :ref:`guide-combine_split_unit`.