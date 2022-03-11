================================
SMEM (supermaximal exact match)
================================ 

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

Note: 
Software Emulation not supported.

Tool
-----
Xilinx Vitis 2021.2


Resources and Results
----------------------

SMEM example resides in ``L2/tests/smem`` directory. 

Follow build instructions to generate host executable and binary.

The binary host file generated is named as "**xil_smem**" and it is present in ``./build`` directory.

Executable Usage
----------------

To execute file	: ``./build/xil_smem  ./build/xclbin_<xsa_name>_<TARGET mode>/smem.xclbin  <file_name>``

         
Results
-------

Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~

Table below presents resource utilization of Xilinx SMEM Kernels
The final Fmax achieved is 300MHz .

======= ===== ====== ===== ===== ===== 
Flow    LUT   LUTMem REG   BRAM  URAM 
======= ===== ====== ===== ===== ===== 
SMEM    46K     5K   60K    70    70    
======= ===== ====== ===== ===== ===== 

Performance Data
~~~~~~~~~~~~~~~~

Table below presents kernel throughput achieved for a single compute
unit. 

=========================== =========================
Topic                         Results
=========================== =========================
Average DRAM Bandwidth         2 GB/s
=========================== =========================

Standard SMEM Support
---------------------

This application is compatible with standard SMEM Genomics application.
