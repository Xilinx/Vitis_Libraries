.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

=========================================
AMD ZSTD Decompression
=========================================

The ZSTD example resides in the ``L2/demos/zstd_decompress`` directory. 

Follow the build instructions to generate the host executable and binary.

The binary host file generated is named "**xil_zstd**", and it is present in the ``./build`` directory.

Results
-------

Overall Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following table presents the resource utilization of an AMD Zstd decompress kernel supporting a Window Size of 32 KB. The final Fmax achieved is 252 MHz.

========== ===== ====== ===== ===== ===== 
Flow       LUT   LUTMem REG   BRAM  URAM  
========== ===== ====== ===== ===== ===== 
DeCompress 21K   456    17K   33    6    
---------- ----- ------ ----- ----- -----
GzipMM2S   1.4K  295    1.6K  1     0  
---------- ----- ------ ----- ----- -----
GzipS2MM   2.7K  630K   4K    2     0
========== ===== ====== ===== ===== ===== 

Performance Data
~~~~~~~~~~~~~~~~

+----------------------------+------------------------+
| Topic                      | Kernel Throughput      |
+============================+========================+
| Decompression              | 463 Mb/s               |
+----------------------------+------------------------+

.. [*] Decompression uses feasibile options (Bitwidth: 32bit, Window Size: 32 KB). 

Executable Usage:

1. To execute single file for decompression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_zlib -xbin ./build_dir.<TARGET mode>.<xsa_name>/compress_decompress.xclbin -d <compressed file_name>``
2. To decompress multiple files: ``./build_dir.<TARGET mode>.<xsa_name>/xil_zlib -xbin ./build_dir.<TARGET mode>.<xsa_name>/compress_decompress.xclbin -l <files.list>``

	- ``<files.list>``: Contains various file names with the current path.

The usage of the generated executable is as follows:

.. code-block:: bash
 
   Usage: application.exe -[-h-d-sx-l]
        --help,                 -h      Print Help Options   Default: [false]
        --decompress,           -d      Decompress
        --decompress_xclbin,    -dx     Decompress XCLBIN
        --file_list,            -l      List of Input Files

