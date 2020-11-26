====================
GZip Application
====================

This section presents brief introduction about GZip application and step by step
guidelines to build and deployment.

Overview
--------

GZip is an Open Source data compression library* which provides
high compression ratio compared to Limpel Ziev based data compression algorithms
(Byte Compression). It applies two levels of compression,

*  Byte Level (Limpel Ziev  LZ Based Compression Scheme)
*  Bit Level (Huffman Entropy)

Due to its high compression ratio it takes higher precedence over LZ based
compression schemes. Traditionally the CPU based solutions are limited to MB/s
speed but there is a high demand for accelerated GZip which provides throughput
in terms of GB/s. 

This demo is aimed at showcasing Xilinx Alveo U250 acceleration of GZip for both
compression and decompression, it also supports Zlib with a host argument switch. 

.. code-block:: bash

   Tested Tool: 2020.2
   Tested XRT:  2020.2
   Tested XSA:  xilinx_u250_xdma_201830_2


Executable Usage
----------------

This application is present under ``L3/demos/gzip_app/`` directory. Follow build instructions to generate executable and binary.

The host executable generated is named as "**xil_gzip**" and it is generated in ``./build`` directory.

Following is the usage of the executable:

1. To execute single file for compression 	          : ``./build/xil_gzip -sx ./build/xclbin_<xsa_name>_<TARGET mode>/compress_decompress.xclbin -c <input file_name>``
2. To execute single file for decompression           : ``./build/xil_gzip -sx ./build/xclbin_<xsa_name>_<TARGET mode>/compress_decompress.xclbin -d <compressed file_name>``
3. To validate single file (compress & decompress)    : ``./build/xil_gzip -sx ./build/xclbin_<xsa_name>_<TARGET mode>/compress_decompress.xclbin -v <input file_name>``
4. To validate multiple files (compress & decompress) : ``./build/xil_gzip -sx ./build/xclbin_<xsa_name>_<TARGET mode>/compress_decompress.xclbin -l <files.list>``

	- ``<files.list>``: Contains various file names with current path

The default design flow is GZIP design to run the ZLIB, enable the switch ``-zlib`` in the command line, as mentioned below:
``./build/xil_gzip -sx ./build/xclbin_<xsa_name>_<TARGET mode>/compress_decompress.xclbin -c <input file_name> -zlib 1``

The -sx option mentioned above is optional, you can provide path to your binary file using -sx option otherwise it will by default map to ``./build/xclbin_<xsa_name>_<TARGET mode>/compress_decompress.xclbin`` 


The usage of the generated executable is as follows:

.. code-block:: bash

   Usage: application.exe -[-h-c-d-sx-v-l-k-id-mcr]

          --help,                    -h        Print Help Options
          --compress,                -c        Compress
          --decompress,              -d        DeCompress
          --single_xclbin,           -sx       Single XCLBIN           Default: [single]
          --compress_decompress,     -v        Compress Decompress
          --zlib,                    -zlib     [0:GZIP, 1:ZLIB]        Default: [0]
          --file_list,               -l        List of Input Files
          --cu,                      -k        CU                      Default: [0]
          --id,                      -id       Device ID               Default: [0]
          --max_cr,                  -mcr      Maximum CR              Default: [10]
===========================================================

