.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

=====================================================
AMD Snappy-Streaming Compression and Decompression
=====================================================

LZ4-Streaming example resides in the ``L2/demos/snappy_streaming`` directory. To compile and test run this example, execute the following commands:

Follow the build instructions to generate the host executable and binary.

The binary host file generated is named "**xil_snappy_streaming**", using a `PARALLEL_BLOCK` value of 8 (default), and present in the ``./build`` directory.

Results
-------

Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~

The following table presents the resource utilization of AMD Snappy Streaming compress/decompress kernels (excluding data movers). It achieves Fmax of 300 MHz. 

========== ===== ====== ==== ===== ===== 
Flow       LUT   LUTMem REG  BRAM  URAM 
========== ===== ====== ==== ===== ===== 
Compress   3K    137    3.5K  4     6     
---------- ----- ------ ---- ----- ----- 
DeCompress 6.4K  316    5.7K  0     4     
========== ===== ====== ==== ===== ===== 

Performance Data
~~~~~~~~~~~~~~~~

The following table presents the kernel throughput achieved for a single compute unit (Single Engine). 

============================= =========================
Topic                         Results
============================= =========================
Compression Throughput        260 Mb/s
Decompression Throughput      1.97 Gb/s
Average Compression Ratio     2.13x (Silesia Benchmark)
============================= =========================

.. note:: The overall throughput can still be increased with multiple compute units.

Executable Usage
~~~~~~~~~~~~~~~
                                                                                                                                                             
1. To execute single file for compression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_snappy_streaming -xbin ./build_dir.<TARGET mode>.<xsa_name>/<compress_decompress_streaming.xclbin> -c <file_name>``
2. To execute single file for decompression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_snappy_streaming -xbin ./build_dir.<TARGET mode>.<xsa_name>/<compress_decompress_streaming.xclbin> -d <file_name.snappy>``
3. To validate single file (compress & decompress): ``./build_dir.<TARGET mode>.<xsa_name>/xil_snappy_streaming -xbin ./build_dir.<TARGET mode>.<xsa_name>/<compress_decompress_streaming.xclbin> -t <files_name>``
4. To execute multiple files for compression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_snappy_streaming -xbin ./build_dir.<TARGET mode>.<xsa_name>/<compress_decompress_streaming.xclbin -cfl <files.list>``
5. To execute multiple files for decompression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_snappy_streaming -xbin ./build_dir.<TARGET mode>.<xsa_name>/<compress_decompress_streaming.xclbin -dfl <compressed files.list>``   
6. To validate multiple files (compress & decompress): ``./build_dir.<TARGET mode>.<xsa_name>/xil_snappy_streaming -xbin ./build_dir.<TARGET mode>.<xsa_name>/<compress_decompress_streaming.xclbin -l <files.list>``  
        
      - ``<files.list>``: Contains various file names with the current path.

The usage of the generated executable is as follows:

.. code-block:: bash
      
   Usage: application.exe -[-h-c-d-t-cfl-dfl-l-B-id]
          --help,                -h        Print Help Options
          --compress,            -c        Compress
          --decompress,          -d        Decompress
          --test,                -t        Compress & Decompress
          --compress_list,       -cfl      Compress List of Input Files
          --decompress_list,     -dfl      Decompress List of compressed Input Files
          --test_list,           -l        Compress & Decompress on Input Files
          --max_cr,              -mcr      Maximum CR                                            Default: [10]
          --xclbin,              -xbin     XCLBIN
          --device_id,           -id       Device ID                                             Default: [0]
          --block_size,          -B        Compress Block Size [0-64: 1-256: 2-1024: 3-4096]     Default: [0]            



