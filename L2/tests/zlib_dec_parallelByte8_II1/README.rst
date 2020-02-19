===================================
Xilinx ZLIB-Streaming Decompression
===================================

ZLIB example resides in ``L2/demos/zlib_streaming`` directory. 

Follow build instructions to generate host executable and binary.

The binary host file generated is named as "**zlib_stream**" and it is present in ``./build`` directory.

Results
-------

Resource Utilization 
~~~~~~~~~~~~~~~~~~~~

Table below presents resource utilization of Xilinx Zlib Decompress Streaming
kernel. 

========== ===== ====== ==== ===== ===== ======
Flow       LUT   LUTMem REG  BRAM  URAM  Fmax
========== ===== ====== ==== ===== ===== ======
DeCompress 14.2K  34     9K   11    2    219MHz
========== ===== ====== ==== ===== ===== ======

Performance Data
~~~~~~~~~~~~~~~~

Table below presents best kernel throughput achieved for a single compute
unit (Single Engine). 

============================= =========================
Topic                         Results
============================= =========================
Best Kernel Throughput        371.91 MB/s
============================= =========================

Note: Overall throughput can still be increased with multiple compute units.


Executable Usage:

1. To execute single file for decompression           : ``./build/zlib_stream -dx ./build/xclbin_<xsa_name>_<TARGET mode>/decompress_stream.xclbin -d <compressed file_name>``

The usage of the generated executable is as follows:

.. code-block:: bash
 
   Usage: application.exe -[-h-d]
        --help,                 -h      Print Help Options   Default: [false]
        --decompress,           -d      Decompress
