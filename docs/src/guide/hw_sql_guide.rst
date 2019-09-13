.. 
   Copyright 2019 Xilinx, Inc.
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

.. _hw_sql_guide:

***************************************************
Primitive Overview
***************************************************

The Algorithm Library provides a range of primitives for implementing SQL queries in C++ for Vitis.
Headers for these hardware APIs can be found in ``include`` directory of the package.

.. toctree::
   :maxdepth: 1

Stream-based Interface
----------------------

The interface of primitives in this library are mostly HLS streams, with a single bit stream
along with the main data stream throughout the dataflow.

.. code:: cpp

        hls::stream<ap_uint<W> >& data_strm,
        hls::stream<bool>&        e_data_strm,

The benefits of this interface are

* Within a HLS dataflow region, all primitives connected via HLS streams can work in
  parallel, and this is the key to FPGA acceleration.

* Using the single bit stream to mark *end of operation* can trigger stream consumer
  as soon as the first row data becomes available, without known how many rows will be
  generated later. Moreover, it can represent empty table.

Some primitives work with separate columns, so their interface are naturally design
to receive multiple data streams, one for each column. As these columns are from the
same table and always pass through same number of items, they would share one
end signal stream in the interface.

Other primitives' semantic may treat multiple-columns as one concept,
For example, Hash-Join only cares about key and payload.
For such primitives, :ref:`cid-xf::database::combineCol` and
:ref:`cid-xf::database::splitCol` helpers can be used to combine or split columns.
For more details on helper utilities, see :ref:`guide-glue`.

Implementing Join
-----------------

Join Implementation Summary
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Join primitives are summaries in the following table:

:Hash-Join:        It performs inner join, but with a multi-process-unit design,
                   and stores only hash-table on chip. The small table is cached in
                   DDR/HBM banks during execution.

:Merge-Join:       It performs inner join with two tables both pre-sorted with the join key.

:Merge-Left-Join:  It performs left join with two tables both pre-sorted with the join key.

:Nested-Loop:      On FPGA, hash-based joins are typically perferred with unique keys,
                   and nested loop is often used to join table with duplications with
                   another table.

Hash-Join
~~~~~~~~~

The hash-join primitive performs innner-join. When the key is unique, use the default 8M version,
which can tolerate hash-collision up to 512 times.
When the key is not unique, the chance in which different rows falls into the same hash entry
is significantly boosted, and thus the 2M version should be used. It allows up to 2^18 rows
to be hashed in the same hash-table slot, including both hash collision and key duplication.

For internal structure and execution flow, see :ref:`guide-hash_join_mpu`.

Merge-Join and Merge-Left-Join
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The merge-join performs inner-join of two tables. The merge-left-join performs left join of two tables. The limitation includes:

        - Left table should not contatin duplicated keys.
        - Both input tables should be sorted.

See :ref:`guide-merge_join`.

Nested-Loop
~~~~~~~~~~~

See :ref:`guide-nested_loop_join`.



Implementing Filter
-------------------

.. tip::
   Filter works with boolean conditions, so to build an expression based filter, the :ref:`guide-dynamic-eval`
   primitive can be used to implementing the evaluation part. With evaluated expression value pipelined to
   filter, the payload can be conditionally kept or dropped.

This primitive allow the filter condition to be specified via some config bits at run time,
so that the xclbin does not need to be recompiled to implement a different filter logic.

See :ref:`guide-dynamic_filter`.



Implementing Evaluation
-----------------------

The Dynamic Evaluation primitive can calculate different expressions using data columns,
based on post-bitstream configurations set during run-time.

See :ref:`guide-dynamic-eval`.



Implementing Group-by Aggregation
---------------------------------

Group Aggregate Sorted Rows
~~~~~~~~~~~~~~~~~~~~~~~~~~~

This library provides group-aggregate operator for sorted data. It assumes the input has already
been sorted by group-keys, and simply emit aggregation result for continuous same group-key.

See :ref:`guide-group-aggregate`.

On-Chip Group-Aggregate
~~~~~~~~~~~~~~~~~~~~~~~

When the group key is of limited width, it can be used directly as address of on-chip storage in which
group aggregation can be implemented. This scenario is describe as a "direct group aggregate".
Although the retirement on group key limits its use case, this algorithm is light on LUTs,
and the URAM usage could be optimized to fit the expected key width.

See :ref:`guide-direct-aggregate`


Off-chip Hash-Aggregate
~~~~~~~~~~~~~~~~~~~~~~~

This primitive performs multi-round Hash-Aggregate, flushing the overflow entries to
DDR temporally. Thus it enforces no limitation on the number of unique keys to be grouped.

See :ref:`guide-hash_aggr_general`



Implementing Sort
-----------------

Sort Implementation Summary
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Sort primitives are summaries in the following table:

:Bitonic-Sort:    It performs sort by bitonic sort algorithm.

:Insert-Sort:    It performs sort by insert sort algorithm.

:Merge-Sort:    It merge two sorted stream into one sorted stream.


Bitonic-Sort
~~~~~~~~~~~~

This algorithm can offer very high-throughput, but is not resource-efficient when the I/O rate is limited.

See :ref:`guide-bitonic_sort`

Insert-Sort
~~~~~~~~~~~

This algorithm fits the scenarios where I/O rate is low, and uses the FPGA resources efficiently.
However, the resource is linear to the max number of elements allowed to be sorted once,
so to scale to large input, it should be used together with merge-sort.

See :ref:`guide-insert_sort`

Merge-Sort
~~~~~~~~~~

This algorithm simply zips two *sorted* streams into one sorted stream.

See :ref:`guide-merge_sort`



.. _guide-glue:

Glue Logic
----------

Combine and Split Columns
~~~~~~~~~~~~~~~~~~~~~~~~~

See :ref:`guide-combine-split-unit`.

