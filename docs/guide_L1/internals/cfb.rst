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
CFB Mode
*************************

.. toctree::
   :maxdepth: 1

Overview
========

The Cipher Feedback (CFB) mode is a typical encryption and decryption mode using AES algorithm.
In this version, we provide AES-256 processing ability, which means the cipherkey in this design should be 256 bits.
Another limitation is that our working mode works on units of a fixed size (128 bits for 1 block), but text in the real world has a variety of lengths.
So, the final block of the text must be padded to 128 bits before encryption or decryption. 

Implementation on FPGA
======================

We support three different modes in this implementation: CFB1, CFB8, and CFB128.
The length of the text to be processed in one iteration corresponding to specific mode.
CFB1 is 1 bit per iteration, CFB8 is 8 bits per iteration, and CFB128 is 128 bits per iteration.
Notice that the plaintext, ciphertext, and initialization vector of each mode should also be 128 bits.
The algorithm flow chart is shown as follow, and the length of the text is denoted as 's':

.. image:: /images/CFB_working_mode.png
   :alt: algorithm flow chart of CFB
   :width: 100%
   :align: center

As we can see from the chart, the encryption part of each CFB mode has dependencies, so the input block of each iteration (except for iteration 0) needs a feedback data from its last iteration.
As the one-word AES encryption module needs 20 cycles to process one text block, the initiation interval (II) of each CFB encryption mode cannot achieve an II = 1.
However, the decryption part of each CFB mode has no dependencies, so it can achieve an II = 1.

CFB1 Encryption Performance(Device: VU9P)
==========================================

====== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
====== ====== ====== ===== ====== ===== ====== ========
 851    3336   4730    0    123    368    0     2.812
====== ====== ====== ===== ====== ===== ====== ========

CFB1 Decryption Performance(Device: VU9P)
==========================================

====== ====== ====== ===== ====== ====== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL    URAM   CP(ns)
====== ====== ====== ===== ====== ====== ====== ========
 804    3315   4635    0    123    378     0     2.830
====== ====== ====== ===== ====== ====== ====== ========


CFB8 Encryption Performance(Device: VU9P)
==========================================

====== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
====== ====== ====== ===== ====== ===== ====== ========
 922    3798   4797    0    123    368    0     2.889
====== ====== ====== ===== ====== ===== ====== ========

CFB8 Decryption Performance(Device: VU9P)
==========================================

====== ====== ====== ===== ====== ====== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL    URAM   CP(ns)
====== ====== ====== ===== ====== ====== ====== ========
 995    3315   4653    0    123    382     0     2.948
====== ====== ====== ===== ====== ====== ====== ========


CFB128 Encryption Performance(Device: VU9P)
============================================

====== ====== ====== ===== ====== ===== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
====== ====== ====== ===== ====== ===== ====== ========
 971    3436   4881    0    138    416    0     2.810
====== ====== ====== ===== ====== ===== ====== ========

CFB128 Decryption Performance(Device: VU9P)
============================================

====== ====== ====== ===== ====== ====== ====== ========
 CLB    LUT     FF    DSP   BRAM   SRL    URAM   CP(ns)
====== ====== ====== ===== ====== ====== ====== ========
 1051   3583   5012    0    138    545     0     2.737
====== ====== ====== ===== ====== ====== ====== ========
