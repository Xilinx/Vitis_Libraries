.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. toctree::
   :hidden:

.. _overview:

Overview
------------

The AMD Vitis™ Data Analytics Library is an open-sourced Vitis library written in C++ for accelerating data analytics applications in a variety of use cases.

Three categories of APIs are provided by this library, namely:

* **Data Mining** APIs, including all most common subgroups:

  * *Classification*: Decision tree, random forest, native Bayes, and SVM algorithms.
  * *Clustering*: K-means algorithm.
  * *Regression*: Linear, gradient, and decision tree based algorithms.

* **Text Processing** APIs for unstructured information extraction and transformation. New in the 2020.2 release.

  * *Regular expression* with capture groups is a powerful and popular tool of information extraction.
  * *Geo-IP* enables looking up the IPv4 address for geographic information.
  * Combining these APIs, a complete demo has been developed to batch transform the Apache HTTP server log into structured JSON text.

* **DataFrame** APIs, also new in 2020.2, can be used to store and load multiple types of data with both fixed and variable length into DataFrames.

  * The in-memory format follows the design principles of `Apache Arrow <https://arrow.apache.org/>`_ with the goal of allowing access without per-element transformation.
  * Loaders from common formats are planned to be added in future releases, for example, CSV and JSONLine loaders.

* **GeoSpatial** APIs for for spatial analysis and spatial data mining; new in the 2022.1 release.

  * Spatial Join API inserts the columns from one feature table to another based on location or proximity.
  * KNN is often used to find the K nearest neighbors around the center point.

Like most other Vitis sublibraries, the Data Analytics Library also organize its APIs by levels.

* The bottom level, L1, is mostly hardware modules with its software configuration generators.
* The second level, L2, provides kernels that are ready to be built into the FPGA binary and invoked with standard OpenCL™ calls.
* The top level, L3, is meant for solution integrators as pure software C++ APIs.
Little background knowledge of FPGA or heterogeneous development is required for using L3 APIs.

At each level, the APIs are designed to be as reusable as possible, combined with the corporate-friendly Apache 2.0 license, advanced users are empowered to easily tailor, optimize, and assemble solutions.
