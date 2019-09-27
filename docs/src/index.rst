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

======================
Vitis Database Library
======================

Vitis Database Library is an open-sourced Vitis library written in C++ for
accelerating database applications in a variety of use cases.
It now covers two levels of acceleration: the module level and the pre-defined kernel level,
and will evolve to offer the third level as pure software APIs working with pre-defined hardware overlays.

* At module level, it provides an optimized hardware implementation of most common relational database execution plan steps,
  like hash-join and aggregation.
* In kernel level, the post-bitstream-programmable kernel can be used to map a sequence of execution plan steps,
  without having to compile FPGA binaries for each query.
* The upcoming software API level will wrap the details of offloading acceleration with prebuilt binary (overlay)
  and allow users to accelerate supported database tasks on Alveo cards without hardware development.

Since all the kernel code is developed in HLS C++ with the permissive Apache 2.0 license,
advanced users can easily tailor, optimize or combine with property logic at any levels.
Demo/examples of different database acceleration approach are also provided with the library for easy on-boarding.


.. toctree::
   :caption: Library Overview
   :maxdepth: 1

   overview.rst
   release.rst

.. toctree::
   :caption: User Guide
   :maxdepth: 2

   usecase.rst
   guide/L1.rst
   gqe_guide/L2.rst
   gqe_guide/L3.rst

.. toctree::
   :caption: Benchmark Result
   :maxdepth: 1

   benchmark/tpc_h.rst


Library API Summary
-------------------

L1 APIs
~~~~~~~

+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| Library API             | Description                                                                                                                   |
+=========================+===============================================================================================================================+
| aggregate               | A group of overloaded aggregate functions, supports SUM, MAX, MIN, MEAN, VARIANCE, COUNT, COUNTNONZERO operation.             |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| bitonicSort             | Bitonic sort is a parallel algorithm for sorting.                                                                             |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| bfGen                   | Generate the bloom-filter in on-chip RAM blocks.                                                                              |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| bfGenStream             | Generate the bloom-filter in on-chip RAM blocks, and emit the vectors through FIFO upon finish.                               |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| bfCheck                 | Check existence of a value using bloom-filter vectors.                                                                        |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| combineCol              | A group of overloaded functions for combining two to five columns into one.                                                   |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| splitCol                | A group of overloaded functions for splitting previously combined column into two to five separate ones.                      |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| directGroupAggregate    | Group-by aggregation with limited key width.                                                                                  |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| duplicateCol            | Duplicate one column into two columns.                                                                                        |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| dynamicEval             | Dynamic expression evaluation.                                                                                                |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| dynamicFilter           | A group overloaded functions for filtering payloads according to one to four conditions columns pro gamed at during run-time. |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| groupAggregate          | A series of overloaded functions for group-aggregation. Input rows are required to be sorted by grouping key(s).              |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| hashAntiJoin            | Hash-Anti-Join primitive.                                                                                                     |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| hashGroupAggregate      | Generic hash group aggregate primitive.                                                                                       |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| hashJoinMPU             | Hash-Join primitive, using multiple DDR/HBM buffers.                                                                          |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| hashJoinV3              | Hash-Join v3 primitive, it is designed for HBM device and performs better in large size of table.                             |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| hashBuildProbeV3        | Hash-Build-Probe v3 primitive, it can perform hash build and hash probe separately.                                           |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| hashJoinV4              | Hash-Join v4 primitive, using bloom-filter to enhance performance of hash join, designed for HBM device.                      |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| hashBuildProbeV4        | Hash-Build-Probe v4 primitive, build and probe are separately performed. This primitive is designed for HBM device only.      |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| hashLookup3             | Lookup3 algorithm generates 64-bit or 32-bit hash.                                                                            |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| hashMultiJoin           | Hash-Multi-Join primitive is based on hashJoinV3, and can be programmed at run-time to perform inner, anti or left join.      |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| hashMurmur3             | Murmur3 hash algorithm.                                                                                                       |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| hashPartition           | Hash-Partition primitive splits a table into partitions of rows based on hash of a selected key.                              |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| hashSemiJoin            | Hash-Semi-Join primitive is based on hashJoinMPU, but performs semi-join.                                                     |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| insertSort              | Insert sort algorithm on chip.                                                                                                |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| mergeJoin               | Merge join algorithm for sorted tables without duplicated keys in the left table.                                             |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| mergeLeftJoin           | Merge left join function for sorted tables, the left table should not have duplicated keys.                                   |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| mergeSort               | Merge sort algorithm.                                                                                                         |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| nestedLoopJoin          | Nested loop join.                                                                                                             |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| scanCmpStrCol           | Scan multiple string columns in global memory, and compare each of them with a constant string                                |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| scanCol                 | A group of overloaded functions for Scanning 1 to 6 columns as a table from DDR/HBM buffers.                                  |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+
| staticEval              | A group of overloaded functions for evaluating a compile-time selected expression on each row with one to four columns.       |
+-------------------------+-------------------------------------------------------------------------------------------------------------------------------+

L2 APIs
~~~~~~~

+---------+----------------------+
| gqeAggr | GQE aggregate kernel |
+---------+----------------------+
| gqeJoin | GQE join kernel      |
+---------+----------------------+
| gqePart | GQE partition kernel |
+---------+----------------------+

