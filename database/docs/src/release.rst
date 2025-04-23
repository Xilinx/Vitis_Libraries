.. Copyright © YEAR–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis, Database, Vitis Database Library, release
   :description: Vitis Database library release notes.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _release_note:

Release Note
============

.. toctree::
   :hidden:
   :maxdepth: 1

.. note:: Known Issue

   * GQE Kernel does not build with 2023.1 tool. Last known working version is 2022.2.

2022.2
------

In the 2022.2 release, the GQE implementation supports AMD Alveo™ U55C cards.

2022.1
------

In the 2022.1 release, database library introduced (1) GQE combines partition/bloomfilter/join into a single kernel (2) Key-Value store offloading.

* Merge partition/bloomfilter/join into single kernel makes the three operators share resources on the FPGA. Although such a kernel could only performance one of the three operators at the same time, it will take much less resource than three standalone kernels. With such design, it will help eliminate the time cost to switch xclbins for different operators. Also such a design will enable the pipelined execution of kernels and reduce the DMA workload. This design targets for the Alveo U50. U50 costs less and still retains HBM.

* Key-Value store offloading introduced a new kernel for the accelerate K-V compaction operation in a log-structure merge tree database.

.. note:: Known Issue

   * GQE filter case hw_emu will fail due to a design change. This will be fixed later.


2021.2
------

In the 2021.2 release, GQE start to support asynchronous input/output feature, along with multi-card support.

* Asynchronous input/output: Use std::future<size_t> to notify GQE L3 readiness of each input sections, and its value is the effective row number of the input section. It will use std::promise<size_t> to notify the caller of GQE L3 the readiness of each section of the final result, and its value is the effective row number of output section. Asynchronous support will allow the FPGA start to process as soon as part of the input data is ready. In such way, the FPGA will not wait until all input data is ready and shrink the overhead to prepare data for FPGA.
* Multi-Cards support: Allows you to identify multiple Alveo cards that are suitable for working. It will load the same xclbins for these cards and called them when there is more task than one cards could handle at the same time. The data structure will also keep the pinned host buffer and device buffer alive before they are explicitly released. This will help save the time to load xclbins/create pinned buffer/create device buffer.

2021.1
------

In 2021.1 release, GQE receives early access support for the following features:

* 64-bit join support: Now the gqeJoin kernel and its companion gqePart kernel has been extended to 64-bit key and payload, so that larger scale of data can be supported.
* Initial Bloom-filter support: The gqeJoin kernel now ships with a mode in which it executes Bloom-filter probing. This improves efficiency on certain multi-node flows where minimizing data size in early stage is important.

Both features are offered now as L3 pure software APIs; check the corresponding L3 test cases.

2020.2
------

The 2020.2 release brings a major update to the GQE kernel design and brand new L3 APIs for JOIN and GROUP-BY AGGREGATE.

* The GQE kernels now take each column as an input buffer, which can greatly simplify the data preparation on the host-code side. Also, allocating multiple buffers on the host side turns should cause less out-of-memory issues comparing to a big contiguous one, especially when the server is under heavy load.
* The L2 layer now provides command classes to generate the configuration bits for GQE kernels. Developers no longer have to dive into the bitmap table to understand which bit(s) to toggle, to enable, or disable a function in GQE pipeline. Thus the host code can be less error-prone and more sustainable.
* The all-new experimental L3 APIs are built with experiments and insights into scaling the problem size that GQE can handle. They can breakdown the tables into parts based on hash and call the GQE kernels multiple rounds in a well-schedule fashion. The strategy of execution is separated from execution, so database gurus can fine-tune the execution based on table statistics, without messing with the OpenCL|trade| execution part.

2020.1
------

The 2020.1 release contains:

* Compound sort API (compoundSort): Previously, three sort algorithm modules have been provided, and this new API combines ``insertSort`` and ``mergeSort`` to provide a more scalable solution for on-chip sorting. When working with 32-bit integer keys, the UltraRAM resource on one SLR could support the design to scale to 2M entries.
* Better HBM bandwidth usage in hash-join (``hashJoinV3``): In the 2019.2 Alveo U280 shell, error-correction code (ECC) has been enabled. So sub-ECC size write to HBM becomes read-modify-write and wastes some bandwidth. To avoid this problem, the ``hashJoinV3`` primitive in this release has been modified to use 256-bit port.
* Various bug fixes: Many small issues have been cleaned up, especially in the host code of L2/demos.

2019.2
------

The 2019.2 release introduces generic query engine (GQE) kernels, which are post-bitstream programmable and allow different SQL queries to be accelerated with one xclbin. It is conceptually a big step from per-query design, and a sound example of AMD's acceleration approach.

Each GQE kernel is essentially a programmable pipeline of the execution step primitives, which can be enabled or bypassed via runtime configuration.

Internal Release
----------------

The first release provides a range of HLS primitives for mapping the execution plan steps in relational database. They cover most of the occurrence in the plan generated from TPC-H 22 queries.

These modules work in streaming fashion and can work in parallel when possible.
