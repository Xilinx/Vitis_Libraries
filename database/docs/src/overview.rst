.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis, Database, Vitis Database Library, Alveo
   :description: Vitis Database Library is an open-sourced Vitis library written in C++ for accelerating database applications in a variety of use cases.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _brief:

Overview
--------

The AMD Vitis™ Database Library is an open-sourced Vitis library written in C++ and released under `Apache 2.0 license <https://www.apache.org/licenses/LICENSE-2.0>`_ for accelerating database applications in a variety of use cases.

The main target audience of this library is SQL engine developers who want to accelerate the query execution with FPGA cards. Currently, this library offers three levels of acceleration:

* At the module level, it provides an optimized hardware implementation of most common relational database execution plan steps, like hash-join and aggregation.
* At the kernel level, the post-bitstream-programmable kernel can be used to map a sequence of execution plan steps, without having to compile FPGA binaries for each query.
* The software APIs level wraps the details of offloading acceleration with programmable kernels and allows you to accelerate supported database tasks on AMD Alveo™ cards without heterogeneous development knowledge.

At each level, this library strives to make modules configurable through documented parameters, so that advanced users can easily tailor, optimize, or combine with property logic for specific needs. Test cases are provided for all the public APIs and can be used as examples of usage.

Generic Query Engine
--------------------

This library refers its solution to accelerated key execution step(s) in the query plan, like table JOIN as Generic Query Engine (GQE). GQE consists of post-bitstream programmable kernel(s) and the corresponding software stack.

.. image:: /images/gqe_overview.png
   :alt: General Query Engine Overview
   :scale: 50%
   :align: left

.. NOTE::
   GQE is still under extensive development, so its APIs are subject to future changes.
