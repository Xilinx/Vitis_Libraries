.. 
   Copyright 2022 Xilinx, Inc.
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.


**************
STRTree Engine
**************

Overview
========

STRTree (Sort-Tile-Recursive Tree) Engine is an GeoSpatial index engine that uses bottom-up way to build an R tree for two-dimensional points. The STR tree packed R tree is simple to implement and maximizes space utilization; that is, as many leaves as possible are filled to capacity. Overlap between nodes is far less than in a basic R-tree. However, once the tree has been built, points cannot be added or removed.

Algorithm
=========

There are n nodes (rectangles), and the center point of its i-th node is represented as (xi, yi). A parent node has at most r child nodes.

(1) Sort all nodes according to x.

.. _my-figure1:
.. figure:: /images/strtree_sort_by_x_and_split.png
    :alt: Figure 1 STRTree sort by x
    :width: 80%
    :align: center

(2) Divide all nodes into sqrt (n/r) parts, each of which is sorted by y.

.. _my-figure2:
.. figure:: /images/strtree_sort_by_y_and_split.png
    :alt: Figure 2 STRTree sort by y
    :width: 80%
    :align: center

(3) Every r nodes are merged into a new parent node.
(4) Repeat (1)~(3) until the number of parent nodes is 1.


Implementation
==============

STRTree Kernel is implemented according to the algorithm flow. Its core design is the sorting of a dataset (x, y, id). In order to realize the sorting of data (size>16M+), two sorting modules are provided here: `blockSort` and `mergeTreeSort`.

blockSort
---------

For input data (size = N), it divides the data into M blocks, sorts each block, and obtains M ordered blocks. The size of N depends on the capacity of the DDR, and the size of M depends on the in-chip LUT and URAM resources. Its design is show in the figure below:

.. _my-figure3:
.. figure:: /images/strtree_block_sort.png
    :alt: Figure 3 STRTree block sort
    :width: 80%
    :align: center


mergeTreeSort
-------------

For K ordered blocks as input, it can get all data ordered output. The size of K cannot affect the frequency of the kernel, and it can also ensure that each cycle outputs a data. ts design is show in the figure below:

.. _my-figure4:
.. figure:: /images/strtree_merge_tree_sort.png
    :alt: Figure 4 STRTree merge tree sort
    :width: 80%
    :align: center

Resource Utilization
====================
 
The Kernel is validated on Alveo U200 card.
The hardware resources utilization are listed in the table above (not include Platform).

+------------------+---------+---------+--------+--------+-------+----------+
|       Name       |   LUT   |   REG   |  BRAM  | URAM   | DSP   |   Freq   |
+------------------+---------+---------+--------+--------+-------+----------+
|                  | 148,278 | 159,273 |   30   |  324   |  22   |          |
|  STRTree_Kernel  +---------+---------+--------+--------+-------+  183 MHz |
|   (MSN=2^24)     |  12.54% |  8.36%  | 1.39%  | 40.50% | 0.32% |          |
+------------------+---------+---------+--------+--------+-------+----------+

.. toctree::
   :maxdepth: 1
