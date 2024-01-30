.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

=========================================
AMD ZSTD Compression
=========================================

The ZSTD example resides in the ``L2/tests/zstd_quadcore_compress`` directory. 

Follow the build instructions to generate the host executable and binary.

The binary host file generated is named "**xil_zstd**", and it is present in the ``./build`` directory.

Results
-------

Overall Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following table presents the resource utilization of an AMD Zstd decompress kernel supporting a Window Size of 32 KB. The final Fmax achieved is 284 MHz.

========== ===== ====== ===== 
Flow       LUT   BRAM   URAM  
========== ===== ====== ===== 
Compress   40K   79     37   
========== ===== ====== ===== 


Performance Data
~~~~~~~~~~~~~~~~

============================  ===========================
 Topic                          Results       
============================  ===========================
Compression                     1.17 Gb/s                
Average Compression Ratio	    2.68x (Silesia Benchmark)
============================  ===========================

Executable Usage:

1. To execute asingle file for decompression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_zstd -xbin ./build_dir.<TARGET mode>.<xsa_name>/xilZstdCompressStream.xclbin -c <compressed file_name>``
2. To decompress multiple files: ``./build_dir.<TARGET mode>.<xsa_name>/xil_zstd -xbin ./build_dir.<TARGET mode>.<xsa_name>/xilZstdCompressStream.xclbin -cfl <files.list>``

	- ``<files.list>``: Contains various file names with the current path.

The usage of the generated executable is as follows:

.. code-block:: bash
 
   Usage: application.exe -[-h-d-sx-l]
          --help,               -h      Print Help Options   Default: [false]
          --Compress,           -c      Compress
          --Compress_xclbin,    -cx     Compress XCLBIN
          --file_list,          -cfl    List of Input Files

