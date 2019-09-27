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
 286MHz      203,595(11.8%)   312,900(9.1%)    761(28.3.0%)    0      29(0.3%)    4.7GB/s
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

To profile performance of hmacSha1, we prepare a datapack of 24 messages, each message is 2Mbyte.
We have 4 kernels, each kernel has 8 PUs.
Kernel utilization and throughput is shown in table below. 

=========== ================ ================== ============== ======= ========== =============
 Frequency        LUT                REG             BRAM       URAM       DSP     Throughput
=========== ================ ================== ============== ======= ========== =============
 281MHz      648,274(37.5%)   1,074,803(31.1%)   726 (27.0%)    0      56(0.5%)    2.1GB/s
=========== ================ ================== ============== ======= ========== =============

