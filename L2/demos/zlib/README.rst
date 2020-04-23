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

Table below presents resource utilization of Xilinx Zlib compress/decompress kernels. The final Fmax achieved is 284MHz 

========== ===== ====== ===== ===== ===== 
Flow       LUT   LUTMem REG   BRAM  URAM  
========== ===== ====== ===== ===== ===== 
Compress   82.6K 26K    84.7K  105   48    
---------- ----- ------ ----- ----- ----- 
DeCompress 13.8K 506    13.7K  8     4     
========== ===== ====== ===== ===== ===== 

Compress Kernel Resource Utilization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

======== ===== ======= ===== ===== ===== 
Kernel   LUT   LUTMem  REG   BRAM  URAM 
======== ===== ======= ===== ===== =====
LZ77     43.8K 10.4K   46.9K 69    48   
-------- ----- ------- ----- ----- -----
Huffman  38.8K 10.2K   37.8K 36    0
======== ===== ======= ===== ===== =====

Decompress Kernel Resource Utilization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

=========== ===== ======= ===== ===== =====
Kernel      LUT   LUTMem  REG   BRAM  URAM
=========== ===== ======= ===== ===== =====
Decompress  10.7K 162     8.1K  4     4
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
| Compression                | 2 GB/s                 |
+----------------------------+------------------------+
| Decompression              | 733 MB/s               |
+----------------------------+------------------------+
| Average Compression Ratio  | 2.74 (Silesia Corpus)  |
+----------------------------+------------------------+

.. [*] Decompression uses high fmax options (Bitwidth: 32bit, ByteGen Loop: 2) 

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

