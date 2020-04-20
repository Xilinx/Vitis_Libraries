=========================================
Xilinx ZLIB Compression and Decompression
=========================================

ZLIB example resides in ``L2/demos/zlib`` directory. 

Follow build instructions to generate host executable and binary.

The binary host file generated is named as "**xil_zlib**" and it is present in ``./build`` directory.



Results
-------

Overall Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Table below presents resource utilization of Xilinx Zlib compress/decompress kernels. The final Fmax achieved is 291MHz 

========== ===== ====== ===== ===== ===== 
Flow       LUT   LUTMem REG   BRAM  URAM  
========== ===== ====== ===== ===== ===== 
Compress   110K  26K    117K  132   48    
---------- ----- ------ ----- ----- ----- 
DeCompress 13.8K 506    13.8K  8     4     
========== ===== ====== ===== ===== ===== 

Compress Kernel Resource Utilization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

======== ===== ======= ===== ===== ===== 
Kernel   LUT   LUTMem  REG   BRAM  URAM 
======== ===== ======= ===== ===== =====
LZ77     49.8K 12.7K   56.8K 74    48   
-------- ----- ------- ----- ----- -----
TreeGen  4.4K  140     6.4K  13    0   
-------- ----- ------- ----- ----- -----
Huffman  56.5K 13.5K   54.2K 45    0
======== ===== ======= ===== ===== =====

Decompress Kernel Resource Utilization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

=========== ===== ======= ===== ===== =====
Kernel      LUT   LUTMem  REG   BRAM  URAM
=========== ===== ======= ===== ===== =====
Decompress  10.6K 162     6.4K  4     4
----------- ----- ------- ----- ----- -----
DmReader    2.1K  294     3.8K  3     0
----------- ----- ------- ----- ----- -----
DmWriter    1K    50      1.8K  1     0
=========== ===== ======= ===== ===== =====


Performance Data
~~~~~~~~~~~~~~~~

+----------------------------+------------------------+
| Topic                      | Best Kernel Throughput |
+============================+========================+
| Compression                | 1.9 GB/s               |
+----------------------------+------------------------+
| Decompression              | 631 MB/s               |
+----------------------------+------------------------+
| Average Compression Ratio  | 2.74 (Silesia Corpus)  |
+----------------------------+------------------------+

Executable Usage:

1. To execute single file for compression 	          : ``./build/xil_zlib -sx ./build/xclbin_<xsa_name>_<TARGET mode>/compress_decompress.xclbin -c <input file_name>``
2. To execute single file for decompression           : ``./build/xil_zlib -sx ./build/xclbin_<xsa_name>_<TARGET mode>/compress_decompress.xclbin -d <compressed file_name>``
3. To validate single file (compress & decompress)    : ``./build/xil_zlib -sx ./build/xclbin_<xsa_name>_<TARGET mode>/compress_decompress.xclbin -v <input file_name>``
4. To validate multiple files (compress & decompress) : ``./build/xil_zlib -sx ./build/xclbin_<xsa_name>_<TARGET mode>/compress_decompress.xclbin -l <files.list>``

	- ``<files.list>``: Contains various file names with current path

The usage of the generated executable is as follows:

.. code-block:: bash
 
   Usage: application.exe -[-h-c-d-sx-v-l-k]
        --help,                 -h      Print Help Options   Default: [false]
        --compress,             -c      Compress
        --decompress,           -d      Decompress
        --single_xclbin,        -sx     Single XCLBIN        Default: [single]
        --file_list,            -l      List of Input Files
        --compress_decompress,  -v      Compress Decompress
        --cu,                   -k      CU                   Default: [0]

