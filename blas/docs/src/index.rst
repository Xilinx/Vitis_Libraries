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

.. meta::
   :keywords: BLAS, Library, Vitis BLAS Library, linear, algebra, subroutines, vitis
   :description: Vitis BLAS Library is a fast FPGA-accelerated implementation of the standard basic linear algebra subroutines (BLAS).
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

=====================
Vitis BLAS Library
=====================

Vitis BLAS Library is a fast FPGA-accelerated implementation of the standard basic
linear algebra subroutines (BLAS). Three types of implementations are provided
in this library, namely L1 primitives, L2 kernels and L3 software APIs. Those 
implementations are organized in their corresponding directories L1, L2 and L3.
The L1 primitives' implementations can be leveraged by FPGA hardware developers.
The L2 kernels' implementations provide usage examples for Vitis host code developers.
The L3 software APIs' implementations provide C, C++ and Python function interfaces
to allow pure software developers  to offload BLAS operations to pre-built FPGA images, 
also called overlays. 

Since all the kernel code is developed with the permissive Apache 2.0 license,
advanced users can easily tailor, optimize or combine them for their own need.
Demos and usage examples of different levels' implementations are also provided
for reference. 

.. toctree::
   :caption: Introduction
   :maxdepth: 1

   Overview <overview.rst>
   Release Note <release.rst>
 
.. toctree::
   :caption: User Guide
   :maxdepth: 1 

   L1 Primitives User Guide <user_guide/L1/L1.rst>
   L2 Kernels User Guide <user_guide/L2/L2.rst>
   L3 API User Guide <user_guide/L3/L3.rst>
   
.. toctree::
   :caption: Benchmark
   :maxdepth: 1
   
   Benchmark <benchmark.rst>
