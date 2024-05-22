.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.
.. _release_note:

Release Note
============

.. toctree::
   :hidden:
   :maxdepth: 1

An AMD Vitis |trade| Graph Library is an open-sourced Vitis library written in C++ for accelerating graph applications in a variety of use cases. It now covers a level of acceleration: the module level (L1), the pre-defined kernel level (L2) and the asynchronous software level (L3).

2024.1
------
There are some known issues for this release.

- Louvain Modularity cases meet routing failure when build with 2024.1 tool. Last known working version is 2023.2.


2022.2
----
- Add a new algorithm for merging vertices belong to same community.
- Enhanced Louvain Modularity. L2 Louvain Modularity is able to support large-degree graphs.
- One known issue for L3 Louvain Modularity: it works well on U55C in 2022.1 release, but it has been enouncing routing problem in this release. To be fixed in the coming addition release.

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


.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
