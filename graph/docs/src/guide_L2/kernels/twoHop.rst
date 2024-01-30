.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.


*************************************************
Internal Design of two hop path count 
*************************************************


Overview
========
The two hop path counts calculate how many hop paths are there between a given set of source and the destination pairs.

Implementation
==============
The implementation is shown in the following figure:

.. image:: /images/twoHop_design.png
   :alt: two hop path count design
   :width: 80%
   :align: center

The kernel does the following steps:

1. Load the ID of the source and destination vertices. Pass the source vertices ID to the first hop path finder through hls::stream. Pass the destination vertices to the filter and counter.

2. The first hop path finder finds all the first hop vertices and pass them to the 2nd hop path finder through hls::stream.

3. The second hop path finder finds all the second hop vertices and pass them to the final filter and counter through hls::stream.

4. The filter and finder will drop all the second hop vertices that is not the target destination vertex and count all the second hop vertices that is the targeted destination vertex. And then stored it into the result. 

Interface
=========
The input should be a directed graph in compressed sparse row (CSR) format.

The result is an array, which shows the number of two hop paths. The order of the result is the same as the order of input pairs.

