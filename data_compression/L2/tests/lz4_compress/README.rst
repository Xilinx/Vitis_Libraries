.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

================================
AMD LZ4 Compression 
================================

The LZ4 Compress example resides in the ``L2/tests/lz4_compress`` directory. 

Follow the build instructions to generate the host executable and binary.

The binary host file generated is named **xil_lz4**, and it is present in the ``./build`` directory.

Executable Usage
----------------

1. To execute a single file for compression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_lz4 -xbin ./build_dir.<TARGET mode>.<xsa_name>/compress.xclbin -c <input file_name>``
2. To execute multiple files for compression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_lz4 -xbin ./build_dir.<TARGET mode>.<xsa_name>/compress.xclbin -cfl <files.list>``

    - ``<files.list>``: Contains various file names with the current path.

The usage of the generated executable is as follows:

.. code-block:: bash
       
   Usage: application.exe -[-h-c-cfl-xbin-id]
          --help,                -h        Print Help Options
          --compress,            -c        Compress
          --compress_list,       -cfl      Compress List of Input Files
          --max_cr,              -mcr      Maximum CR                                            Default: [10]
          --xclbin,              -xbin     XCLBIN
          --device_id,           -id       Device ID                                             Default: [0]
          --block_size,          -B        Compress Block Size [0-64: 1-256: 2-1024: 3-4096]     Default: [0]

Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~

The following table presents the resource utilization of AMD LZ4 Streaming Compression kernels. The final Fmax achieved is 300 MHz.                                                                                                                   

========== ===== ====== ===== ===== ===== 
Flow       LUT   LUTMem REG   BRAM  URAM 
========== ===== ====== ===== ===== ===== 
Compress   51.6K 10.8K   57K   56    48
========== ===== ====== ===== ===== ===== 

Performance Data
~~~~~~~~~~~~~~~~

The following table presents the kernel throughput achieved for a single compute unit. 

============================= =========================
Topic                         Results
============================= =========================
Compression Throughput        2.2 Gb/s
Average Compression Ratio     2.13x (Silesia Benchmark)
============================= =========================
