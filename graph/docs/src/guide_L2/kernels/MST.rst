.. 
   Copyright 2021 Xilinx, Inc.
  
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
Internal Design of Minimum Spanning Tree 
*************************************************


Overview
========
Minimum Spanning Tree is the problem of finding a set of edges that can connect all the vertices in a undirected graph with the minumal sum of edge weights.

Algorithm
=========
The implemented Minimum Spanning Tree algorithm is based on the Prim algorithm. The description of the algorithm is as below:

Given an undirected graph, and a source vertex, print out a MST for the connected subgraph which contains the source vertex.

1. Start with a set MSTNodes = {source vertex}

2. For all vertices in MSTNodes, find another vertex y in the graph but not MSTNodes which is connected to a vertex x in MSTNodes such that the weight on the edge e(x,y) is the smallest among all such edges from a vertex in MSTNodes to a vertex not in MSTNodes. Add y to MSTNodes, and add the edge (x,y) to MST

3. Repeat 2 until MSTNodes has no edge linking to any other vertex in the graph.

Interface
=========
The input should be an undirected graph in compressed sparse row (CSR) format.
The result is an array which shows the predecessor of a vertex in the generated tree. The vertex ID can be used to index the result array.

Implementation
==============
The algorithm implemention is shown in the figure below:

.. image:: /images/mst_design.png
   :alt: block design of minimum spanning tree
   :width: 80%
   :align: center

There are 4 functional blocks as shown in the figure:

1. QueuePop is responsible to load the next vertex in the priority queue and pass it to the loadOffset.

2. loadOffset loads the offset value associate with current vertex from the CSR offset values and pass it to the next block.

3. loadCol&Wei loads the ID and weight of the next hop vertices according to the offset values. And it passes these IDs and weights to the loadRes.

4. Queue pushes the next hop vertices into the priority queue. And it sorts the priority again.

This system starts from pushing the source vertex into the queue and iterate until the queue is empty.

Resource
=========
The hardware resource utilizations are listed in the following table.

.. image:: /images/mst_resource.png
   :alt: Resource utilization of minimum spanning tree
   :width: 70%
   :align: center

