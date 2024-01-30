.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

====================================
AMD ZLIB Streaming Decompression
====================================

The ZLIB example resides in the ``L2/tests/zlibd_quadcores_8KB`` directory. 

Follow the build instructions to generate the host executable and binary.

The binary host file generated is named "**xil_zlib**", and it is present in the ``./build`` directory.

Executable Usage
----------------

1. To execute a single file for compression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_zlib -xbin ./build_dir.<TARGET mode>.<xsa_name>/decompress.xclbin  -d <file_name>``
2. To execute multiple files for compression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_zlib -xbin ./build_dir.<TARGET mode>.<xsa_name>/decompress.xclbin  -dfl <files.list>``

	- ``<files.list>``: Contains various file names with the current path.

The usage of the generated executable is as follows:

.. code-block:: bash
 
   Usage: application.exe -[-h-d-dfl-xbin-B]
          --help,                -h        Print Help Options
          --decompress,          -d        Decompress
          --decompress_list,     -dfl      Decompress List of compressed Input Files
          --max_cr,              -mcr      Maximum CR                                      Default: [10]
          --xclbin,              -xbin     XCLBIN
          --device_id,           -id       Device ID                                       Default: [0]
          --zlib,                -zlib     [0:GZip, 1:Zlib]                                Default: [0]
 
Results
-------

Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~

The following table presents the resource utilization of an AMD ZLIB Quadcore Decompress kernel. The final Fmax achieved is 287 MHz. 

========== ===== ====== ===== ===== ===== 
Flow       LUT   LUTMem REG   BRAM  URAM 
========== ===== ====== ===== ===== ===== 
Decompress 26.2K   3.6K 18.9K   24    0
---------- ----- ------ ----- ----- -----
GzipMM2S   1.6K   253   2.5K    2    0   
---------- ----- ------ ----- ----- -----
GzipS2MM   2.8K   690   4.2K    2    0
========== ===== ====== ===== ===== ===== 

Performance Data
~~~~~~~~~~~~~~~~

The following table presents the kernel throughput achieved for a single compute unit. 

============================= =========================
Topic                         Results
============================= =========================
Decompression Throughput       1.15 Gb/s
============================= =========================

Standard GZip Support
---------------------

This application is compatible with a standard Gzip/Zlib application (compress/decompress).  
