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
Chacha20 Algorithms
*****************************

.. toctree::
   :maxdepth: 1

Chacha20 is a cipher stream. Its input includes a 256-bit key, a 64-bit counter, a 64-bit nonce and plain text.
Its initial state is a 4*4 matrix of 32-bit words. The first row is a constant("expand 32-byte k"); the second and the third are fill with 256-bit key; the first two words in last row are 64-bit counter and the last 2 words are 64-bit nonce. 
It generate 512-bit keystream in each iteration to encrypt a 512-bit bolck of plain text.
When the rest of plain text is less 512-bit after encryption many times, some bits in the last 512-bit keystream will not be used.
Its encryption and decryption are same as long as input same initial key, counter and nonce.


Implementation
=======================


Chacha20 consists of 4 parts: generateBlock, packMsg, chachaEncrypt, convert2Byte.

GenerateBlock is responsible for initialization innner state.
packMsg packs each 64 bytes from plain text to a 512-bit block used to encprypt.
chachaEncrypt generates a 512-bit key which is operated XOR with 512-bit plain text then output a cipher block in each iteration.
covert2Byte converts cipher block to many bytes. It will remove meaningless bytes because packMsg maybe bring superfluous bytes in last block. 


Performance(Device: U250)
=================================

==== ===== ====== ====== ===== ====== ===== ====== ========
 II   CLB   LUT     FF    DSP   BRAM   SRL   URAM   CP(ns)
==== ===== ====== ====== ===== ====== ===== ====== ========
 4    1103  5082   8134    0     45    18     0     2.852
==== ===== ====== ====== ===== ====== ===== ====== ========
