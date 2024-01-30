.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

===========================================
AMD Snappy Compression and Decompression
===========================================

The Snappy example resides in the ``L2/demos/snappy`` directory. 

AMD Snappy compression/decompression is a FPGA based implementation of standard Snappy. The AMD implementation of the Snappy application is aimed at achieving high throughput for both compression and decompression. This AMD Snappy application is developed and tested on AMD Alveo™ U200. To learn more about the standard Snappy application, refer to https://github.com/google/snappy.

This application is accelerated using the generic hardware architecture for LZ based data compression algorithms.

Results
-------

Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~

The following table presents the resource utilization of Snappy compress/decompress kernels with eight engines for a single compute unit. The final Fmax achieved for this design is 299 MHz. 

========== ===== ====== ===== ===== ===== 
Flow       LUT   LUTMem REG   BRAM  URAM  
========== ===== ====== ===== ===== ===== 
Compress   48K   10.8K  50.6K 48    48    
---------- ----- ------ ----- ----- ----- 
DeCompress 12.4K 2.9K   16.5K 16    4    
========== ===== ====== ===== ===== ===== 

Performance Data
~~~~~~~~~~~~~~~~

The following presents the kernel throughput achieved with single compute unit during execution of this application.

============================= =========================
Topic                         Results
============================= =========================
Compression Throughput        1.8 Gb/s
Decompression Throughput      1 Gb/s
Average Compression Ratio     2.14x (Silesia Benchmark)
============================= =========================

.. note:: The overall throughput can still be increased with multiple compute units.

Software and Hardware
-------------------

::

     Software: AMD Vitis™ 2022.2
     Hardware: xilinx_u200_xdma_201830_2 (Alveo U200)

Usage
-----

Build Steps
~~~~~~~~~~~

Emulation Flows
^^^^^^^^^^^^^^^

::

     make run TARGET=<sw_emu/hw_emu> PLATFORM=xilinx_u200_xdma_201830_2
     
    NOTE: This command compiles for the targeted emulation mode and executes the application.

Hardware
^^^^^^^^

::

     make all TARGET=hw PLATFORM=xilinx_u200_xdma_201830_2

     Note: This command compiles for hardware execution. It generates the kernel binary ".xclbin" file. 
           This file is placed in the ./build/xclbin*/ directory under the Snappy folder.

Executable Usage
----------------
 
1. To execute a single file for compression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_snappy -xbin ./build_dir.<TARGET mode>.<xsa_name>/<compress_decompress.xclbin> -c <file_name>``
2. To execute a single file for decompression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_snappy -xbin ./build_dir.<TARGET mode>.<xsa_name>/<compress_decompress.xclbin> -d <file_name.snappy>``
3. To validate a single file (compress & decompress): ``./build_dir.<TARGET mode>.<xsa_name>/xil_snappy -xbin ./build_dir.<TARGET mode>.<xsa_name>/<compress_decompress.xclbin> -t <file_name>``
4. To execute multiple files for compression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_snappy -xbin ./build_dir.<TARGET mode>.<xsa_name>/<compress_decompress.xclbin> -cfl <files.list>``
5. To execute multiple files for decompression: ``./build_dir.<TARGET mode>.<xsa_name>/xil_snappy -xbin ./build_dir.<TARGET mode>.<xsa_name>/<compress_decompress.xclbin> -dfl <compressed files.list>``
6. To validate multiple files (compress and decompress) : ``./build_dir.<TARGET mode>.<xsa_name>/xil_snappy -xbin ./build_dir.<TARGET mode>.<xsa_name>/<compress_decompress.xclbin> -l <files.list>``  
               
      - ``<files.list>``: Contains various file names with the current path.

      - Note: The default arguments are set in the Makefile.

The usage of the generated executable is as follows:

.. code-block:: bash

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
