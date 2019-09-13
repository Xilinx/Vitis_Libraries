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

*******************************
CBC Mode
*******************************

.. toctree::
   :maxdepth: 1

Overview
========

The Cipher Block Chaining (CBC) mode is a typical encryption and decryption mode using AES algorithm.
In this version, we provide AES-256 processing ability, which means the cipherkey in this design should be 256 bits.
Another limitation is that our working mode works on units of a fixed size (128 bits for 1 block), but text in the real world has a variety of lengths.
So, the final block of the text must be padded to 128 bits before encryption or decryption. 

Implementation on FPGA
======================

We support CBC-AES256 mode in this implementation.
Notice that the plaintext, ciphertext, and initialization vector should be 128 bits.
The algorithm flow chart is shown as follow:

.. image:: /images/CBC_working_mode.png
   :alt: algorithm flow chart of CBC
   :width: 100%
   :align: center

As we can see from the chart, the encryption part of CBC mode has dependencies, so the input block of each iteration (except for iteration 0) needs a feedback data from its last iteration.
As the one-word AES encryption module needs 20 cycles to process one text block, the initiation interval (II) of CBC encryption mode cannot achieve an II = 1.
However, the decryption part of CBC mode has no dependencies, so it can achieve an II = 1.

Encryption Performance(Device: VU9P)
=====================================

====== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
====== ====== ====== ===== ====== ===== ====== ========
 1019   3579   4880    0    138    416    0     2.912
====== ====== ====== ===== ====== ===== ====== ========

Decryption Performance(Device: VU9P)
=====================================

====== ====== ====== ===== ====== ====== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL    URAM   CP(ns)
====== ====== ====== ===== ====== ====== ====== ========
 2268   7156   8393    0    532    2338    0     3.170
====== ====== ====== ===== ====== ====== ====== ========

