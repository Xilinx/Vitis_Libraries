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
   :keywords: Vitis Sparse Matrix Library, kernel
   :description: The kernel implementation to support cscmv opreation.

.. _L2_intro:

************************************
CSCMV Overview
************************************


The CSCMV operation is implemented by a group of kernels connected via AXI STREAM interfaces. 


.. image:: /images/cscmv.png
   :alt: xBarCol Diagram
   :align: center

- The ``loadColPtrVal`` kernel reads the column vector and pointer entries from DDR and send them to the ``xBarCol`` kernel to select corresponding column vector entries for NNZs. 
- The ``cscRowPkt`` kernel reads the value and row indices of NNZs from one HBM channel and mulplies the values with their corresponding column entries and accumulates the results along the row indices. 
- The result row vector entries are sent to ``storeDatPkt`` kernel to be written back to DDR. 
.. NOTE::
   Only one HBM channel is implemented to compute a block of sparse matrix vector multiplication results. Future versions may support multiple HBM channels with each channel storing part of the sparse matrix data.
- Each HBM channel connects to its own computation path to allow multiple blocks of sparse matrix being processed in parallel. 
- The ``dispCol`` module implemented as L1 primitive will be used to distribute the column vector entries accross multiple HBM channels, hence multiple computation paths for supporting this parallelism. 
- Design with multiple kernels connected via AXI STREAM interfaces allows you to control the placement of each kernel in the most suitable SLRs and avoid congestions at the routing stage.

