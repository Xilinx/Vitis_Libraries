..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis, Solver, Vitis Solver Library, release
   :description: Vitis Solver library release notes.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _release_note:

Release Note
============

.. toctree::
   :hidden:
   :maxdepth: 1

2026.1
------

* **Cholesky** - 2025.2 EA library element now in Production.
* **QRD** - 2025.2 EA library element now in Production
* **QRD_HH** - New EA library element
* **SVD** - New EA library element.
* **Substitution** - New EA library element

+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and class name**                                                |
+=======================================+=============================================================================+
| Cholesky                              |  xf::solver::aie::cholesky_graph                                            |
+---------------------------------------+-----------------------------------------------------------------------------+
| QRD                                   |  xf::solver::aie::qrd_graph                                                 |
+---------------------------------------+-----------------------------------------------------------------------------+
| QRD_HH                                |  xf::solver::aie::qrd_hh_graph                                              |
+---------------------------------------+-----------------------------------------------------------------------------+
| SVD                                   |  xf::solver::aie::svd::svd_graph                                            |
+---------------------------------------+-----------------------------------------------------------------------------+
| Substitution                          |  xf::solver::aie::substitution::substitution_graph                          |
+---------------------------------------+-----------------------------------------------------------------------------+

The Cholesky Decomposition is now at Production standard. 2026.1 adds CASC_LEN and DIAG_INV parameters for performance improvements.
The QRD Decomposition is also now at Production standard.
The QRD Householder Decomposition is introduced at Early Access standard.
The SVD Decomposition is introduced at Early Access standard.
The Substitution function is introduced at Early Acccess standard.


2025.2
------

*  **Cholesky** - New EA library element.
*  **QRD** - New EA library element.


+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and class name**                                                |
+=======================================+=============================================================================+
| Cholesky                              |  xf::solver::aie::cholesky                                                  |
+---------------------------------------+-----------------------------------------------------------------------------+
| QRD                                   |  xf::solver::aie::qrd                                                       |
+---------------------------------------+-----------------------------------------------------------------------------+

Starting from 2025.2, the Cholesky and QRD APIs are available as EA library elements. The previous AIE library elements are deprecated and are not supported in future releases.


2025.1
------

In this release, two APIs running on AIE Engine is added

* Flexible QRD (QR decomposition), for complex float matrix input (using Householdere transformation method).
* Flexible cholesky decomposition, for complex float matrix input. 


2024.1
------

In this release, one API running on AIE Engine is added

* QRF (QR decomposition), for complex float matrix input (using Householdere transformation method)

2023.2
------

In this release, two APIs running on AI Engine are added

* Singular value decomposition, for complex float matrix input
* Pseudoinverse, for complex float matrix input

2023.1
------

In this release, two APIs running on AI Engine are added.

* QRF (QR decomposition), for complex float matrix input (using Modified Gram-Schmidt method)
* Cholesky decomposition, for float / complex float matrix input

2022.1
------

In this release, legacy API from Vivado_HLS to solver library is migrated. They are all hls::stream based API and support std::complex type.

* Cholesky
* Cholesky Inverse
* QR Inverse
* QRF (QR decomposition)
* SVD
