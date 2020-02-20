===============
LZ4 Application
===============

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
    xlz.m_BinFlow = 1;   

    // XCLBIN file
    // OpenCL setup
    xlz.init(binaryFileName);
        
    // Set the block size
    xlz.m_BlockSizeInKb = block_size     
    
    // 0 means Xilinx Flow
    xlz.m_SwitchFlow = 0;
    
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
    xlz.m_BinFlow = 1;   

    // XCLBIN file
    // OpenCL setup
    xlz.init(binaryFileName);
        
    // Set the block size
    xlz.m_BlockSizeInKb = block_size     
    
    // 0 means Xilinx Flow
    xlz.m_SwitchFlow = 0;
    
    // File I/O operations
    // Internally invokes FPGA Accelerated LZ4 Compress 
    // xf::compression::compress() 
    uint64_t enc_bytes = xlz.compressFile(inFile, outFile, input_size, file_list_flag);

    // Release OpenCL objects
    // Release device buffers
    xlz.release();
