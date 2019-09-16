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
CFB Mode
********

.. toctree::
   :maxdepth: 1

Overview
========

The Cipher Feedback (CFB) mode is a typical block cipher mode of operation using block cipher algorithm.
In this version, we provide Data Encryption Standard (DES) and Advanced Encryption Standard (AES) processing ability,
the cipherkey length for DES should be 64 bits, and 128/192/256 bits for AES.
Another limitation is that our working mode works on units of a fixed size (64 or 128 bits for 1 block),
but text in the real world has a variety of lengths.
So, the last block of the text provided to this primitive must be padded to 128 bits before encryption or decryption.
Although, CFB1 and CFB8 modes share the same interface with CFB128 mode, the plaintext and ciphertext is prcossed bit-by-bit or byte-by-byte not block-by-block for CFB1 and CFB8 modes repectively.

Implementation on FPGA
======================

We support three different modes in this implementation: CFB1, CFB8, and CFB128.
The length of the text to be processed in one iteration corresponding to specific mode.
CFB1 is 1 bit per iteration, CFB8 is 8 bits per iteration, and CFB128 is 128 bits per iteration.

.. ATTENTION::
    The bit-width of the interfaces we provide is shown as follows:
    =============== =========== ============ =========== ====
                     plaintext   ciphertext   cipherkey   IV 
    =============== =========== ============ =========== ====
       CFB1-DES         64          64           64       64
    =============== =========== ============ =========== ====
     CFB1-AES128        128         128          128      128
    =============== =========== ============ =========== ====
     CFB1-AES192        128         128          192      128
    =============== =========== ============ =========== ====
     CFB1-AES256        128         128          256      128
    =============== =========== ============ =========== ====
       CFB8-DES         64          64           64       64
    =============== =========== ============ =========== ====
     CFB8-AES128        128         128          128      128
    =============== =========== ============ =========== ====
     CFB8-AES192        128         128          192      128
    =============== =========== ============ =========== ====
     CFB8-AES256        128         128          256      128
    =============== =========== ============ =========== ====
       CFB128-DES       64          64           64       64
    =============== =========== ============ =========== ====
     CFB128-AES128      128         128          128      128
    =============== =========== ============ =========== ====
     CFB128-AES192      128         128          192      128
    =============== =========== ============ =========== ====
     CFB128-AES256      128         128          256      128
    =============== =========== ============ =========== ====

The algorithm flow chart is shown as follow, and the length of the text is denoted as 's':

.. image:: /images/CFB_working_mode.png
   :alt: algorithm flow chart of CFB
   :width: 100%
   :align: center

As we can see from the chart, the encryption part of each CFB mode has loop-carried dependency which is enforced by the algorithm, the input block of each iteration (except for iteration 0) needs a feedback data from its last iteration.
Thus, the initiation interval (II) of each CFB encryption mode cannot achieve an II = 1.
However, the decryption part of each CFB mode has no dependencies, so it can achieve an II = 1.

Profiling
=========

CFB1-DES encryption
-------------------

====== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
====== ====== ====== ===== ====== ===== ====== ========
 329    1548   2520    0     0      0     0     2.658
====== ====== ====== ===== ====== ===== ====== ========

CFB1-DES decryption
-------------------

====== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
====== ====== ====== ===== ====== ===== ====== ========
 337    1532   2482    0     0      9     0     2.618
====== ====== ====== ===== ====== ===== ====== ========

CFB1-AES128 encryption
----------------------

====== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
====== ====== ====== ===== ====== ===== ====== ========

====== ====== ====== ===== ====== ===== ====== ========

CFB1-AES128 decryption
----------------------

====== ====== ====== ===== ====== ====== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL    URAM   CP(ns)
====== ====== ====== ===== ====== ====== ====== ========

====== ====== ====== ===== ====== ====== ====== ========

CFB1-AES192 encryption
----------------------

====== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
====== ====== ====== ===== ====== ===== ====== ========

====== ====== ====== ===== ====== ===== ====== ========

