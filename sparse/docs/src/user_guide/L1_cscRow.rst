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
   :keywords: Vitis Sparse Matrix Library, primitive details
   :description: Vitis Sparse Matrix Library primitive implementation details.

.. _L1_cscRow:

**************************************************************************************
Row-wise Accumulator Implementation
**************************************************************************************

.. toctree::
   :maxdepth: 1
This page provides the Row-wise accumulator implementation details. The following figure shows the row-wise accumulator logic: 

.. image:: /images/cscRow.png
   :alt: cscRow Diagram
   :align: center

- There are three input streams, namely the columnvector value stream, the NNZ value stream and the NNZ row indices stream. 
- Each stream contains multiple entries being processed in parallel. For example, the ``4`` entries shown in the figure. 
- The number of parallel entries can be configured at the compile time by ``SPARSE_parEntries``. 
- The ``xBarRow`` logic carries out the multiplication of the column value and the NNZ value, and distributes the results and their corresponding row indices to the accumulator logic implemented by the ``rowInterleave`` and ``rowAcc`` modules. 
- The ```rowInterleave`` module is designed to address the long latency of floating point accumulation by overprovising the accumulators. 
- Each ``rowInterleave`` module will drive several ``rowAcc`` modules for row-wise accumulation. The number of ``rowInterleave`` modules is configured at compile time by ``SPARSE_parEntries``. 
- The number of ``rowAcc`` modules driven by each ``rowInterleave`` module is configured by ``SPARSE_parGroups`` at the compile time.
- There is a on-chip memory block in each ``rowAcc`` module to store the accumulation results. 
- Once the accumulation operation is finished, each ``rowAcc`` module will read out the data in the on-chip memory and send them to ``rowAgg`` module to be merged together and written to the device memory, for example, HBM.
