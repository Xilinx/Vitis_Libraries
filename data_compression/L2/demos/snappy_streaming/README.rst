=====================================================
Xilinx Snappy-Streaming Compression and Decompression
=====================================================

LZ4-Streaming example resides in ``L2/demos/snappy_streaming`` directory. To compile and test run this example execute the following commands:

Follow build instructions to generate host executable and binary.

The binary host file generated is named as "**xil_snappy_streaming**", using `PARALLEL_BLOCK` value of 8 (default), and present in ``./build`` directory.

Results
-------

Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~

Table below presents resource utilization of Xilinx Snappy Streaming 
compress/decompress kernels (excluding data movers). It achieves Fmax of 300MHz 

========== ===== ====== ==== ===== ===== 
Flow       LUT   LUTMem REG  BRAM  URAM 
========== ===== ====== ==== ===== ===== 
Compress   2.9K  112    3.2K 4     6     
---------- ----- ------ ---- ----- ----- 
DeCompress 878   31     983  16    0     
========== ===== ====== ==== ===== ===== 

Performance Data
~~~~~~~~~~~~~~~~

Table below presents best kernel throughput achieved for a single compute
unit (Single Engine). 

============================= =========================
Topic                         Results
============================= =========================
Best Compression Throughput   260 MB/s
Best Decompression Throughput 290 MB/s
Average Compression Ratio     2.13x (Silesia Benchmark)
============================= =========================

Note: Overall throughput can still be increased with multiple compute units.

Usage
-----

Execution Steps
~~~~~~~~~~~~~~~

1. To execute single file for compression 	: ``./build/xil_snappy_streaming -cx <compress xclbin> -c <file_name>``

2. To execute single file for decompression	: ``./build/xil_snappy_streaming -dx <decompress xclbin> -d <file_name.snappy>``

3. To validate various files together		: ``./build/xil_snappy_streaming -cx <compress xclbin> -dx <decompress xclbin> -l <files.list>``
	
	- ``<files.list>``: Contains various file names with current path

The usage of the generated executable is as follows:

.. code-block:: bash
   
   Usage: application.exe -[-h-c-l-d-B-x]
        --help,             -h      Print Help Options   Default: [false]
        --compress,         -c      Compress
    	--compress_xclbin   -cx     Compress binary
        --file_list,        -l      List of Input Files
    	--decompress_xclbin -dx     Decompress binary
        --decompress,       -d      Decompress
        --block_size,       -B      Compress Block Size [0-64: 1-256: 2-1024: 3-4096] Default: [0]
        --flow,             -x      Validation [0-All: 1-XcXd: 2-XcSd: 3-ScXd] Default: [1]
