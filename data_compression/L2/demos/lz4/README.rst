=========================================
Xilinx LZ4 Compression and Decompression
=========================================

LZ4 example resides in ``L2/demos/lz4`` directory. 

Xilinx LZ4 compression/decompression is FPGA based implementation of
standard LZ4. Xilinx implementation of LZ4 application is aimed at
achieving high throughput for both compression and decompression. This
Xilinx LZ4 application is developed and tested on Xilinx Alveo U200. To
know more about standard LZ4 application please refer
https://github.com/lz4/lz4

This application is accelerated using generic hardware architecture for
LZ based data compression algorithms.

For more details refer this
`link <https://xilinx.github.io/Vitis_Libraries/data_compression/source/L2/design.html>`__

Results
-------

Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~

Table below presents resource utilization of Xilinx LZ4
compress/decompress kernels with 8 engines for single compute unit. It
is possible to extend number of engines to achieve higher throughput.

========== ===== ====== ===== ===== ===== ======
Flow       LUT   LUTMem REG   BRAM  URAM  Fmax
========== ===== ====== ===== ===== ===== ======
Compress   51.5K 14.1K  63.8K 58    48    300MHz
---------- ----- ------ ----- ----- ----- ------
DeCompress 30.6K 13.7K  39.5K 146    0    300MHz
========== ===== ====== ===== ===== ===== ======

Performance Data
~~~~~~~~~~~~~~~~

Table below presents best kernel throughput achieved for a single compute
unit. 

============================= =========================
Topic                         Results
============================= =========================
Best Compression Throughput   1.8 GB/s
Best Decompression Throughput 1.8 GB/s
Average Compression Ratio     2.13x (Silesia Benchmark)
============================= =========================

Note: Overall throughput can still be increased with multiple compute
units.

Software & Hardware
-------------------

::

     Software: Xilinx Vitis 2019.2
     Hardware: xilinx_u200_xdma_201830_2 (Xilinx Alveo U200)

Usage
-----

Execution Steps
~~~~~~~~~~~~~~~


::

     Input Arguments: 
       
           1. To execute single file for compression :  ./build/xil_lz4 -sx <compress_decompress xclbin> -c <file_name>
           2. To execute single file for decompression: ./build/xil_lz4 -sx <compress_decompress xclbin> -d <file_name.lz4>
           3. To validate various files together:       ./build/xil_lz4 -sx <compress_decompress xclbin> -l <files.list>
               3.a. <files.list>: Contains various file names with current path  
           4. To execute single file for compression and decompression : ./build/xil_lz4 -sx <compress_decompress xclbin> -v <file_name>  
           
      Note: Default arguments are set in Makefile

     Help:

           ===============================================================================================
           Usage: application.exe -[-h-c-l-d-sx-v-B-x]

                   --help                  -h      Print Help Options   Default: [false]
                   --compress              -c      Compress
                   --file_list             -l      List of Input Files
                   --decompress            -d      Decompress
                   --compress_decompress   -sx     Compress_Decompress binary
                   --validate              -v      Single file validate for Compress and Decompress  
                   --block_size            -B      Compress Block Size [0-64: 1-256: 2-1024: 3-4096] Default: [0]
                   --flow                  -x      Validation [0-All: 1-XcXd: 2-XcSd: 3-ScXd]   Default:[1]
           ===============================================================================================
