===============================
SMEM (supermaximal exact match)
=============================== 

Description
-----------

This design contains hardware accelerated implementation of Super Maximal Exact
Match algorithm used in BWA-MEM. BWA-MEM does seeding and extension operations
to find mapping between read and reference sequences. Seeding operation (SMEM) of
BWA-MEM is critical and compute intensive. SMEM algorithm is used for seeding
operations in BWA-MEM. This demo presents accelerated version of SMEM using
Xilinx Alveo U250 discrete FPGA device. 

Usage
-----

1. Emulation Flows: make run TARGET=hw_emu
2. Hardware Build: make all TARGET=hw

Note: Software Emulation not supported.

Resources & Results
-------------------

**Tool:** Xilinx Vitis 2021.2

**FPGA Card:** Xilinx Alveo U250

**Design:** 4CU

========== ===== ===== ===== ====== ================= 
Flow       LUT   BRAM  URAM  Fmax   Performance (Avg) 
========== ===== ===== ===== ====== ================= 
SMEM       185K  820   280   291MHz 4.8GB/s           
========== ===== ===== ===== ====== ================= 

Custom Reference File
---------------------
At present this demos uses a fixed reference file as part of mapping.
