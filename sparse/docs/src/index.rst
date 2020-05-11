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
   :keywords: SPARSE, Library, Vitis SPARSE Library, linear, algebra, subroutines, vitis
   :description: Vitis SPARSE Library is a fast FPGA-accelerated implementation of the basic linear algebra subroutines for handling sparse matrices.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

=====================
Vitis SPARSE Library
=====================

Vitis SPARSE Library is a fast FPGA-accelerated implementation of the basic
linear algebra subroutines for handling sparse matrices. Two types of implementations 
in this library, namely L1 primitives and  L2 kernels. Those 
implementations are organized in their corresponding directories L1 and L2.
The L1 primitives' implementations can be leveraged by FPGA hardware developers.
The L2 kernels' implementations provide usage examples for system and host code developers.

Since all the kernel code is developed with the permissive Apache 2.0 license,
advanced users can easily tailor, optimize or combine them for their own need.
Demos and usage examples of different levels' implementations are also provided
for reference. 

.. toctree::
   :caption: Library Overview
   :maxdepth: 1

   overview.rst
   release.rst
 
.. toctree::
   :caption: L1 User Guide
   :maxdepth: 2

   user_guide/L1_user_guide.rst

Index
-----

* :ref:`genindex` 
