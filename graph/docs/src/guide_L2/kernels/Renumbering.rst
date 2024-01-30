.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.


*************************************************
Internal Design of Renumber 
*************************************************


Overview
========
The renumbering recode the categorized graph's table and it supports 64M data for input. The output is a number from 0 to some value, the size of the new graph can be
calculated based on the value.

Implementation
==============
The implementation is shown in the following figure:

.. image:: /images/renumbering.png
   :alt: renumber design
   :width: 80%
   :align: center

The kernel does the following steps:

1. Set Uram: Load the original cids of the graph and scan vertices to set URAM. If vertex's cid appears first, the flag on URAM is written true. Otherwise, the flag is written false.

2. Lookup HBM: Lookup HBM to get new cid that have been written success, put it into stream. If the cid hasn't written success, the cid is put as a waiting buffer. The buffer is a first-in first-out circular cache and read it regularly.

3. Updated HBM: Scan stream to get new cid and write back to HBM.

Interface
=========
The input should be a categorized graph's table by clustering algorithm such as louvain.

The result is a renumbered table which shows the number of vertices. The order of the result is the same as the order of input pairs.

