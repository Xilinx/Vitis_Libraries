=========================================
Xilinx ZSTD Decompression
=========================================

ZSTD example resides in ``L2/demos/zstd_decompress`` directory. 

Follow build instructions to generate host executable and binary.

The binary host file generated is named as "**xil_zstd**" and it is present in ``./build`` directory.



Results
-------

Overall Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Table below presents resource utilization of Xilinx Zstd decompress kernel (excluding data movers) supporting Window Size of 32KB. The final Fmax achieved is 232MHz.

========== ===== ====== ===== ===== ===== 
Flow       LUT   LUTMem REG   BRAM  URAM  
========== ===== ====== ===== ===== ===== 
DeCompress 18.8K 153    17.2K 52    4     
========== ===== ====== ===== ===== ===== 


Decompress Kernel Resource Utilization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

=========== ===== ======= ===== ===== =====
Kernel      LUT   LUTMem  REG   BRAM  URAM
=========== ===== ======= ===== ===== =====
Decompress  22.3K 153     8.1K  52    4
----------- ----- ------- ----- ----- -----
DmReader    3.2K  717     3.8K  10    0
----------- ----- ------- ----- ----- -----
DmWriter    2.6K  281     1.8K  15    0
=========== ===== ======= ===== ===== =====


Performance Data
~~~~~~~~~~~~~~~~

+----------------------------+------------------------+
| Topic                      | Best Kernel Throughput |
+============================+========================+
| Decompression              | 783 MB/s               |
+----------------------------+------------------------+
| Average Compression Ratio  | 2.78 (Silesia Corpus)  |
+----------------------------+------------------------+

.. [*] Decompression uses feasibile options (Bitwidth: 32bit, Window Size: 32KB) 

Executable Usage:

1. To execute single file for decompression           : ``./build/xil_zlib -dx ./build/xclbin_<xsa_name>_<TARGET mode>/compress_decompress.xclbin -d <compressed file_name>``
2. To decompress multiple files                       : ``./build/xil_zlib -dx ./build/xclbin_<xsa_name>_<TARGET mode>/compress_decompress.xclbin -l <files.list>``

	- ``<files.list>``: Contains various file names with current path

The usage of the generated executable is as follows:

.. code-block:: bash
 
   Usage: application.exe -[-h-d-sx-l]
        --help,                 -h      Print Help Options   Default: [false]
        --decompress,           -d      Decompress
        --decompress_xclbin,    -dx     Decompress XCLBIN
        --file_list,            -l      List of Input Files

