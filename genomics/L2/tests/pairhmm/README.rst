=========================
Xilinx PairHMM Algorithm
=========================

PairHMM examples resides in ``L2/tests/pairhmm`` directory.

Follow build instructions to build host executable and directory.

THe binary host file generated is named as "**xil_pairhmm**" and is present in ``./build`` directory.

Executable Usage
-----------------

To execute Single PairHMM kernel :      ``./build/xil_pairhmm ./build/xclbin_<xsa_name>_<TARGET mode>/pairhmm.xclbin``




**Software Emulation:** make run TARGET=sw_emu 

**Hardware Emulation:** make run TARGET=hw_emu

**Hardware:** make all TARGET=hw 

**Execution:** ./xil_pairhmm ./pairhmm.xclbin --syn <number of tests> 


Resources  
---------

**Design:** PairHMM Algorithm

**Board:** Alveo U200 board.

========== ====== ====== ====== ======= ========
Flow        LUT    BRAM   URAM   DSP    Fmax
========== ====== ====== ====== ======= ========
pairhmm     253K    95     56    2004     175MHz
========== ====== ====== ====== ======= ========


Results
-------

====================== =========================
Topic                         Results
====================== =========================
Performance                   11GCUPS
====================== =========================
