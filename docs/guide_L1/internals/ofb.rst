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

*************************
OFB Mode
*************************

.. toctree::
   :maxdepth: 1

Overview
========

The Output Feedback (OFB) mode is a typical encryption and decryption mode using AES algorithm.
In this version, we provide AES-256 processing ability, which means the cipherkey in this design should be 256 bits.
Another limitation is that our working mode works on units of a fixed size (128 bits for 1 block), but text in the real world has a variety of lengths.
So, the final block of the text must be padded to 128 bits before encryption or decryption. 

Implementation on FPGA
======================

We support OFB-AES256 mode in this implementation.
Notice that the plaintext, ciphertext, and initialization vector should be 128 bits.
The algorithm flow chart is shown as follow:

.. image:: /images/OFB_working_mode.png
   :alt: algorithm flow chart of OFB
   :width: 100%
   :align: center

As we can see from the chart, both encryption and decryption part of OFB mode has dependencies,
so the input block of each iteration (except for iteration 0) needs a feedback data from its last iteration.
As the one-word AES encryption module needs 20 cycles to process one text block, the initiation interval (II) of OFB mode cannot achieve an II = 1.

Encryption Performance(Device: VU9P)
============================================

====== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
====== ====== ====== ===== ====== ===== ====== ========
 1087   4385   4880    0    138    416    0     2.888
====== ====== ====== ===== ====== ===== ====== ========

Decryption Performance(Device: VU9P)
============================================

===== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT    FF    DSP   BRAM   SRL   URAM   CP(ns)
===== ====== ====== ===== ====== ===== ====== ========
 980   3600   4880    0    138    416    0     2.949
===== ====== ====== ===== ====== ===== ====== ========
