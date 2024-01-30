.. 
   .. Copyright © 2022–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.


*************************************************
Internal Design of Maximal Independent Set
*************************************************

Overview
========
Maximal Independent Set is an algorithm that finds the maximal set of vertices that have no adjacent between each other. For this vertices set, there is no edge connecting any of the two, and the size of the vertex set is different by different method.

Algorithm
=========
The implemented Maximal Independent Set (MIS) is based on an iterative method. The algorithm goes several rounds to traverse all vertices to select vertices into an MIS set. For each round, label every available vertex with a random weight. Then, if the weights of the current vertex is the smallest among its neighbours, this vertex is labeled into an MIS set and unlabeled its neighbours at the same time. By running several times of the iteration, all vertexed would be traversed and labeled vertices are grouped into an MIS set. The output is the vertex list of the MIS set. 

Interface
=========
The input should be a directed graph in compressed sparse row (CSR) format.
The result includes the MIS vertex list (res_out). 

Implementation
==============
The algorithm implementation is shown in the following figure:

.. image:: /images/MIS.png
   :alt: Figure 1 Maximal Independen Set (MIS) design
   :width: 60%
   :align: center

There are six functional blocks as shown in the figure:

1. Get_Unselected_V is responsible to load the next set of vertices, which are labeled as unselected and pass it to the Get_degree.

2. Get_degree: get all degrees for each vertex and pass them to the next module.

3. V_edge_stream: extract all edges for input vertices, and assigned random-weights to each of them.

4. Check_V module: check vertices and their connecting edges. If the random-weights of current is the smallest around its neighbors, label it as the MIS set members. If not,move to next candidate.

5. Update_CS module: check the labeled status for vertexed. If it is labeled as an MIS set member, it along with the neighbors,are removed from the waiting group. 

6. Mem_sync: update all vertex status for processed vertex.

This processing repeat until there is a selected vertex for the current round.

Resources
=========
The hardware resource utilizations are listed in the following table. The BFS kernel is validated on an AMD Alveo |trade| U250 board at 300MHz frequency.

.. table:: Table 1 Hardware resources
    :align: center

    +-------------------+----------+----------+----------+---------+-----------------+
    |    Kernel         |   BRAM   |   URAM   |    FF    |   LUT   | Frequency(MHz)  |
    +-------------------+----------+----------+----------+---------+-----------------+
    |   mis_kernel      |    786   |    0     |    12    |   13595 |     211.9       |
    +-------------------+----------+----------+----------+---------+-----------------+

    .. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim: