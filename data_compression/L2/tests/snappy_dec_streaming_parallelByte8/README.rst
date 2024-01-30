.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

=====================================
AMD Snappy Streaming Decompression 
=====================================

The Snappy Compress Streaming example resides in the ``L2/tests/snappy_dec_streaming_parallelByte8`` directory. 

Follow the build instructions to generate the host executable and binary.

The binary host file generated is named **xil_snappy_decompress_streaming**, and it is present in ``./build`` directory.

Executable Usage
----------------

1. To execute a single file for decompression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_snappy_decompress_streaming  -xbin ./build_dir.<TARGET mode>.<xsa_name>/decompress_streaming.xclbin -d <input file_name>``
2. To execute multiple files for decompression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_snappy_decompress_streaming  -xbin ./build_dir.<TARGET mode>.<xsa_name>/decompress_streaming.xclbin -dfl <files.list>``

    - ``<files.list>``: Contains various file names with the current path.

The usage of the generated executable is as follows:

.. code-block:: bash
       
   Usage: application.exe -[-h-d-dfl-xbin-id]
          --help,                -h        Print Help Options
          --decompress,          -d        Decompress
          --decompress_list,     -dfl      Decompress List of compressed Input Files
          --max_cr,              -mcr      Maximum CR                                            Default: [10]
          --xclbin,              -xbin     XCLBIN
          --device_id,           -id       Device ID                                             Default: [0]
          --block_size,          -B        Compress Block Size [0-64: 1-256: 2-1024: 3-4096]     Default: [0]

Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~

The following table presents the resource utilization of AMD Snappy Streaming Decompression kernels. The final Fmax achieved is 300 MHz.                                                                                                                   

========== ===== ====== ===== ===== ===== 
Flow       LUT   LUTMem REG   BRAM  URAM 
========== ===== ====== ===== ===== ===== 
Decompress 6.5K  316    5.7K   0     4
========== ===== ====== ===== ===== ===== 

Performance Data
~~~~~~~~~~~~~~~~

The following table presents the kernel throughput achieved for a single compute unit. 

============================= =========================
Topic                         Results
============================= =========================
Decompression Throughput       1.97 Gb/s
============================= =========================
