.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis Sparse Matrix Library, primitive details
   :description: Vitis Sparse Matrix Library primitive implementation details.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _L1_xBarCol:

**************************************************************************************
Scatter-Gather Logic Implementation
**************************************************************************************

.. toctree::
   :maxdepth: 1
This page provides the implementation details of the scatter-gather logic for selecting column vector entries.

The following figure shows the scatter-gather logic: 

.. image:: /images/xBarCol.png
   :alt: xBarCol Diagram
   :align: center

- The input column vector and column pointer data streams contain multiple entries. For example, four entries as shown in the diagram. 
- The number of entries in the stream can be configured at compile time by ``SPARSE_parEntries``. 
- Split logic distributes the column vector values into different single-entry streams according to their corresponding column pointer values. 
- The merge logic looks through all the streams and merges multiple single entry streams into one stream with multiple column vector values. 
