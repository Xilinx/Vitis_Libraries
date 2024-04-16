.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis Sparse Matrix Library, primitive details
   :description: Vitis Sparse Matrix Library primitive implementation details.

.. _L1_dispCol:

**************************************************************************************
Column Vector Buffering and Distribution Implementation
**************************************************************************************

.. toctree::
   :maxdepth: 1

This page provides the column vector buffering and distribution implementation details. The following figure shows the column vector buffering and distribution logic. 

.. image:: /images/dispCol.png
   :alt: cscRow Diagram
   :align: center

- The input parameter streams contain the information of the size of each column vector block, the minimum and maximum column vector entry indices. 
- The ``dispColVec`` module reads the parameters and multiple column vector enties, buffers the column entires in its own on-chip memory and forwards the rest parameters and vector entires to the next ``disColVec`` module. If the module is the last one in the chain, the forwarding logic is omitted. 
- After the buffereing operation, each ``dispColVec`` module reads out the data from the on-chip memory and sends them to the output stream to be processed by its own computation path. 
- Apart from buffering and reading data operations, the ``dispColVec`` module also aligns the data and pads the data according to ``SPARSE_parEntries`` and the minimum and maximum row indices. 
