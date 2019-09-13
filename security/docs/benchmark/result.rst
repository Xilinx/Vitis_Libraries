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

.. result:

*******************************************************
Benchmark of AES256 Encryption/Decryption in CBC Mode
*******************************************************

Overview
=========

This is a benchmark of aes256CbcEncrypt and aes256CbcDecrypt function using the SDx environment.
The underlying device is Alveo U250.


Highlights
==========

The performance of aes256CbcEncrypt and aes256CbcDecrypt is shown in the table blow.


===========================
aes256CbcDecrypt throughput
===========================

To profile performance of aes256CbcDecrypt, we prepare a datapack of 32K messages, each message is 1Kbyte. 
All these are decrypted in using AES256 algorithm in CBC mode, each message uses a different key.
We test aes256CbCDecrypt kernel with 10 datapack pipeline, its End to End performance is 590MB/s.
Kernel utilization is shown in table below. 

=========== ================ ================ ============== ======= ========== ============
 Frequency        LUT               REG            BRAM       URAM       DSP     Throughput
=========== ================ ================ ============== ======= ========== ============
 218MHz      245,131(10.6%)   326,430(10.2%)   1,350(56.3%)    0      29(0.2%)    590MB/s
=========== ================ ================ ============== ======= ========== ============


===========================
aes256CbcEncrypt throughput
===========================

To profile performance of aes256CbcEncrypt, we prepare a datapack of 48K messages, each message is 1Kbyte. 
All these are encrypted in using AES256 algorithm in CBC mode, each message uses a different key.
We test aes256CbCDecrypt kernel with 10 datapack pipeline, its End to End performance is 708MB/s.
Kernel utilization is shown in table below. 


=========== ================ ================ ============== ======= ========== =============
 Frequency        LUT               REG            BRAM       URAM       DSP     Throughtput
=========== ================ ================ ============== ======= ========== =============
 218MHz      288,096(12.4%)   358,507(11.2%)   1,056(44.0%)    0      44(0.4%)    780MB/s
=========== ================ ================ ============== ======= ========== =============


.. Notice:: This benchmark is not finished, yet. It will update lately.

