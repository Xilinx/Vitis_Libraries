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


*************************************************
Internal Design of Renumber 
*************************************************


Overview
========
The renumbering recode the categorized graph's table and it supports 64M data for input. The output is a number from 0 to some value, the size of new graph can  
calculate based the value.

Implementation
==============
The implemention is shown in the figure below:

.. image:: /images/renumbering.png
   :alt: renumber design
   :width: 80%
   :align: center

The kernel will do the following steps:

1. Set Uram: Load the original cids of the graph and scan vertices to set URAM. If vertex's cid is first appear, the flag on URAM will be writen true，otherwise the flag will be writen false.

2. Lookup HBM: Lookup HBM to get new cid that have been writen success, put it into stream. If the cid haven't writen success, the cid will be put a waiting buffer. The buffer is a first-in first-out circular cache and read it regularly.

3. Updated HBM: Scan stream to get new cid and write back to HBM.

Interface
=========
The input should be a categorized graph's table by clustering algorithm such as louvain.

The result is a renumbered table which shows the number of vertices. The order of the result is the same as the order of input pairs.

