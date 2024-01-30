.. 
  .. Copyright © 2019-2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _guide-stream_sync:

*****************************************
Internals of streamSync
*****************************************

.. toctree::
   :hidden:
   :maxdepth: 3

This document describes the structure and execution of streamSync,
implemented as :ref:`streamSync <cid-xf::common::utils_hw::streamSync>` function.

.. image:: /images/stream_sync.png
   :alt: stream sync Structure
   :width: 80%
   :align: center

The streamSync synchronizes all streams by sharing a same end-flag stream. Each input data stream has an end-flag stream. After sync, all output streams share one end-flag stream, and each of them is a duplicate of an input stream. That is to say, before output an end-flag, each ouput stream should output a data from corresponding input stream.

.. CAUTION::
  Applicable conditions.

  1. It assumes the input elements in each input stream have the same number.
   
  2. The data type of input stream is same as the one of output.

