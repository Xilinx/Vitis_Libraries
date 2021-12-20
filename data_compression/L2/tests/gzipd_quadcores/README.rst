============================================
Xilinx Zlib Streaming Quadcore Decompression
============================================

Zlib example resides in ``L2/tests/zlibd`` directory. 

Follow build instructions to generate host executable and binary.

The binary host file generated is named as "**xil_zlib**" and it is present in ``./build`` directory.

Executable Usage
----------------

To execute single file for compression 	    : ``./build/xil_zlib ./build/xclbin_<xsa_name>_<TARGET mode>/compress.xclbin  <file_name>``

Results
-------

Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~

Table below presents resource utilization of Xilinx Zlib Decompress
kernels. The final Fmax achieved is 250MHz. 

========== ===== ====== ===== ===== ===== 
Flow       LUT   LUTMem REG   BRAM  URAM 
========== ===== ====== ===== ===== ===== 
Decompress 27.2K 3.1K   20.8K 32    8    
========== ===== ====== ===== ===== ===== 

Performance Data
~~~~~~~~~~~~~~~~

Table below presents kernel throughput achieved for a single compute
unit. 

============================= =========================
Topic                         Results
============================= =========================
Compression Throughput        2 GB/s
============================= =========================

Standard GZip Support
---------------------

This application is compatible with standard Gzip/Zlib application (compress/decompress).  
