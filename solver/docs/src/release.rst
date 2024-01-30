.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

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

2023.2
------

In this release, two APIs running on AI Engine are added

* Singular value decomposition, for complex float matrix input
* Pseudoinverse, for complex float matrix input

2023.1
------

In this release, two APIs running on AI Engine are added.

* QRF (QR decomposition), for float / complex float matrix input
* Cholesky decomposition, for complex float matrix input

2022.1
------

In this release, legacy API from Vivado_HLS to solver library is migrated. They are all hls::stream based API and support std::complex type.

* Cholesky
* Cholesky Inverse
* QR Inverse
* QRF (QR decomposition)
* SVD
