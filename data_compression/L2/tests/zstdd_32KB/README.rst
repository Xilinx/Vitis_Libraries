.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

=========================================
AMD ZSTD Decompression
=========================================

The ZSTD example resides in the ``L2/tests/zstd_32KB`` directory. 

Follow the build instructions to generate xthe host executable and binary.

The binary host file generated is named "**xil_zstd**", and it is present in the ``./build`` directory.

Results
-------

Overall Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following table presents the resource utilization of an AMD Zstd decompress kernel supporting a Window Size of 32 KB. The final Fmax achieved is 234 MHz.

========== ===== ====== =====  
Flow       LUT   BRAM   URAM  
========== ===== ====== =====  
DeCompress  22K  32     3     
========== ===== ====== =====  


Performance Data
~~~~~~~~~~~~~~~~

+----------------------------+------------------------+
| Topic                      | Kernel Throughput      |
+============================+========================+
| Decompression              |  658.86 Mb/s           |
+----------------------------+------------------------+

.. [*] Decompression uses feasibile options (Bitwidth: 32bit, Window Size: 32 KB) 

Executable Usage:

1. To execute a single file for decompression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_zstd -xbin ./build_dir.<TARGET mode>.<xsa_name>/xilZstdDecompressStream.xclbin -d <compressed file_name>``
2. To decompress multiple files: ``./build_dir.<TARGET mode>.<xsa_name>/xil_zstd -xbin ./build_dir.<TARGET mode>.<xsa_name>/xilZstdDecompressStream.xclbin -dfl <files.list>``

	- ``<files.list>``: Contains various file names with the current path.

The usage of the generated executable is as follows:

.. code-block:: bash
 
   Usage: application.exe -[-h-d-sx-l]
        --help,                 -h      Print Help Options   Default: [false]
        --decompress,           -d      Decompress
        --decompress_xclbin,    -dx     Decompress XCLBIN
        --file_list,            -dfl    List of Input Files

