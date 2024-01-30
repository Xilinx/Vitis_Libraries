.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

=====================================
LZ4 P2P Application for Decompression
=====================================

This LZ4 P2P Decompress application runs with standard compression and decompression flow. This application gives best kernel  throughput when multiple files run concurrently on both compute units.

Results
-------

Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~

The following table presents the resource utilization of an AMD LZ4 P2P Decompress kernel with eight engines for single compute unit. It is possible to extend number of engines to achieve higher throughput.

========== ===== ====== ===== ===== ===== 
Flow       LUT   LUTMem REG   BRAM  URAM 
========== ===== ====== ===== ===== ===== 
Decompress   34.9K 14.2K  45.1K  145   0    
---------- ----- ------ ----- ----- ----- 
Packer     10.6K  435   13.6K  15     0    
========== ===== ====== ===== ===== ===== 

Throughput and Decompression Ratio
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following table presents the best end to end decompress kernel execution with SSD write throughput achieved with two compute units during execution of this application.

=========================== ========
Topic                       Results
=========================== ========
Decompression Throughput 2.5 Gb/s
=========================== ========

.. note:: The overall throughput can still be increased with multiple compute units.

Executable Usage
----------------

This application is present in the ``L3/benchmarks/lz4_p2p_decompress`` directory. Follow the build instructions to generate the executable and binary.

The binary host file generated is named "**xil_lz4**", and it is present in ``./build`` directory.

1. To execute a single file for decompression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_lz4 -dx ./build_dir.<TARGET mode>.<xsa_name>/<decompress.xclbin> -d <file_name>``
2. To execute multiple files for decompression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_lz4 -dx ./build_dir.<TARGET mode>.<xsa_name>/decompress.xclbin -l <files.list>``
     - ``<files.list>``: Contains various file names with the current path.

The usage of the generated executable is as follows:

.. code-block:: bash
         
   Usage: application.exe -[-h-d-l-B-p2p] 
          --help,                -h       Print Help Options
          --decompress_xclbin,   -dx      Decompress XCLBIN                                       Default: [decompress]
          --decompress,          -d       Decompress
          --file_list,           -l       List of Input Files
          --p2p_mod,             -p2p     P2P Mode
          --block_size,          -B       Decompress Block Size [0-64: 1-256: 2-1024: 3-4096]     Default: [0]
          --id,                  -id      Device ID                                             Default: [0]    
