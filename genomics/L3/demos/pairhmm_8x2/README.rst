=======
PairHMM
=======

This test case consists of multi kernel (3CU) pairhmm implementation.
The host and device communication is overlapped.

Usage
-----

**Software Emulation:** make run TARGET=sw_emu
 
**Hardware Emulation:** make run TARGET=hw_emu

**Hardware:** make all TARGET=hw 

**Execution:** ./build/xil_pairhmm ./build/xclbin_<xsa_name>_<TARGET mode>/pairhmm.xclbin --syn <number of tests> 


Resources
---------

**Design:** 3 CUs / 192PEs

**Board:** Alveo U200

========== ===== ===== ===== ====== ======
Flow       LUT   BRAM  URAM  DSP    Fmax
========== ===== ===== ===== ====== ======
pairHMM    759K  285   178   6008   175MHz
========== ===== ===== ===== ====== ======


## Results

============================= =========================
Topic                         Results
============================= =========================
Average GCUPs                 33 
============================= =========================