CFB1-AES192 decryption
----------------------

====== ====== ====== ===== ====== ====== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL    URAM   CP(ns)
====== ====== ====== ===== ====== ====== ====== ========

====== ====== ====== ===== ====== ====== ====== ========

CFB1-AES256 encryption
----------------------

====== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
====== ====== ====== ===== ====== ===== ====== ========

====== ====== ====== ===== ====== ===== ====== ========

CFB1-AES256 decryption
----------------------

====== ====== ====== ===== ====== ====== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL    URAM   CP(ns)
====== ====== ====== ===== ====== ====== ====== ========

====== ====== ====== ===== ====== ====== ====== ========

CFB8-DES encryption
-------------------

====== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
====== ====== ====== ===== ====== ===== ====== ========
 344    1628   2703    0     0      0     0     2.478
====== ====== ====== ===== ====== ===== ====== ========

CFB8-DES decryption
-------------------

====== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
====== ====== ====== ===== ====== ===== ====== ========
 336    1613   2723    0     0     13     0     2.474
====== ====== ====== ===== ====== ===== ====== ========

CFB8-AES128 encryption
----------------------

====== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
====== ====== ====== ===== ====== ===== ====== ========

====== ====== ====== ===== ====== ===== ====== ========

CFB8-AES128 decryption
----------------------

====== ====== ====== ===== ====== ====== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL    URAM   CP(ns)
====== ====== ====== ===== ====== ====== ====== ========

====== ====== ====== ===== ====== ====== ====== ========

CFB8-AES192 encryption
----------------------

====== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
====== ====== ====== ===== ====== ===== ====== ========

====== ====== ====== ===== ====== ===== ====== ========

CFB8-AES192 decryption
----------------------

====== ====== ====== ===== ====== ====== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL    URAM   CP(ns)
====== ====== ====== ===== ====== ====== ====== ========

====== ====== ====== ===== ====== ====== ====== ========

CFB8-AES256 encryption
----------------------

====== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
====== ====== ====== ===== ====== ===== ====== ========

====== ====== ====== ===== ====== ===== ====== ========

CFB8-AES256 decryption
----------------------

====== ====== ====== ===== ====== ====== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL    URAM   CP(ns)
====== ====== ====== ===== ====== ====== ====== ========

====== ====== ====== ===== ====== ====== ====== ========

CFB128-DES encryption
---------------------

====== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
====== ====== ====== ===== ====== ===== ====== ========
 341    1677   2734    0     0      0     0     2.290
====== ====== ====== ===== ====== ===== ====== ========

CFB128-DES decryption
---------------------

====== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
====== ====== ====== ===== ====== ===== ====== ========
 367    1734   2802    0     0     33     0     2.216
====== ====== ====== ===== ====== ===== ====== ========

CFB128-AES128 encryption
------------------------

====== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
====== ====== ====== ===== ====== ===== ====== ========

====== ====== ====== ===== ====== ===== ====== ========

CFB128-AES128 decryption
------------------------

====== ====== ====== ===== ====== ====== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL    URAM   CP(ns)
====== ====== ====== ===== ====== ====== ====== ========

====== ====== ====== ===== ====== ====== ====== ========

CFB128-AES192 encryption
------------------------

====== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
====== ====== ====== ===== ====== ===== ====== ========

====== ====== ====== ===== ====== ===== ====== ========

CFB128-AES192 decryption
------------------------

====== ====== ====== ===== ====== ====== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL    URAM   CP(ns)
====== ====== ====== ===== ====== ====== ====== ========

====== ====== ====== ===== ====== ====== ====== ========

CFB128-AES256 encryption
------------------------

====== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
====== ====== ====== ===== ====== ===== ====== ========

====== ====== ====== ===== ====== ===== ====== ========

CFB128-AES256 decryption
------------------------

====== ====== ====== ===== ====== ====== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL    URAM   CP(ns)
====== ====== ====== ===== ====== ====== ====== ========

====== ====== ====== ===== ====== ====== ====== ========

