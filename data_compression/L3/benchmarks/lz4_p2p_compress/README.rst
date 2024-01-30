.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

===================================
LZ4 P2P Application for Compression
===================================

This LZ4 P2P Compress application runs with the AMD compression and standard decompression flow. This application gives best kernel throughput when multiple files run concurrently on both compute units.

Results
-------

Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~

The following table presents the resource utilization of an AMD LZ4 P2P compress kernel with eight engines for single compute unit. It is possible to extend the number of engines to achieve higher throughput.

========== ===== ====== ===== ===== ===== 
Flow       LUT   LUTMem REG   BRAM  URAM 
========== ===== ====== ===== ===== ===== 
Compress   51.7K 14.2K  64.2K 58    48    
---------- ----- ------ ----- ----- ----- 
Packer     10.9K 1.8K   16.7K 16     0    
========== ===== ====== ===== ===== ===== 

Throughput and Compression Ratio
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following table presents the best end to end compress kernel execution with SSD write throughput achieved with two compute units during execution of this application.

=========================== ========
Topic                       Results
=========================== ========
Compression Throughput 1.6 Gb/s
=========================== ========

.. note:: The overall throughput can still be increased with multiple compute units.

Executable Usage
----------------

This application is present in the ``L3/benchmarks/lz4_p2p_compress`` directory. Follow the build instructions to generate the executable and binary.

The binary host file generated is named "**xil_lz4**", and it is present in the ``./build`` directory.

1. To execute a single file for compression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_lz4 -cx ./build_dir.<TARGET mode>.<xsa_name>/<compress.xclbin> -c <file_name>``
2. To execute multiple files for compression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_lz4 -cx ./build_dir.<TARGET mode>.<xsa_name>/compress.xclbin -l <files.list>``

     - ``<files.list>``: Contains various file names with the current path.

The usage of the generated executable is as follows:

.. code-block:: bash
      
   Usage: application.exe -[-h-c-l-B-p2p] 
          --help,                -h       Print Help Options
          --compress_xclbin,     -cx      Compress XCLBIN                                       Default: [compress]
          --compress,            -c       Compress
          --file_list,           -l       List of Input Files
          --p2p_mod,             -p2p     P2P Mode
          --block_size,          -B       Compress Block Size [0-64: 1-256: 2-1024: 3-4096]     Default: [0]
          --id,                  -id      Device ID                                             Default: [0]
