.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

Vitis Graph Library
==========================

An AMD Vitis |trade| Graph Library is an open-sourced Vitis library written in C++ for accelerating graph applications in a variety of use cases. It now covers a level of acceleration: the pre-defined kernel level (L2), and evolves to offer the module level (L1) and the software API level (L3).

Currently, this includes the following algorithm implementation:

 - Similarity analysis: Cosine Similarity, Jaccard Similarity, k-nearest Neighbor.
 - Centrality analysis: PageRank.
 - Pathfinding: Single Source Shortest Path (SSSP), Multi-Source Shortest Path (MSSP), Minimum Spanning Tree and Estimated Diameter.
 - Connectivity analysis: Weakly Connected Components and Strongly Connected Components.
 - Community Detection: Louvain Modularity (From 22.1, Louvain API can support large-scale graphs), Label Propagation and Triangle Count.
 - Search: Breadth First Search and 2-Hop Search.
 - Graph Format: Renumber, Calculate Degree and Format Conversion between CSR and CSC.


Shell Environment
------------------

Setup the build environment using the Vitis and XRT scripts.

.. ref-code-block:: bash
	:class: overview-code-block

        source <install path>/Vitis/2022.1/settings64.sh
        source /opt/xilinx/xrt/setup.sh
        export PLATFORM_REPO_PATHS=/opt/xilinx/platforms

Setting ``PLATFORM_REPO_PATHS`` to the installation folder of platform files can enable makefiles.
in this library to use ``PLATFORM`` variable as a pattern.
Otherwise, full path to .xpfm file needs to be provided via ``PLATFORM`` variable.


.. toctree::
   :caption: Introduction
   :maxdepth: 1

   Overview <overview.rst>
   Release Note <release.rst>
   Vitis Graph Library Tutorial <tutorial.rst>

.. toctree::
   :caption: L1 User Guide
   :maxdepth: 3

   API Document <guide_L1/api.rst>

.. toctree::
   :maxdepth: 2

   Design Internals <guide_L1/internals.rst>

.. toctree::
   :caption: L2 User Guide
   :maxdepth: 3

   API Document <guide_L2/api.rst>

.. toctree::
   :maxdepth: 2

   Design Internals <guide_L2/internals.rst>

.. toctree::
   :caption: L3 User Guide
   :maxdepth: 4

   User Guide <guide_L3/utilization_L3.rst>

.. toctree::
   :maxdepth: 2

   API Document <guide_L3/api.rst>

.. toctree::
   :caption: TigerGraph Plugin
   :maxdepth: 3

   TigerGraph Integration <plugin/tigergraph_integration.rst>

.. toctree::
   :caption: Benchmark 
   :maxdepth: 1 

   Benchmark <benchmark.rst>

Indices and tables
------------------

* :ref:`genindex`
* :ref:`search`

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
