.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

=========================================
AMD LZ4 Compression and Decompression
=========================================

The LZ4 demo resides in the ``L2/demos/lz4`` directory.

AMD LZ4 compression/decompression is a FPGA based implementation of standard LZ4. The AMD implementation of the LZ4 application is aimed at achieving high throughput for both compression and decompression. This
AMD LZ4 application is developed and tested on AMD Alveo™ U200. To learn more about standard LZ4 application, refer https://github.com/lz4/lz4.

This application is accelerated using a generic hardware architecture for LZ based data compression algorithms.

Results
-------

Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~

The following table presents the resource utilization of LZ4 Compress/Decompress kernels. The final Fmax achieved is 262 MHz. 

========== ===== ====== ===== ===== ===== 
Flow       LUT   LUTMem REG   BRAM  URAM 
========== ===== ====== ===== ===== ===== 
Compress   47.6K 10.7K  50.7K 56    48    
---------- ----- ------ ----- ----- ----- 
DeCompress 7.3K  1.1K   7K    2     4     
========== ===== ====== ===== ===== ===== 

Performance Data
~~~~~~~~~~~~~~~~

The following table presents the kernel throughput achieved for a single compute unit. 

============================= =========================
Topic                         Results
============================= =========================
Compression Throughput        1.7 Gb/s
Decompression Throughput      443 Mb/s
Average Compression Ratio     2.13x (Silesia Benchmark)
============================= =========================

.. note:: The overall throughput can still be increased with multiple compute units.

Software and Hardware
-------------------

::

     Software: AMD Vitis™ 2022.2
     Hardware: xilinx_u200_xdma_201830_2 (Alveo U200)

Executable Usage
----------------
 
1. To execute single file for compression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_lz4 -xbin ./build_dir.<TARGET mode>.<xsa_name>/<compress_decompress.xclbin> -c <file_name>``
2. To execute single file for decompression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_lz4 -xbin ./build_dir.<TARGET mode>.<xsa_name>/<compress_decompress.xclbin> -d <file_name.lz4>``
3. To validate single file (compress & decompress): ``./build_dir.<TARGET mode>.<xsa_name>/xil_lz4 -xbin ./build_dir.<TARGET mode>.<xsa_name>/<compress_decompress.xclbin> -t <file_name>``
4. To execute multiple files for compression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_lz4 -xbin ./build_dir.<TARGET mode>.<xsa_name>/<compress_decompress.xclbin> -cfl <files.list>``
5. To execute multiple files for decompression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_lz4 -xbin ./build_dir.<TARGET mode>.<xsa_name>/<compress_decompress xclbin> -dfl <compressed files.list>``
6. To validate multiple files (compress and decompress): ``./build_dir.<TARGET mode>.<xsa_name>/xil_lz4 -xbin ./build_dir.<TARGET mode>.<xsa_name>/<compress_decompress xclbin> -l <files.list>``  
           
      - ``<files.list>``: Contains various file names with the current path.

      .. note:: The default arguments are set in the Makefile.

The usage of the generated executable is as follows:

.. code-block:: bash

   Usage: application.exe -[-h-c-d-t-cfl-dfl-l-xbin-B-id]
          --help,                -h        Print Help Options
          --compress,            -c        Compress
          --decompress,          -d        Decompress
          --test,                -t        Compress & Decompress
          --compress_list,       -cfl      Compress List of Input Files
          --decompress_list,     -dfl       Decompress List of compressed Input Files
          --test_list,           -l        Compress & Decompress on Input Files
          --max_cr,              -mcr      Maximum CR                                            Default: [10]
          --xclbin,              -xbin     XCLBIN
          --device_id,           -id       Device ID                                             Default: [0]
          --block_size,          -B        Compress Block Size [0-64: 1-256: 2-1024: 3-4096]     Default: [0]
