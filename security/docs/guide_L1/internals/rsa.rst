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

****************
RSA Cryptography
****************

.. toctree::
   :maxdepth: 1

RSA is a public-key cryptosystem. Its encryption key is public and different from decryption key. 
RSA cryptosystem includes key generation, key distribution, encryption/decryption and padding schemes. In this release we provide the encryption/decryption part.

Implementation
==============

RSA key pair has 3 part: modulus :math:`n` , encryption key exponent :math:`e`, decryption key exponent :math:`d`. 
Plain text is an unsigned integer :math:`m`, not larger than modulus and Cipher text is also an unsigned integer :math:`c`.

In Encryption we have:

.. math::
   c = m^{e} \mod{n}

In Decryption we have:

.. math::
   m = c^{d} \mod{n}

Optimized Implementation on FPGA
=================================

We seperate encryption into two part: updateKey and process. Each time we got an message with a new RSA key, we have to call updateKey before get into encryption/decryption. If we process messages with the same key continuously, updateKey only need be call once at the beginning.

RSA encryption and decryption are basically the same calculation: big integer exponent modulus.
We first transform message to its Montgomery representation, then perform Montgomery modular exponentiation. 
At last we tranform it back to normal expression to get final result. 
By such implementation, we avoid all big integer division to get modulus, except for the one during transformation to Montegomery representation.

In updateKey, we calculate modular multiplicative inverse using extend Eculid Algorithms. Also we calculate the position of first non-zero big of exponent.Both calculation is to prepare for later process and save unnecessary calculations.

In process, we need to do a lot of big integer multiplication which will cost a lot of DSPs. 
Since All these multiplication are inter dependent, there're no way to parallelize. 
So most optimization focused on pipeline DSP inside multiplications.
We treat big integer, like a 2048 bits integer, as a BlockNum digits integer in radix BlockWidth.
We choose BlockNum and BlockWidth so that :math:`2048 = BlockNum * BlockWidth`.
A BlockWidth x BlockWdith multiplication is a basic operation, and a 2048 bits x 2048 bits multiplication could be done by BlockNum x BlockNum basic operations.
We utilize DSPs in a way that they could finish these basic operations independently.
In such way, we utilize fewer DSP and have better timing.

.. image:: /images/rsa_1.png
   :alt: Illustration of big integer multiplication.
   :width: 30%
   :align: center

Reference
========

Peter Montgomery. "Modular Multiplication Without Trial Division", Mathematics of Computation, vol. 44 no. 170, pp. 519–521, April 1985.

Knuth, Donald. The Art of Computer Programming. Addison-Wesley. Volume 2, Chapter 4.

RSA Performance(Device: U250)
=========================================

====== ======= ======= ===== ====== ===== ====== ========
 CLB     LUT     FF     DSP   BRAM   SRL   URAM   CP(ns)
====== ======= ======= ===== ====== ===== ====== ========
 3428   28668   94455   128    0     942    0     3.160
====== ======= ======= ===== ====== ===== ====== ========
