.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis Database Library, GQE, kernel, TPC-H
   :description: TPC-H queries with GQE.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _tpch:

**********************
TPC-H Queries with GQE
**********************

.. NOTE::
   Source code reference: https://github.com/Xilinx/Vitis_Libraries/tree/2019.2/database/L2/demos
   
   TPC-H queries have been obsolete in GQE 2020.2, because GQE 2020.2 has been redesigned with non-compatible APIs for better integration with SQL engines.

GQE acceleration on TPC-H queries is introduced in Section :ref:`gqe_kernel_demo`. The current experiment only involves the GQE kernels, and a dedicated host C++ code is developed for each query.