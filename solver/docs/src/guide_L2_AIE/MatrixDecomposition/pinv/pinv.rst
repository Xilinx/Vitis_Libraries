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
   :keywords: Pseudoinverse
   :description: This function computes the Pseudoinverse of matrix.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

***************
Pseudoinverse
***************

Introduction
==============

This function computes the pseudoinverse of matrix :math:`A`

Template Parameters
---------------------
* `column_num`: the number of columns;
* `row_num`: the number of rows;
* `k_rep`: the number of input matrix;

To access more details, see :ref:`AIE APIs Overview`.

Ports
-------
To access more details, see :ref:`AIE APIs Overview`.


AIE Kernel
===============

Design Notes
--------------------
* Description: 
    Pseudoinverse take the way to calculate singular value decomposition first. After the number of iteration of SVD is reached, it will perform extra post-processing to generate the final result of pseudoinverse. Please take reference of document of SVD.
