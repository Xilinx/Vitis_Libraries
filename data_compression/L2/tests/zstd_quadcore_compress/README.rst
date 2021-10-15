=========================================
Xilinx ZSTD Compression
=========================================

ZSTD example resides in ``L2/tests/zstd_quadcore_compress`` directory. 

Follow build instructions to generate host executable and binary.

The binary host file generated is named as "**xil_zstd**" and it is present in ``./build`` directory.



Results
-------

Overall Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Table below presents resource utilization of Xilinx Zstd decompress kernel supporting Window Size of 32KB. The final Fmax achieved is 284MHz.

========== ===== ====== ===== 
Flow       LUT   BRAM   URAM  
========== ===== ====== ===== 
Compress   40K   79     37   
========== ===== ====== ===== 


Performance Data
~~~~~~~~~~~~~~~~

============================  ===========================
 Topic                          Results       
============================  ===========================
Compression                     1.17 GB/s                
Average Compression Ratio	    2.68x (Silesia Benchmark)
============================  ===========================

Executable Usage:

1. To execute single file for decompression           : ``./build/xil_zlib -cx ./build/xclbin_<xsa_name>_<TARGET mode>/compress.xclbin -c <compressed file_name>``
2. To decompress multiple files                       : ``./build/xil_zlib -cx ./build/xclbin_<xsa_name>_<TARGET mode>/compress.xclbin -cfl <files.list>``

	- ``<files.list>``: Contains various file names with current path

The usage of the generated executable is as follows:

.. code-block:: bash
 
   Usage: application.exe -[-h-d-sx-l]
        --help,                 -h      Print Help Options   Default: [false]
        --decompress,           -c      Decompress
        --decompress_xclbin,    -cx     Decompress XCLBIN
        --file_list,            -cfl    List of Input Files

