===========================================
Xilinx Snappy Compression and Decompression
===========================================

Snappy example resides in ``L2/demos/snappy`` directory. 

Xilinx Snappy compression/decompression is FPGA based implementation of
standard Snappy. Xilinx implementation of Snappy application is aimed at
achieving high throughput for both compression and decompression. This
Xilinx Snappy application is developed and tested on Xilinx Alveo U200.
To know more about standard Snappy application please refer
https://github.com/snappy/snappy

This application is accelerated using generic hardware architecture for
LZ based data compression algorithms.

For more details refer this
`link <https://xilinx.github.io/Vitis_Libraries/data_compression/source/L2/design.html>`__

Results
-------

Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~

Table below presents resource utilization of Xilinx Snappy
compress/decompress kernels with 8 engines for single compute unit. It
is possible to extend number of engines to achieve higher throughput.

========== ===== ====== ===== ===== ===== ======
Flow       LUT   LUTMem REG   BRAM  URAM  Fmax
========== ===== ====== ===== ===== ===== ======
Compress   52.7K 13.8K  64.8K 50    48    300MHz
---------- ----- ------ ----- ----- ----- ------
DeCompress 30.7K 13.4K  39.4K 146    0    300MHz
========== ===== ====== ===== ===== ===== ======

Performance Data
~~~~~~~~~~~~~~~~

Table below presents the best kernel throughput achieved with single
compute unit during execution of this application.

============================= =========================
Topic                         Results
============================= =========================
Best Compression Throughput   1.5 GB/s
Best Decompression Throughput 1.8 GB/s
Average Compression Ratio     2.15x (Silesia Benchmark)
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

Build Steps
~~~~~~~~~~~

Emulation flows
^^^^^^^^^^^^^^^

::

     make run TARGET=<sw_emu/hw_emu> DEVICE=xilinx_u200_xdma_201830_2
     
     Note: This command compiles for targeted emulation mode and executes the
           application.

Hardware
^^^^^^^^

::

     make all TARGET=hw DEVICE=xilinx_u200_xdma_201830_2

     Note: This command compiles for hardware execution. It generates kernel binary ".xclbin" file. 
           This file is placed in ./build/xclbin*/ directory under Snappy folder.

Execution Steps
~~~~~~~~~~~~~~~

::

     Input Arguments: 
       
           1. To execute single file for compression :  ./build/xil_snappy -sx <compress_decompress xclbin> -c <file_name>
           2. To execute single file for decompression: ./build/xil_snappy -sx <compress_decompress xclbin> -d <file_name.snappy>
           3. To validate various files together:       ./build/xil_snappy -sx <compress_decompress xclbin> -l <files.list>
               3.a. <files.list>: Contains various file names with current path
           4. To execute single file for compression and decompression : ./build/xil_snappy -sx <compress_decompress xclbin> -v <file_name>    
           
      Note: Default arguments are set in Makefile

     Help:

           ===============================================================================================
           Usage: application.exe -[-h-c-l-d-sx-v-B-x]

                   --help                  -h      Print Help Options   Default: [false]
                   --compress              -c      Compress
                   --file_list             -l      List of Input Files
                   --decompresss           -d      Decompress
                   --compress_decompress   -sx     Compress_Decompress binary
                   --validate              -v      Single file validate for Compress and Decompress 
                   --block_size            -B      Compress Block Size [0-64: 1-256: 2-1024: 3-4096] Default: [0]
                   --flow                  -x      Validation [0-All: 1-XcXd: 2-XcSd: 3-ScXd]   Default:[1]
           ===============================================================================================
