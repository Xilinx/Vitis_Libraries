=========================================
Xilinx GZip Compression and Decompression
=========================================

GZip example resides in ``L2/demos/gzip`` directory. 

Follow build instructions to generate host executable and binary.

The binary host file generated is named as "**xil_gzip**" and it is present in ``./build`` directory.

Executable Usage
----------------

1. To execute single file for compression 	          : ``./build/xil_gzip -xbin ./build/xclbin_<xsa_name>_<TARGET mode>/compress_decompress.xclbin -c <input file_name>``
2. To execute single file for decompression           : ``./build/xil_gzip -xbin ./build/xclbin_<xsa_name>_<TARGET mode>/compress_decompress.xclbin -d <compressed file_name>``
3. To validate single file (compress & decompress)    : ``./build/xil_gzip -xbin ./build/xclbin_<xsa_name>_<TARGET mode>/compress_decompress.xclbin -t <input file_name>``
4. To execute multiple files for compression    : ``./build/xil_gzip -xbin ./build/xclbin_<xsa_name>_<TARGET mode>/compress_decompress.xclbin -cfl <files.list>``
5. To execute multiple files for decompression    : ``./build/xil_gzip -xbin ./build/xclbin_<xsa_name>_<TARGET mode>/compress_decompress.xclbin -dfl <compressed files.list>``
6. To validate multiple files (compress & decompress) : ``./build/xil_gzip -xbin ./build/xclbin_<xsa_name>_<TARGET mode>/compress_decompress.xclbin -l <files.list>``

	- ``<files.list>``: Contains various file names with current path

The usage of the generated executable is as follows:

.. code-block:: bash
 
   Usage: application.exe -[-h-c-d-t-cfl-dfl-l-xbin-id]
          --help,                -h        Print Help Options
          --compress,            -c        Compress
          --decompress,          -d        Decompress
          --test,                -t        Xilinx compress & Decompress
          --compress_list,       -cfl      Compress List of Input Files
          --decompress_list,     -dfl      Decompress List of compressed Input Files
          --test_list,           -l        Xilinx Compress & Decompress on Input Files
          --max_cr,              -mcr      Maximum CR                                      Default: [10]
          --xclbin,              -xbin     XCLBIN
          --device_id,           -id       Device ID                                       Default: [0]
          --zlib,                -zlib     [0:GZip, 1:Zlib]                                Default: [0]
          
Results
-------

Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~

Table below presents resource utilization of Xilinx GZip Compress/Decompress
kernels. The final Fmax achieved is 236MHz 

Compression
~~~~~~~~~~~

========== ===== ====== ===== ===== ===== 
Flow       LUT   LUTMem REG   BRAM  URAM 
========== ===== ====== ===== ===== ===== 
Compress   53.8K 3.8K   55.4K 75    72    
========== ===== ====== ===== ===== ===== 

Decompression
~~~~~~~~~~~~~

========== ===== ====== ===== ===== ===== 
Flow       LUT   LUTMem REG   BRAM  URAM 
========== ===== ====== ===== ===== ===== 
DeCompress 11.3K 113    7K    6     3    
---------- ----- ------ ----- ----- -----
DM Reader  1.5K  282    2.1K  2     0
---------- ----- ------ ----- ----- -----
DM Writer  2.7K  624    4K    2     0
========== ===== ====== ===== ===== ===== 

Performance Data
~~~~~~~~~~~~~~~~

Table below presents kernel throughput achieved for a single compute
unit. 

============================= =========================
Topic                         Results
============================= =========================
Compression Throughput        1.5 GB/s
Decompression Throughput      423 MB/s
Average Compression Ratio     2.67x (Silesia Benchmark)
============================= =========================

Note: Overall throughput can still be increased with multiple compute


Standard GZip Support
---------------------

This application is compatible with standard Gzip/Zlib application (compress/decompres).  
