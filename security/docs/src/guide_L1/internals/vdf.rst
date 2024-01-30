.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis, Security, Library, VDF
   :description: Verifiable Delay Function
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


*************************
Verifiable Delay Function
*************************

.. toctree::
   :maxdepth: 1

Overview
========

A Verifiable Delay Function (VDF) is a function whose evaluation requires running a given number of sequentialsteps, yet the result can be efficiently verified. Here, implement two VDFs: Efficient verifiable delay functions (Wesolowski) and Simple Verifiable Delay Functions (Pietrzak). Its algorithm is defined in `REF Wesolowski`_ and `REF Pietrzak`_.

.. _`REF Wesolowski`: https://eprint.iacr.org/2018/623.pdf

.. _`REF Pietrzak`: https://eprint.iacr.org/2018/627.pdf


Implementation on FPGA
======================

There are three APIs provided: `evaluate`, `verifyWesolowski`, and `verifyPietrzak`. The APIs are completely implemented according to the algorithm of the above reference paper, except that the multiplication of large-bit-width integers is implemented based on Montgomery Production. For Montgomery Production, refer to `REF monProduct`_ for details.

.. _`REF monProduct`: http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.99.1897&rep=rep1&type=pdf
