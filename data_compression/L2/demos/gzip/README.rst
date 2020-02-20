=========================================
Xilinx GZip Compression and Decompression
=========================================

GZip example resides in ``L2/demos/gzip`` directory. 

Follow build instructions to generate host executable and binary.

The binary host file generated is named as "**xil_gzip**" and present in ``./build`` directory.

Results
-------

Overall Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Table below presents resource utilization of Xilinx GZip compress/decompress kernels 

========== ===== ====== ==== ===== ===== ======
Flow       LUT   LUTMem REG  BRAM  URAM  Fmax
========== ===== ====== ==== ===== ===== ======
Compress   134K  30K    134K 140   48    287MHz
---------- ----- ------ ---- ----- ----- ------
DeCompress 12.9K 626    19K  42    0     300MHz
========== ===== ====== ==== ===== ===== ======

Compress Kernel Resource Utilization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

======== ===== ======= ===== ===== ===== 
Kernel   LUT   LUTMem  REG   BRAM  URAM 
======== ===== ======= ===== ===== =====
LZ77     49.3K 14.8K   61.3K 74    48   
-------- ----- ------- ----- ----- -----
TreeGen  8.4K  544     9.5K  15    0   
-------- ----- ------- ----- ----- -----
Huffman  76.6K 15.2K   63.5K 53    0
======== ===== ======= ===== ===== =====

Decompress Kernel Resource Utilization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

=========== ===== ======= ===== ===== 
Kernel      LUT   LUTMem  REG   BRAM  
=========== ===== ======= ===== =====
Decompress  8.8K  99      7.1K  12     
----------- ----- ------- ----- -----
DmReader    1.3K  159     2.3K  1
----------- ----- ------- ----- ----- 
DmWriter    908   34      1.6K  1
=========== ===== ======= ===== =====

Performance Data
~~~~~~~~~~~~~~~~

+----------------------------+------------------------+
| Topic                      | Best Kernel Throughput |
+============================+========================+
| Compression                | 1.4 GB/s               |
+----------------------------+------------------------+
| Decompression              | 286 MB/s               |
+----------------------------+------------------------+
| Average Compression Ratio  | 2.74 (Silesia Corpus)  |
+----------------------------+------------------------+

Executable Usage:

1. To execute single file for compression 	          : ``./build/xil_gzip -sx ./build/xclbin_<xsa_name>_<TARGET mode>/compress_decompress.xclbin -c <input file_name>``
2. To execute single file for decompression           : ``./build/xil_gzip -sx ./build/xclbin_<xsa_name>_<TARGET mode>/compress_decompress.xclbin -d <compressed file_name>``
3. To validate single file (compress & decompress)    : ``./build/xil_gzip -sx ./build/xclbin_<xsa_name>_<TARGET mode>/compress_decompress.xclbin -v <input file_name>``
4. To validate multiple files (compress & decompress) : ``./build/xil_gzip -sx ./build/xclbin_<xsa_name>_<TARGET mode>/compress_decompress.xclbin -l <files.list>``

	- ``<files.list>``: Contains various file names with current path

Standard Compliance:

.. code-block:: bash

    1. Vitis Library GZip Compressed data can be decoded using standard with
       this command "gzip -dc <compressed_input.gz> > out". Generated output is
       original raw file.
    2. Vitis Library GZip decompression supports standard file format 
       when file is compressed using dynamic Huffman.

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
