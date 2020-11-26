This LZ4 P2P Decompress application runs with standard compression and
xilinx decompression flow. This application gives best kernel 
throughput when multiple files run concurrently on both compute units.

- Tested tool/xrt/xsa information 

:: 

    Tool: 2020.2
    XRT : 2020.2
    XSA : xilinx_samsung_u2x4_201920_2



-  Source codes (data_compression): In this folder all the source files
   are available.

::

      Host Sources : ./data_compression/L3/src/
      Host Includes : ./data_compression/L3/include/
      Kernel Code  : ./data_compression/L2/src/
      HLS Modules  : ./data_compression/L1/include/

-  Running Emulation: Steps to build the design and run for sw_emu

::

       $ Setup Xilinx vitis 2020.2 along with XRT 
       $ cd ./data_compression/L3/benchmarks/lz4_p2p_dec/
       $ make run TARGET=sw_emu DEVICE=<path to u.2 xpfm file>

-  Building Design (xclbin): Steps to build the design and run for hw

::

       $ Setup Xilinx vitis 2020.2 along with XRT 
       $ cd ./data_compression/L3/benchmarks/lz4_p2p_dec/
       $ make all TARGET=hw DEVICE=<path to u.2 xpfm file> 

-  Input Test Data

   -  The input files are placed in data/ folder under
      L3/benchmarks/lz4_p2p_dec/ which are used for design.

-  Following are the step by step instructions to run the design on
   board.

   -  Source the XRT

   ::

            $ source /opt/xilinx/xrt/setup.sh

   -  To Mount the data from SSD

   ::

            $ mkfs.xfs -f /dev/nvme0n1
            $ mount /dev/nvme0n1 /mnt
            $ cp -rf <./data/> /mnt/ (copy the input files to /mnt path)

   -  To run the design. Please give minumum of 20 files in test.list file to 
      see the best kernel throughput.
      - Please give input_file name as path in test.list file instead of .lz4 as 
        shown below since host will take care of .lz4 extension part:
        $ /mnt/sample.txt
   ::

            To enable P2P flow give 1
            $ ./build/<host_exe> -cx ./build/decompress.xclbin -p2p 1 -l <./test.list> 

Results
-------

Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~

Table below presents resource utilization of Xilinx LZ4 P2P Decompress
kernel with 8 engines for single compute unit. It is possible to extend
number of engines to achieve higher throughput.

========== ===== ====== ===== ===== ===== 
Flow       LUT   LUTMem REG   BRAM  URAM 
========== ===== ====== ===== ===== ===== 
Compress   34.9K 14.2K  45.1K  145   0    
---------- ----- ------ ----- ----- ----- 
Packer     10.6K  435   13.6K  15     0    
========== ===== ====== ===== ===== ===== 

Throughput & Compression Ratio
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Table below presents the best end to end decompress kernel execution with
SSD write throughput achieved with two compute units during execution of
this application.

=========================== ========
Topic                       Results
=========================== ========
Best Compression Throughput 2.5 GB/s
=========================== ========

Note: Overall throughput can still be increased with multiple compute
units.
