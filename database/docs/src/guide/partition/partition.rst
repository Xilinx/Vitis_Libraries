.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Hash-Partition, hashPartition
   :description: Describes the structure and execution of Hash-Partition.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _guide-hash_partition:

********************************************************
Internals of Hash-Partition
********************************************************

.. toctree::
   :hidden:
   :maxdepth: 2

This document describes the structure and execution of Hash-Partition, implemented as :ref:`hashPartition <cid-xf::database::hashPartition>`.

.. image:: /images/gqe_part_detail.png
   :alt: Hash Join MPU Structure
   :align: center

The Hash-Partition primitive can distribute a giant table into multiple sub-tables. It is a multi-channel and multi-PU design to take the advantage of high memory bandwidth in an AMD FPGA. A URAM array is built in the Partition PU to temporarily store distributed data, and Ping-Pong control is set for the URAM array for higher throughput.

The details of the API description is shown in :ref:`hashPartition <cid-xf::database::hashPartition>`, and a typical setup of ``hashPartition`` could be:

  - HASH_MODE=1
  - KEYW=64
  - PW=192
  - EW=32
  - HASHWH=0
  - HASHWL=8
  - ARW=18
  - CHNM=1
  - COL_NM=8

In the typical setup, the PU number is 1 because there is only one DDR for input and one DDR for output. The resource consumption of the setup is as follows:

  - LUT: 26363
  - Register: 45762
  - BRAM36: 20
  - URAM: 256
  - DSP: 5

.. IMPORTANT::
   Make sure the size of the input table is smaller than the size of the FPGA storage. Take care that the template parameters which can control the number of URAM, and make sure it can be placed in a single SLR.

.. CAUTION::
   Currently, this primitive only supports that patition size is a power of two, and its maximum is 256.