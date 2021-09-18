================
Zlib Application
================

This section presents brief introduction about Zlib application and step by step
guidelines to build and deployment.

Overview
--------

ZLIB is an Open Source data compression library which provides
high compression ratio compared to Limpel Ziev based data compression algorithms
(Byte Compression). It applies two levels of compression,

*  Byte Level (Limpel Ziev  LZ Based Compression Scheme)
*  Bit Level (Huffman Entropy)

Due to its high compression ratio it takes higher precedence over LZ based
compression schemes. Traditionally the CPU based solutions are limited to MB/s
speed but there is a high demand for accelerated ZLIB which provides throughput
in terms of GB/s. 

This demo is aimed at showcasing Alveo U200 acceleration of ZLIB for both
compression and decompression. 


Executable Usage
----------------

This application is present under ``L3/demos/zlib_app/`` directory. Follow build instructions to generate executable and binary.

The host executable generated is named as "**xil_zlib**" and it is generated in ``./build`` directory.

Following is the usage of the executable:

1. To execute single file for compression 	          : ``./build/xil_zlib -sx ./build/xclbin_<xsa_name>_<TARGET mode>/compress_decompress.xclbin -c <input file_name>``
2. To execute single file for decompression           : ``./build/xil_zlib -sx ./build/xclbin_<xsa_name>_<TARGET mode>/compress_decompress.xclbin -d <compressed file_name>``
3. To validate single file (compress & decompress)    : ``./build/xil_zlib -sx ./build/xclbin_<xsa_name>_<TARGET mode>/compress_decompress.xclbin -v <input file_name>``
4. To validate multiple files (compress & decompress) : ``./build/xil_zlib -sx ./build/xclbin_<xsa_name>_<TARGET mode>/compress_decompress.xclbin -l <files.list>``

	- ``<files.list>``: Contains various file names with current path

The usage of the generated executable is as follows:

.. code-block:: bash
 
   Usage: application.exe -[-h-c-d-sx-v-l-k]
        --help,                 -h      Print Help Options   Default: [false]
        --compress,             -c      Compress
        --decompress,           -d      Decompress
        --single_xclbin,        -sx     Single XCLBIN        Default: [single]
        --file_list,            -l      List of Input Files
        --compress_decompress,  -v      Compress Decompress
        --cu,                   -k      CU                   Default: [0]

Software API Usage
------------------

This section provides usage information related to software APIs.

Zlib Compress
~~~~~~~~~~~~~

Code Snippet below explains the general usage of Zlib compress library.

.. code-block:: cpp
    
    // Input: Raw File
    // Output: Compress File (.zlib)

    #include "zlib.hpp" 
    using namespace xf::compression;
    
    // Create Zlib class object
    // a. Initiate OpenCL device setup
    // b. Load XCLBIN file to FPGA
    // c. Allocate host and device buffers
    xfZlib* xlz;
    xlz = new xfZlib(single_xclbin);
    
    // File I/O operations
    // Invoke FPGA Accelerated Zlib Compress 
    // xf::compression::compress() 
    uint32_t enc_bytes = xlz->compress_file(inFile, outFile, input_size);

Zlib Decompress
~~~~~~~~~~~~~~~

Code Snippet below explains the general usage of Zlib decompress library.

.. code-block:: cpp

    // Input: Compress File (.zlib)
    // Output: Raw File

    #include "zlib.hpp" 
    using namespace xf::compression;
    
    // Create Zlib class object
    // a. Initiate OpenCL device setup
    // b. Load XCLBIN file to FPGA
    // c. Allocate host and device buffers
    xfZlib* xlz;
    xlz = new xfZlib(single_xclbin);
    
    // File I/O operations
    // Internally invokes FPGA Accelerated Zlib Compress 
    // xf::compression::decompress() 
    uint32_t dec_bytes = xlz->decompress_file(inFile, outFile, input_size);

Zlib Shared Library (libz.so)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The makefile presented in this demo can be used to create standard
``libz.so`` which can be linked against the user applications.

.. code-block:: bash
   
    1. make lib (To Create libz.so)
    2. Location: ./build/libz.so

Note: This feature is first release it is ideal to use FPGA based compress and
decompress for standard compliance.
