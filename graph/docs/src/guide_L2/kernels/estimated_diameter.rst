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
Internal Design of Estimated Diameter 
*************************************************


Overview
========
The diameter of a graph is the worst-case distance interm of shortest path between any pairs of vertices. In other word, the graph diameter is the largest distance to traverse from one vertex to another when one always take the shortest path.

Algorithm
=========
The implemented algorithm estimates the diameter in a heuristic way. It randomly chooses several vertices and computes the distance from each of the chosen vertex to all the other vertices. And it finds out the maximum distance. The higher the final distance is, the greater the likelihood of getting the actual graph diameter.

Interface
=========
The input should be a directed graph in compressed sparse row (CSR) format.
The result is a value that shows the final estimated diameter. Also, the source and destination of the path are given.

Implementation
==============
The algorithm implemention is shown in the figure below:

.. image:: /images/Diameter_design.png
   :alt: block design of Estimated Diameter
   :width: 80%
   :align: center

There are several SSSP algorithms run in parallel as shown in the figure.

The max module will find the maximum distance found and keep a record of it.

To find more distance between pairs of vertices, user can re-run this kernel multiple times.

Resource
=========
The hardware resource utilizations are listed in the following table.

.. image:: /images/diameter_resource.png
   :alt: Resource utilization of estimated diameter
   :width: 70%
   :align: center

