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
Internal Design of Merge 
*************************************************


Overview
========
In graph algorithm, sometimes we need to merge a large graph to a small graph based on specific sequence.

Implementation
==============
The implemention is shown in the figure below:

.. image:: /images/merging_design.png
   :alt: merge design
   :width: 70%
   :align: center
   
The design used several cascade hash aggregation. Because the power law distribution of the graph. Most of the vertex degree is small, so first we use a 64 depth LUT ram to do II=1 hash aggregation. It the number overflow 64, the data streaming to 5 hash aggregation URAM module which can do hash aggregation with II = 1. The 5 hash aggregation URAM module including 4 32K URAMs and one 4K uram to further reduce the number of hash collision to the final stage. In the final stage, the hash collision should be very small, so the last stage hash aggregation performance is very poor, II>3. After all 7 aggregation stages done, the output module will collect the result of each stage and output to HBMs.

Algorithm
===============

* Step1: Compute the offset/edge/weight access sequence

  * 1.1 Count the number and index for the same C (random update count_c_single).
  * 1.2 Aggregate count_c_single to get the total offset of each cid (burst read and write).
  * 1.3 Compute final jump sequence to access input offset (random read and write).

.. image:: /images/merging_algor.png 
   :alt: merge algorithm  
   :width: 70%
   :align: center

* Step2: Hash aggregation based on the previous access sequence
  The pseudo-code is shown below:

.. code::
  
    foreach c in count_c //scan count_c to get each cid number
        for v from prev_c → current_c // scan each input vertex with the same cid
            for e in v // scan each edge in vertex
                Hash aggregation for each single C
        output result for each single C

Conclusion
==========
1. When the vertex is small, the CPU is faster, so the speed up is small; when vertex is large, the speed up is larger.
2. When the average degree is large, the speed up is larger.
3. Issues of Merge with louvain:
    * Current Merge only support maximum 128K degree.
    * If with current Louvain, the URAM usage may overflow, so we can reuse the URAM for main louvain and merge.
    * If we can also use Hash cascade for main louvain.
    * The interfaces for merge is not WIDEBUS, if merge with main louvain, need enable WIDEBUS for some input and output interfaces.
