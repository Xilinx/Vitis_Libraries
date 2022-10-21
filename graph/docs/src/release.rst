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

.. _release_note:

Release Note
============

.. toctree::
   :hidden:
   :maxdepth: 1

Vitis Graph Library is an open-sourced Vitis library written in C++ for accelerating graph applications in a variety of use cases. It now covers a level of acceleration: the module level (L1), the pre-defined kernel level (L2) and the asynchronous software level (L3).

2022.2
----
- Add a new algorithm for merging vertices belong to same community.
- Enhanced Louvain Modularity. L2 Louvain Modularity is able to support large-degree graphs.
- One known issue for L3 Louvain Modularity: it works well on U55C in 2022.1 release, but it has been enouncing routing problem in this release. To be fixed in comming addition release.

2022.1
----
- Add a new algorithm Maximal Independent Set.
- Enhanced Louvain Modularity. L2 Louvain Modularity is able to support large-scale graphs.
- Add a L3 API to divide huge graphs into multiple parts and add other data structures to support the Louvain Modularity on these parts.

2021.2
----
The algorithms implemented by Vitis Graph Library include:
 - Similarity analysis: Cosine Similarity, Jaccard Similarity, k-nearest neighbor. From 2021.2, the 'weight' feature is supported for Cosin Similarity. 
 - Centrality analysis: PageRank.
 - Pathfinding: Single Source Shortest Path (SSSP), Multi-Sources Shortest Path (MSSP).
 - Connectivity analysis: Weekly Connected Components and Strongly Connected Components.
 - Community Detection: Louvain Modularity, Label Propagation and Triangle Count.
 - Search: Breadth First Search, 2-Hop Search 
 - Graph Format: Renumber(2021.2), Calculate Degree and Format Convert between CSR and CSC.
