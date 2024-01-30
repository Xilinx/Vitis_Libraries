.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

========================================
AMD Zlib Streaming Static Compression
========================================

The Zlib example resides in the ``L2/tests/zlibc_static`` directory. 

Follow the build instructions to generate the host executable and binary.

The binary host file generated is named "**xil_zlib**", and it is present in the ``./build`` directory.

Executable Usage
----------------

1. To execute a single file for compression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_zlib -xbin ./build_dir.<TARGET mode>.<xsa_name>/compress.xclbin -c <file_name> -zlib 1``
2. To execute multiple files for compression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_zlib -xbin ./build_dir.<TARGET mode>.<xsa_name>/compress.xclbin -cfl <files.list> -zlib 1``

	- ``<files.list>``: Contains various file names with the current path.

The usage of the generated executable is as follows:

.. code-block:: bash
 
   Usage: application.exe -[-h-c-l-xbin-B]
          --help,           -h      Print Help Options
          --xclbin,         -xbin   XCLBIN                                               Default: [compress]
          --compress,       -c      Compress
          --file_list,      -cfl    Compress List of Input Files
          --max_cr,         -mcr    Maximum CR    
          --device_id,      -id     Device ID                                       Default: [0]
          --zlib,           -zlib   [0:GZip, 1:Zlib]                                Default: [0]

Results
-------

Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~

The following table presents the resource utilization of Zlib Compress/Decompress kernels. The final FMax achieved is 300 MHz.

========== ===== ====== ===== ===== ===== 
Flow       LUT   LUTMem REG   BRAM  URAM 
========== ===== ====== ===== ===== ===== 
Compress   35.7K  2.3K  36.5K  39    64    
========== ===== ====== ===== ===== ===== 

Performance Data
~~~~~~~~~~~~~~~~

The following table presents the kernel throughput achieved for a single compute unit. 

============================= =========================
Topic                         Results
============================= =========================
Compression Throughput        2 Gb/s
Average Compression Ratio     2.31x (Silesia Benchmark)
============================= =========================

Standard GZip Support
---------------------

This application is compatible with the standard Gzip/Zlib application (compress/decompress).  
