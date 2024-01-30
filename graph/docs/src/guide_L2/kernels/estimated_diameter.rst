.. 
   .. Copyright © 2021–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.


*************************************************
Internal Design of Estimated Diameter 
*************************************************


Overview
========
The diameter of a graph is the worst-case distance interm of shortest path between any pairs of vertices. In other words, the graph diameter is the largest distance to traverse from one vertex to another when one always take the shortest path.

Algorithm
=========
The implemented algorithm estimates the diameter in a heuristic way. It randomly chooses several vertices and computes the distance from each of the chosen vertex to all the other vertices. And it finds out the maximum distance. The higher the final distance is, the greater the likelihood of getting the actual graph diameter.

Interface
=========
The input should be a directed graph in compressed sparse row (CSR) format.
The result is a value that shows the final estimated diameter. Also, the source and destination of the path are given.

Implementation
==============
The algorithm implementation is shown in the following figure:

.. image:: /images/Diameter_design.png
   :alt: block design of Estimated Diameter
   :width: 80%
   :align: center

There are several SSSP algorithms run in parallel as shown in the figure.

The max module finds the maximum distance found and keep a record of it.

To find more distance between pairs of vertices, you can re-run this kernel multiple times.

Resource
=========
The hardware resource utilizations are listed in the following figure.

.. image:: /images/diameter_resource.png
   :alt: Resource utilization of estimated diameter
   :width: 70%
   :align: center

