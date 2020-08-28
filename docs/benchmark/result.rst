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

.. meta::
   :keywords: Vitis, Security, Library, Vitis Security design, benchmark, result
   :description: Vitis Security Library benchmark results.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


.. result:

*****************
Benchmark Result
*****************


===========================
aes256CbcDecrypt throughput
===========================

To profile performance of aes256CbcDecrypt, we prepare a datapack of 32K messages, each message is 1Kbyte. 
We have 1 kernels, each kernel has 4 PUs.
Kernel utilization and throughput is shown in table below. 

=========== ================ ================ ============== ======= ========== =============
 Frequency        LUT               REG            BRAM       URAM       DSP     Throughput
=========== ================ ================ ============== ======= ========== =============
 286MHz      203,595(11.8%)   312,900(9.1%)     761(28.3%)     0      29(0.3%)    4.7GB/s
=========== ================ ================ ============== ======= ========== =============



===========================
aes256CbcEncrypt throughput
===========================

To profile performance of aes256CbcEncrypt, we prepare a datapack of 96K messages, each message is 1Kbyte. 
We have 4 kernels, each kernel has 12 PUs.
Kernel utilization and throughput is shown in table below. 

=========== ================ ================ ============== ======= ========== =============
 Frequency        LUT               REG            BRAM       URAM       DSP     Throughput
=========== ================ ================ ============== ======= ========== =============
 224MHz     1,059,093(61.3%) 1,010,145(34.9%)  654(24.3%)       0     152(1.3%)    5.5GB/s
=========== ================ ================ ============== ======= ========== =============



==============
rc4 throughput
==============

To profile performance of rc4, we prepare a datapack of 24 messages, each message is 2Mbyte.
We have 4 kernels, each kernel has 12 PUs.
Kernel utilization and throughput is shown in table below. 

=========== ================ ================ ============== ======= ========== =============
 Frequency        LUT               REG            BRAM       URAM       DSP     Throughput
=========== ================ ================ ============== ======= ========== =============
 147MHz     1,126,259(73.0%) 1,120,505(34.9%)   640 (44.0%)    0     216(1.8%)    3.0GB/s
=========== ================ ================ ============== ======= ========== =============



===================
hmacSha1 throughput
===================

To profile performance of hmacSha1, we prepare a datapack of 512K messages, each message is 1Kbyte,
key length is 256bits. We have 4 kernels, each kernel has 16 PUs.
Kernel utilization and throughput is shown in table below. 

=========== ================ ================== ============== ======= ========== =============
 Frequency        LUT                REG             BRAM       URAM       DSP     Throughput
=========== ================ ================== ============== ======= ========== =============
 227 MHz     959,078(55.5%)   1,794,522(52.0%)   777 (28.9%)     0      56(0.5%)    8.0 GB/s
=========== ================ ================== ============== ======= ========== =============


===================
crc32 throughput
===================

To profile performance of crc32, we prepare a datapack of 268,435,456 byte messages as kernel input. 
Base on U50, We have 1 kernel, each kernel has 1 PU.
Kernel utilization and throughput is shown in table below. 

=========== ================ ================== ============== ======= ========== =============
 Frequency        LUT                REG             BRAM       URAM       DSP     Throughput
=========== ================ ================== ============== ======= ========== =============
 300 MHz      5,322(0.69%)      10,547(0.65%)     16 (1.37%)      0        0        4.7 GB/s
=========== ================ ================== ============== ======= ========== =============


===================
adler32 throughput
===================

To profile performance of adler32, we prepare a datapack of 268,435,456 byte messages as kernel input. 
Base on U50, We have 1 kernel, each kernel has 1 PU.
Kernel utilization and throughput is shown in table below. 

=========== ================ ================== ============== ======= ========== =============
 Frequency        LUT                REG             BRAM       URAM       DSP     Throughput
=========== ================ ================== ============== ======= ========== =============
 262 MHz      6,348(0.83%)      12,232(0.76%)     16 (1.37%)      0        0        4.1 GB/s
=========== ================ ================== ============== ======= ========== =============

