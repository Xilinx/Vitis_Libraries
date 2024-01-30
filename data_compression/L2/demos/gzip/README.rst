.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

=========================================
AMD GZip Compression and Decompression
=========================================

GZip example resides in the ``L2/demos/gzip`` directory. 

Follow the build instructions to generate the host executable and binary.

The binary host file generated is named as "**xil_gzip**", and it is present in the ``./build`` directory.

Executable Usage
----------------

1. To execute single file for compression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_gzip -xbin ./build_dir.<TARGET mode>.<xsa_name>/compress_decompress.xclbin -c <input file_name>``
2. To execute single file for decompression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_gzip -xbin ./build_dir.<TARGET mode>.<xsa_name>/compress_decompress.xclbin -d <compressed file_name>``
3. To validate single file (compress & decompress): ``./build_dir.<TARGET mode>.<xsa_name>/xil_gzip -xbin ./build_dir.<TARGET mode>.<xsa_name>/compress_decompress.xclbin -t <input file_name>``
4. To execute multiple files for compression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_gzip -xbin ./build_dir.<TARGET mode>.<xsa_name>/compress_decompress.xclbin -cfl <files.list>``
5. To execute multiple files for decompression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_gzip -xbin ./build_dir.<TARGET mode>.<xsa_name>/compress_decompress.xclbin -dfl <compressed files.list>``
6. To validate multiple files (compress & decompress): ``./build_dir.<TARGET mode>.<xsa_name>/xil_gzip -xbin ./build_dir.<TARGET mode>.<xsa_name>/compress_decompress.xclbin -l <files.list>``

	- ``<files.list>``: Contains various file names with the current path.

The usage of the generated executable is as follows:

.. code-block:: bash
 
   Usage: application.exe -[-h-c-d-t-cfl-dfl-l-xbin-id]
          --help,                -h        Print Help Options
          --compress,            -c        Compress
          --decompress,          -d        Decompress
          --test,                -t        Compress & Decompress
          --compress_list,       -cfl      Compress List of Input Files
          --decompress_list,     -dfl      Decompress List of compressed Input Files
          --test_list,           -l        Compress & Decompress on Input Files
          --max_cr,              -mcr      Maximum CR                                      Default: [10]
          --xclbin,              -xbin     XCLBIN
          --device_id,           -id       Device ID                                       Default: [0]
          --zlib,                -zlib     [0:GZip, 1:Zlib]                                Default: [0]
          
Results
-------

Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~

The following table presents the resource utilization of AMD GZip Compress/Decompress kernels. The final Fmax achieved is 300 MHz. 

Compression
~~~~~~~~~~~

========== ===== ====== ===== ===== ===== 
Flow       LUT   LUTMem REG   BRAM  URAM 
========== ===== ====== ===== ===== ===== 
Compress   35.4K  4.6K  31.8K  73    32    
========== ===== ====== ===== ===== ===== 

Decompression
~~~~~~~~~~~~~

========== ===== ====== ===== ===== ===== 
Flow       LUT   LUTMem REG   BRAM  URAM 
========== ===== ====== ===== ===== ===== 
DeCompress 6.7K  866    5K    8     0    
---------- ----- ------ ----- ----- -----
DM Reader  1.5K  281    1.9K  2     0
---------- ----- ------ ----- ----- -----
DM Writer  4K    1681   6K    9     0
========== ===== ====== ===== ===== ===== 

Performance Data
~~~~~~~~~~~~~~~~

The following table presents the kernel throughput achieved for a single compute unit. 

============================= =========================
Topic                         Results
============================= =========================
Compression Throughput        1.5 GB/s
Decompression Throughput      518 MB/s
Average Compression Ratio     2.67x (Silesia Benchmark)
============================= =========================

.. note:: The overall throughput can still be increased with multiple compute.


Standard GZip Support
---------------------

This application is compatible with the standard Gzip/Zlib application (compress/decompress).  
