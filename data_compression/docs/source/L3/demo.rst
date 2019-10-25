.. CompressionLib_Docs documentation master file, created by
   sphinx-quickstart on Thu Jun 20 14:04:09 2019.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

====
Demo
====

This section presents integration of various modules from L1, L2 levels in
combination with software APIs to derive end application that can be directly
deployed or creation of shared library that can be integrated with external
applications.

Demo examples for Zlib and Lz4 applications are present in **L3/demos/**
directory.

Environment Setup
=================

Instructions below setup Vitis environment for builid the application. These
instructions are applicable for all the demos under this category.


.. code-block:: bash

    $source <Vitis_Installation_Path>/installs/lin64/Vitis/2019.2/settings64.csh
    $source <Vitis_Installation_Path>/xbb/xrt/packages/setenv.sh

Build Instructions
------------------

To compile and test run this application execute the following commands:

.. code-block:: bash
   
   $ make run TARGET=sw_emu

Variable ``TARGET`` can take the following values:

	- **sw_emu**	: software emulation
	
	- **hw_emu**	: hardware emulation
	
	- **hw**	: run on actual hardware


Build instructions are common for all the application but generated executable
differ.


Zlib Application
================

This section presents brief introduction about Zlib application and step by step
guidelines to build and deployment.

Overview
--------

ZLIB is an Open Source data compression library [Reference] which provides
high compression ratio compared to Limpel Ziev based data compression algorithms
(Byte Compression). It applies two levels of compression,

*  Byte Level (Limpel Ziev – LZ Based Compression Scheme)
*  Bit Level (Huffman – Entropy)

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

1. To execute single file for compression 	          : ``./build/xil_zlib -sx ./build/xclbin_<dsa_name>_<TARGET mode>/compress_decompress.xclbin -c <input file_name>``
2. To execute single file for decompression           : ``./build/xil_zlib -sx ./build/xclbin_<dsa_name>_<TARGET mode>/compress_decompress.xclbin -d <compressed file_name>``
3. To validate single file (compress & decompress)    : ``./build/xil_zlib -sx ./build/xclbin_<dsa_name>_<TARGET mode>/compress_decompress.xclbin -v <input file_name>``
4. To validate multiple files (compress & decompress) : ``./build/xil_zlib -sx ./build/xclbin_<dsa_name>_<TARGET mode>/compress_decompress.xclbin -l <files.list>``

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


LZ4 Application
================

LZ4 data compression application falls under Limpel Ziev based byte compression
scheme. It is widely known for achieving decompression throughput >GB/s on
high end single core high end CPU. 

This demo presents usage of FPGA accelerated LZ4 compression &
decompression which achieves throughput >GB/s and this application is scalable.


Executable Usage
----------------

This application is present in ``L3/demos/lz4_app`` directory. Follow build instructions to generate executable and binary.

The binary host file generated is named as "**xil_lz4**" and it is present in ``./build`` directory.

1. To execute single file for compression 	: ``./build/xil_lz4 -cx <compress xclbin> -c <file_name>``

2. To execute single file for decompression	: ``./build/xil_lz4 -dx <decompress xclbin> -d <file_name.lz4>``

3. To validate various files together		: ``./build/xil_lz4 -cx <compress xclbin> -dx <decompress xclbin> -l <files.list>``
	
	- ``<files.list>``: Contains various file names with current path

The usage of the generated executable is as follows:

.. code-block:: bash
   
   Usage: application.exe -[-h-c-l-d-B-x]
        --help,             -h      Print Help Options   Default: [false]
    	--compress_xclbin   -cx     Compress binary
        --compress,         -c      Compress
        --file_list,        -l      List of Input Files
        --decompress_xclbin -dx     Decompress binary
        --decompress,       -d      Decompress
        --block_size,       -B      Compress Block Size [0-64: 1-256: 2-1024: 3-4096] Default: [0]
        --flow,             -x      Validation [0-All: 1-XcXd: 2-XcSd: 3-ScXd] Default: [1]

LZ4 Compress
~~~~~~~~~~~~~

Code Snippet below explains the general usage of LZ4 compress library.

.. code-block:: cpp
    
    // Input: Raw File
    // Output: Compress File (.lz4)

    #include "lz4.hpp" 
    using namespace xf::compression;
    
    // Create LZ4 class object
    // a. Initiate OpenCL device setup
    // b. Load XCLBIN file to FPGA
    // c. Allocate host and device buffers
    xfLz4 xlz;
    
    // This variable sets the flows
    // LZ4 Compress or Decompress
    // 1 -> Compress
    xlz.m_bin_flow = 1;   

    // XCLBIN file
    // OpenCL setup
    xlz.init(binaryFileName);
        
    // Set the block size
    xlz.m_block_size_in_kb = block_size     
    
    // 0 means Xilinx Flow
    xlz.m_switch_flow = 0;
    
    // File I/O operations
    // Internally invokes FPGA Accelerated LZ4 Compress 
    // xf::compression::compress() 
    uint64_t enc_bytes = xlz.compressFile(inFile, outFile, input_size, file_list_flag);

    // Release OpenCL objects
    // Release device buffers
    xlz.release();

LZ4 Decompress
~~~~~~~~~~~~~

Code Snippet below explains the general usage of LZ4 compress library.

.. code-block:: cpp
    
    // Input: Compressed File (.lz4)
    // Output: Raw File

    #include "lz4.hpp" 
    using namespace xf::compression;
    
    // Create LZ4 class object
    // a. Initiate OpenCL device setup
    // b. Load XCLBIN file to FPGA
    // c. Allocate host and device buffers
    xfLz4 xlz;
    
    // This variable sets the flows
    // LZ4 Compress or Decompress
    // 1 -> Compress
    xlz.m_bin_flow = 1;   

    // XCLBIN file
    // OpenCL setup
    xlz.init(binaryFileName);
        
    // Set the block size
    xlz.m_block_size_in_kb = block_size     
    
    // 0 means Xilinx Flow
    xlz.m_switch_flow = 0;
    
    // File I/O operations
    // Internally invokes FPGA Accelerated LZ4 Compress 
    // xf::compression::compress() 
    uint64_t enc_bytes = xlz.compressFile(inFile, outFile, input_size, file_list_flag);

    // Release OpenCL objects
    // Release device buffers
    xlz.release();
