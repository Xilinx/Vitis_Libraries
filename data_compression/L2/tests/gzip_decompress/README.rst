====================================
Xilinx GZip Streaming Decompression
====================================

GZip example resides in ``L2/tests/gzip_decompress`` directory. 

Follow build instructions to generate host executable and binary.

The binary host file generated is named as "**xil_gzip**" and it is present in ``./build`` directory.

Executable Usage
----------------

1. To execute single file for compression 	          : ``./build/xil_gzip -dx ./build/xclbin_<xsa_name>_<TARGET mode>/decompress.xclbin  -d <file_name>``
2. To execute multiple files for compression    : ``./build/xil_gzip -dx ./build/xclbin_<xsa_name>_<TARGET mode>/decompress.xclbin  -l <files.list>``

	- ``<files.list>``: Contains various file names with current path

The usage of the generated executable is as follows:

.. code-block:: bash
 
   Usage: application.exe -[-h-d-l-dx-B]
          --help,                  -h       Print Help Options
          --decompress,            -d       DeCompress
          --decompress_xclbin,     -dx      Decompress XCLBIN          Default: [decopress]
          --device,                -dev     FPGA Card # to be used
          --file_list,             -l       List of Input Files
          --cu,                    -k       CU                         Default: [0]
          --max_cr,                -mcr     Maximum CR                 Default: [20]
 
Results
-------

Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~

Table below presents resource utilization of Xilinx GZip Compress/Decompress
kernels. The final Fmax achieved is 252MHz 

========== ===== ====== ===== ===== ===== 
Flow       LUT   LUTMem REG   BRAM  URAM 
========== ===== ====== ===== ===== ===== 
Decompress 11.3K  113    7K    6    3
---------- ----- ------ ----- ----- -----
GzipMM2S   1.4K   282    2K    2    0   
---------- ----- ------ ----- ----- -----
GzipS2MM   2.7K   624    4K    2    0
========== ===== ====== ===== ===== ===== 


Performance Data
~~~~~~~~~~~~~~~~

Table below presents kernel throughput achieved for a single compute
unit. 

============================= =========================
Topic                         Results
============================= =========================
Decompression Throughput       450 MB/s
============================= =========================

Standard GZip Support
---------------------

This application is compatible with standard Gzip/Zlib application (compress/decompres).  
