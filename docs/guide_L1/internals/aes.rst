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

*****************************
AES-256 Encryption Algorithms
*****************************

.. toctree::
   :maxdepth: 1

AES-256 is one of AES Algorithms which processes plain data blocks of 128 bits,
generates cipher data blocks of 128 bits using cipher keys of 256 bits.
Basic unit of AES algorithms operation is a two dimensional array of 16 bytes called states.
Its mapping relation is as illustrated in the figure below.

.. image:: /images/state_of_bytes.png
   :alt: state of bytes
   :width: 100%
   :align: center

Original Implementation
=======================

AES-256 consist of 5 part: KeyExpansion, SubBytes, ShiftRows, MixColumns and AddRoundKey.

KeyExpansion generates 15 round keys from original cipher key and they maps to 2-D array as states do.

AES-256 first does XOR add to input plain data blocks with first roundkey.
Then AES-256 performs 14 round of processing with the last 14 round keys, each at a time.
Each round sequentially does SubBytes, ShiftRows, MixColumns and AddRoundKey.

.. image:: /images/original_flow.png
   :alt: original flow
   :width: 100%
   :align: center

During SubBytes, each bytes of states is transformed by looking up table S-box using their value as address.

.. image:: /images/subbytes.png
   :alt: subbytes
   :width: 100%
   :align: center

During ShiftRows, last 3 rows of states are cyclically shifted over different number of bytes based on row number.

.. image:: /images/shiftrows.png
   :alt: shiftrows
   :width: 100%
   :align: center

During MixColumns, perform operation on each column of states.
Basicallly it's use matrix multiplication to transform each column of states.
Transform matrix is fixed and calculation treats each bytes as polynomials with coefficients in GF(2^8), modulo x^4 + 1.

.. image:: /images/mixcolumns.png
   :alt: mixcolumns
   :width: 100%
   :align: center

During AddRoundKey, states are XOR with roundkey of this round.

.. image:: /images/addroundkey.png
   :alt: addroundkey
   :width: 100%
   :align: center

Optimized Implementation on FPGA
=================================

Since each round of procee only needs one round key and KeyExpansion is independent of process,
we put KeyExpansion into each round to save an overhead.

Because SubBytes is independent of each byte's location in states and ShiftRows only shifts in integer of bytes, these two part could exchange their position in processing sequence without changing the result. Thoough no improvement is achieved here, this will benefit later optimizaiton.

.. image:: /images/new_flow.png
   :alt: new flow
   :width: 100%
   :align: center

The matrix multiplication in MixColumns is actually two parts: multiply bytes in column of states with bytes in row of the matrix, then add the result. Value of matrix elements could only be 01, 02 or 03. we can get the original result of SubBytes and its muplication of 01, 02 or 03 by one time table look up of a new S-Box called "sbox_mix_col_1". This could save a lot of logics to do multiplication.

.. image:: /images/sbox_mix_col_1.png
   :alt: merge SubByte with MixColumns
   :width: 100%
   :align: center

Based on similar consideration, we also merge such multiplication in KeyExpansion into one time table look up of another new S-Box called "sbox_Rcon". Although sbox_Rcon and sbox_mix_col_1 are bigger than original S-Box, they all could be stored in 1 BRAM on chip. Such merges saves logics without additional resource cost.

Performance(Device: VU9P)
=================================

==== ===== ====== ====== ===== ====== ===== ====== ========
 II   CLB   LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
==== ===== ====== ====== ===== ====== ===== ====== ========
 1    963   3422   4628    0    138    417    0     2.857
==== ===== ====== ====== ===== ====== ===== ====== ========
