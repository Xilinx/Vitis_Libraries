.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis, Security, Library, CTR mode
   :description: The Counter (CTR) mode is a typical block cipher mode of operation using block cipher algorithm.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials



********
CTR Mode
********

.. toctree::
   :maxdepth: 1

Overview
========

The Counter (CTR) mode is a typical block cipher mode of operation using block cipher algorithm.
In this version, Advanced Encryption Standard (AES) processing ability is provided.
The cipherkey length for AES should be 128/192/256 bits.
Another limitation is that the working mode works on units of a fixed size (128 bits for 1 block),
but text in the real world has a variety of lengths.
So, the last block of the text provided to this primitive must be padded to 128 bits before encryption or decryption. 

Implementation on FPGA
======================

CTR-AES128, CTR-AES192, and CTR-AES256 modes are supported in this implementation.

.. ATTENTION::
    The bit-width of the interfaces provided is shown as follows:

    +-----------+-----------+------------+-----------+----+
    |           | plaintext | ciphertext | cipherkey | IV |
    +-----------+-----------+------------+-----------+----+
    |CTR-AES128 |    128    |    128     |    128    | 128|
    +-----------+-----------+------------+-----------+----+
    |CTR-AES192 |    128    |    128     |    192    | 128|
    +-----------+-----------+------------+-----------+----+
    |CTR-AES256 |    128    |    128     |    256    | 128|
    +-----------+-----------+------------+-----------+----+


The algorithm flow chart is shown as follows:

.. image:: /images/CTR_working_mode.png
   :alt: algorithm flow chart of CTR
   :width: 100%
   :align: center

As seen from the chart, both encryption and decryption parts of CTR mode have no dependencies,
so the input block of each iteration can be directly calculated by the counter.
Thus, both encryption and decryption part of CTR mode can achieve an initiation interval (II) = 1.

Profiling
=========

CTR-AES128 encryption
---------------------

======= ======= ======= ===== ====== ====== ====== ========
  CLB     LUT      FF    DSP   BRAM   SRL    URAM   CP(ns)
======= ======= ======= ===== ====== ====== ====== ========
 2096    9266    6640     0     2     513     0     2.723
======= ======= ======= ===== ====== ====== ====== ========


CTR-AES128 decryption
---------------------

======= ======= ======= ===== ====== ====== ====== ========
  CLB     LUT      FF    DSP   BRAM   SRL    URAM   CP(ns)
======= ======= ======= ===== ====== ====== ====== ========
 2008    9270    6640     0     2     513     0     2.617
======= ======= ======= ===== ====== ====== ====== ========


CTR-AES192 encryption
---------------------

======= ======= ======= ===== ====== ====== ====== ========
  CLB     LUT      FF    DSP   BRAM   SRL    URAM   CP(ns)
======= ======= ======= ===== ====== ====== ====== ========
 2978    16253   8326     0     6     641     0     3.087
======= ======= ======= ===== ====== ====== ====== ========


CTR-AES192 decryption
---------------------

======= ======= ======= ===== ====== ====== ====== ========
  CLB     LUT      FF    DSP   BRAM   SRL    URAM   CP(ns)
======= ======= ======= ===== ====== ====== ====== ========
 2963    15254   8320     0     6     641     0     3.029
======= ======= ======= ===== ====== ====== ====== ========


CTR-AES256 encryption
---------------------

======= ======= ======= ===== ====== ====== ====== ========
  CLB     LUT      FF    DSP   BRAM   SRL    URAM   CP(ns)
======= ======= ======= ===== ====== ====== ====== ========
 3401    17221   9316     2     0     769     0     3.014    
======= ======= ======= ===== ====== ====== ====== ========


CTR-AES256 decryption
---------------------

======= ======= ======= ===== ====== ====== ====== ========
  CLB     LUT      FF    DSP   BRAM   SRL    URAM   CP(ns)
======= ======= ======= ===== ====== ====== ====== ========
 3286    17229   9316     0     2     769     0     2.857
======= ======= ======= ===== ====== ====== ====== ========



