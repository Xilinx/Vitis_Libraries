===============================
Xilinx PairHMM Algorithm 64PE
===============================

PairHMM examples resides in ``L2/tests/pairhmm_8x8`` directory.

Follow build instructions to build host executable and directory.

THe binary host file generated is named as "**xil_pairhmm**" and is present in ``./build`` directory.

Executable Usage
-----------------

To execute Single PairHMM kernel :      ``./build/xil_pairhmm ./build/xclbin_<xsa_name>_<TARGET mode>/pairhmm.xclbin --syn <number of tests>``




**Software Emulation:** make run TARGET=sw_emu 

**Hardware Emulation:** make run TARGET=hw_emu

**Hardware:** make all TARGET=hw 

**Execution:** ./xil_pairhmm ./pairhmm.xclbin --syn <number of tests> 


Resources  
---------

**Design:** PairHMM Algorithm 64PE

**Board:** Versal vck190 board.

============= ======= ====== ====== ======= ========
Flow           LUT     BRAM   URAM    DSP     Fmax
============= ======= ====== ====== ======= ========
pairhmm 8x8    154K     95     56     588    156MHz
============= ======= ====== ====== ======= ========

Results
-------

====================== =========================
Topic                         Results
====================== =========================
Performance                   9GCups
====================== =========================
