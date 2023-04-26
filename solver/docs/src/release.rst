.. 
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

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

2023.1
------

In this release, we add two API running on AI Engine.

* QRF (QR decomposition), for float / complex float matrix input
* Cholesky decomposition, for complex float matrix input

2022.1
------

In this relese, we migrate legacy API from Vivado_HLS to solver library. They're all hls::stream based API and support std::complex type.

* Cholesky
* Cholesky Inverse
* QR Inverse
* QRF (QR decomposition)
* SVD
