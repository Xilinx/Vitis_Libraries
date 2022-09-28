.. 
   Copyright 2022 Xilinx, Inc.
  
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
Internal Design of Maximal Independent Set
*************************************************

Overview
========
Maximal Independent Set is an algorithm that finds the maximal set of vertexs which has no adjacent between each others. For this vertexs set, there is no edge connecting each two, and the size of the vertex set is different by different method.

Algorithm
=========
The implemented Maximal Independent Set (MIS) is based on a iterative method. The algorithm goes serveral rounds to traverse all vertexs for selecting vertexs into MIS set. For each round, label every available vertex with a random weights. Then, if the weights of current vertex is the smallest among its' neighbours, this vertex is labeled into MIS set and unlabeled its' neighbours at the same time. By running several times of the iteration, all vertexed would be traversed and labeled vertexs are grouped into MIS set. The output is the vertex list of MIS set. 

Interface
=========
The input should be a directed graph in compressed sparse row (CSR) format.
The result include MIS vertex list (res_out). 

Implementation
==============
The algorithm implemention is shown in the figure below:

.. image:: /images/MIS.png
   :alt: Figure 1 Maximal Independen Set (MIS) design
   :width: 60%
   :align: center

There are 6 functional blocks as shown in the figure:

1. Get_Unselected_V is responsible to load the next set of vertexs which are labeled as unselected and pass it to the Get_degree.

2. Get_degree get all degrees for each vertex and pass them to the next module.

3. V_edge_stream extract all edges for input vertexs, and assigned random-weights to each of them.

4. Check_V module check vertexs and their connecting edges, if the random-weights of current is the smallest around its' neighbours, then label it as MIS set members; if not, do nothing and move to next candidate

5. Update_CS module check the labeled status for vertexed, if it is labeled as MIS set member, it would be removed from waiting group and also its' neighbours. 

6. Mem_sync update all vertex status for processed vertex.

This processing repeat utils there is no selected vertex for current round.

Resources
=========
The hardware resource utilizations are listed in the following table. The BFS kernel is validated on Alveo U250 board at 300MHz frqeuency.

.. table:: Table 1 Hardware resources
    :align: center

    +-------------------+----------+----------+----------+---------+-----------------+
    |    Kernel         |   BRAM   |   URAM   |    FF    |   LUT   | Frequency(MHz)  |
    +-------------------+----------+----------+----------+---------+-----------------+
    |   mis_kernel      |    786   |    0     |    12    |   13595 |     211.9       |
    +-------------------+----------+----------+----------+---------+-----------------+