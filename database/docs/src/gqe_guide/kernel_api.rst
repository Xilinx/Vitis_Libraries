.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis Database Library, GQE, kernel, api
   :description: The GQE kernel application programming interface reference.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _gqe_kernel_api:

*******************
GQE Kernel APIs
*******************

These APIs are implemented as OpenCL™ kernels:

.. toctree::
   :maxdepth: 1

.. include:: ../rst_2/global.rst
   :start-after: FunctionSection

.. NOTE::
   3-in-1 GQE has been tested on the AMD Alveo™ U50 card, and makes only use of HBM. Only gqeAggr is now still using the Alveo U280 card, and makes use of both HBM and double-data rate (DDR). While other cards like the U200 and U250 are not supported out-of-box, porting and gaining acceleration is surely possible, with tailoring and tuning.