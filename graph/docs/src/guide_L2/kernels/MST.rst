.. 
   .. Copyright © 2021–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.


*************************************************
Internal Design of Minimum Spanning Tree 
*************************************************


Overview
========
Minimum Spanning Tree is the problem of finding a set of edges that can connect all the vertices in a undirected graph with the minimal sum of edge weights.

Algorithm
=========
The implemented Minimum Spanning Tree algorithm is based on the Prim algorithm. The description of the algorithm is as follows:

Given an undirected graph, and a source vertex, print out a MST for the connected subgraph, which contains the source vertex.

1. Start with a set MSTNodes = {source vertex}

2. For all vertices in MSTNodes, find another vertex y in the graph but not MSTNodes, which is connected to a vertex x in MSTNodes such that the weight on the edge e(x,y) is the smallest among all such edges from a vertex in MSTNodes to a vertex not in MSTNodes. Add y to MSTNodes, and add the edge (x,y) to MST.

3. Repeat step2 until MSTNodes has an edge linking to any other vertex in the graph.

Interface
=========
The input should be an undirected graph in compressed sparse row (CSR) format.
The result is an array, which shows the predecessor of a vertex in the generated tree. The vertex ID can be used to index the result array.

Implementation
==============
The algorithm implementation is shown in the following figure:

.. image:: /images/mst_design.png
   :alt: block design of minimum spanning tree
   :width: 80%
   :align: center

There are four functional blocks as shown in the figure:

1. QueuePop is responsible to load the next vertex in the priority queue and pass it to the loadOffset.

2. loadOffset loads the offset value associate with current vertex from the CSR offset values and pass it to the next block.

3. loadCol&Wei loads the ID and weight of the next hop vertices according to the offset values. It passes these IDs and weights to the loadRes.

4. Queue pushes the next hop vertices into the priority queue. It sorts the priority again.

This system starts from pushing the source vertex into the queue and iterate until the queue is empty.

Resource
=========
The hardware resource utilizations are listed in the following figure.

.. image:: /images/mst_resource.png
   :alt: Resource utilization of minimum spanning tree
   :width: 70%
   :align: center

