.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.


*************************************************
Internal Design of Merge 
*************************************************


Overview
========
In graph algorithm, sometimes you need to merge a large graph to a small graph based on specific sequence.

Implementation
==============
The implementation is shown in the following figure:

.. image:: /images/merging_design.png
   :alt: merge design
   :width: 70%
   :align: center
   
The design used several cascade hash aggregations. Due to the power law distribution of the graph, most of the vertex degree is small. So first, use a 64 depth LUT ram to do II=1 hash aggregation. If the number overflow is 64, the data streaming to 5 hash aggregation URAM module, which can do hash aggregation with II = 1. The 5 hash aggregation URAM module including 4 32K URAMs and one 4K URAM to further reduce the number of hash collision to the final stage. In the final stage, the hash collision should be small, so the last stage hash aggregation performance is poor, II>3. After all seven aggregation stages are done, the output module collects the result of each stage and output to HBMs.

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
    * If with the current Louvain, the URAM usage may overflow, so you can reuse the URAM for main louvain and merge.
    * If you can also use Hash cascade for main louvain.
    * The interfaces for merge is not WIDEBUS. If the merge is with main louvain, enable WIDEBUS for some input and output interfaces.
