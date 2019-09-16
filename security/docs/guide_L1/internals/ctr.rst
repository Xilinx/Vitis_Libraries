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

********
CTR Mode
********

.. toctree::
   :maxdepth: 1

Overview
========

The Counter (CTR) mode is a typical block cipher mode of operation using block cipher algorithm.
In this version, we provide Advanced Encryption Standard (AES) processing ability,
the cipherkey length for AES should be 128/192/256 bits.
Another limitation is that our working mode works on units of a fixed size (128 bits for 1 block),
but text in the real world has a variety of lengths.
So, the last block of the text provided to this primitive must be padded to 128 bits before encryption or decryption. 

Implementation on FPGA
======================

We support CTR-AES128, CTR-AES192, and CTR-AES256 modes in this implementation.

.. ATTENTION::
    The bit-width of the interfaces we provide is shown as follows:
    ============ =========== ============ =========== ====
                  plaintext   ciphertext   cipherkey   IV 
    ============ =========== ============ =========== ====
     CTR-AES128      128         128          128      128
    ============ =========== ============ =========== ====
     CTR-AES192      128         128          192      128
    ============ =========== ============ =========== ====
     CTR-AES256      128         128          256      128
    ============ =========== ============ =========== ====

The algorithm flow chart is shown as follow:

.. image:: /images/CTR_working_mode.png
   :alt: algorithm flow chart of CTR
   :width: 100%
   :align: center

As we can see from the chart, both encryption and decryption part of CTR mode has no dependencies,
so the input block of each iteration can be directly calculated by the counter.
Thus, both encryption and decryption part of CTR mode can achieve an initiation interval (II) = 1.

Profiling
=========

CTR-AES128 encryption
---------------------

====== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
====== ====== ====== ===== ====== ===== ====== ========

====== ====== ====== ===== ====== ===== ====== ========

CTR-AES128 decryption
---------------------

====== ====== ====== ===== ====== ====== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL    URAM   CP(ns)
====== ====== ====== ===== ====== ====== ====== ========

====== ====== ====== ===== ====== ====== ====== ========

CTR-AES192 encryption
---------------------

====== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
====== ====== ====== ===== ====== ===== ====== ========

====== ====== ====== ===== ====== ===== ====== ========

CTR-AES192 decryption
---------------------

====== ====== ====== ===== ====== ====== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL    URAM   CP(ns)
====== ====== ====== ===== ====== ====== ====== ========

====== ====== ====== ===== ====== ====== ====== ========

CTR-AES256 encryption
---------------------

====== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
====== ====== ====== ===== ====== ===== ====== ========

====== ====== ====== ===== ====== ===== ====== ========

CTR-AES256 decryption
---------------------

====== ====== ====== ===== ====== ====== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL    URAM   CP(ns)
====== ====== ====== ===== ====== ====== ====== ========

====== ====== ====== ===== ====== ====== ====== ========

