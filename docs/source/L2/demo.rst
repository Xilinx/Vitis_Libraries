.. CompressionLib_Docs documentation master file, created by
   sphinx-quickstart on Thu Jun 20 14:04:09 2019.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

.. _l2_demo:

====
Demo
====

Demo examples for lz4, snappy, lz4_streaming, zlib and gzip kernels are present in **L2/demos/** directory.

Before building any of the examples, following commands need to be executed:

.. code-block:: bash
   
   $ source <Vitis_Installed_Path>/installs/lin64/Vitis/2019.2/settings64.sh
   $ source <Vitis_Installed_Path>/xbb/xrt/packages/setenv.sh

Build Instructions
------------------

To compile and test run this example execute the following commands:

.. code-block:: bash
   
   $ make run TARGET=sw_emu

Variable ``TARGET`` can take the following values:

	- **sw_emu**	: software emulation
	
	- **hw_emu**	: hardware emulation
	
	- **hw**	: run on actual hardware

By default the target device is set as Alveo U200. In order to target a different
device use "DEVICE" argument. Example below explains the same.

.. code-block:: bash

    make run TARGET=sw_emu DEVICE=<new_device.xpfm>

Build instructions explained in this section are common for all the
applications but the generated executable names differ.


LZ4 Compression and Decompression
---------------------------------

Lz4 example resides in ``L2/demos/lz4`` directory. 

Follow build instructions to generate host executable and binary.

The binary host file generated is named as "**xil_lz4**" and it is present in ``./build`` directory.

Executable Usage:

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

Performance Data
````````````````

+----------------------------+------------------------+
| Topic                      | Best Kernel Throughput |
+============================+========================+
|  Compression               | 1.8 GB/s               |
+----------------------------+------------------------+
| Decompression              | 1.8GB/s                |
+----------------------------+------------------------+
| Average Compression Ratio  | 2.13 (Silesia Corpus)  |
+----------------------------+------------------------+

Snappy Compression and Decompression
------------------------------------

Snappy example resides in ``L2/demos/snappy`` directory. 

Follow build instructions to generate host executable and binary.

The binary host file generated is named as "**xil_snappy**" and it is present in ``./build`` directory.

Executable Usage:

1. To execute single file for compression 	: ``./build/xil_snappy -cx <compress xclbin> -c <file_name>``

2. To execute single file for decompression	: ``./build/xil_snappy -dx <decompress xclbin> -d <file_name.snappy>``

3. To validate various files together		: ``./build/xil_snappy -cx <compress xclbin> -dx <decompress xclbin> -l <files.list>``
	
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

Performance Data
```````````````

+----------------------------+------------------------+
| Topic                      | Best Kernel Throughput |
+============================+========================+
|  Compression               | 1.5 GB/s               |
+----------------------------+------------------------+
| Decompression              | 1.8GB/s                |
+----------------------------+------------------------+
| Average Compression Ratio  | 2.15 (Silesia Corpus)  |
+----------------------------+------------------------+

LZ4-Streaming Compression and Decompression
-------------------------------------------

LZ4-Streaming example resides in ``L2/demos/lz4_streaming`` directory. To compile and test run this example execute the following commands:

Follow build instructions to generate host executable and binary.

The binary host file generated is named as "**xil_lz4_streaming**", using `PARALLEL_BLOCK` value of 8 (default), and present in ``./build`` directory.

Executable Usage:

1. To execute single file for compression 	: ``./build/xil_lz4_streaming -cx <compress xclbin> -c <file_name>``

2. To execute single file for decompression	: ``./build/xil_lz4_streaming -dx <decompress xclbin> -d <file_name.lz4>``

3. To validate various files together		: ``./build/xil_lz4_streaming -cx <compress xclbin> -dx <decompress xclbin> -l <files.list>``
	
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

ZLIB Compression and Decompression
---------------------------------

ZLIB example resides in ``L2/demos/zlib`` directory. 

Follow build instructions to generate host executable and binary.

The binary host file generated is named as "**xil_zlib**" and it is present in ``./build`` directory.

Executable Usage:

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

Performance Data
````````````````

+----------------------------+------------------------+
| Topic                      | Best Kernel Throughput |
+============================+========================+
|  Compression               | 1.5 GB/s               |
+----------------------------+------------------------+
| Decompression              | 250 MB/s               |
+----------------------------+------------------------+
| Average Compression Ratio  | 2.78 (Silesia Corpus)  |
+----------------------------+------------------------+

GZip Compression and Decompression
---------------------------------

GZip example resides in ``L2/demos/gzip`` directory. 

Follow build instructions to generate host executable and binary.

The binary host file generated is named as "**xil_gzip**" and present in ``./build`` directory.

Executable Usage:

1. To execute single file for compression 	          : ``./build/xil_gzip -sx ./build/xclbin_<dsa_name>_<TARGET mode>/compress_decompress.xclbin -c <input file_name>``
2. To execute single file for decompression           : ``./build/xil_gzip -sx ./build/xclbin_<dsa_name>_<TARGET mode>/compress_decompress.xclbin -d <compressed file_name>``
3. To validate single file (compress & decompress)    : ``./build/xil_gzip -sx ./build/xclbin_<dsa_name>_<TARGET mode>/compress_decompress.xclbin -v <input file_name>``
4. To validate multiple files (compress & decompress) : ``./build/xil_gzip -sx ./build/xclbin_<dsa_name>_<TARGET mode>/compress_decompress.xclbin -l <files.list>``

	- ``<files.list>``: Contains various file names with current path

Standard Compliance:

.. code-block:: bash

    1. Vitis Library GZip Compressed data can be decoded using standard with
       this command "gzip -dc <compressed_input.gz> > out". Generated output is
       original raw file.
    2. At present standard GZip compressed stream can't be decompressed using 
       Vitis library GZip decompressor.

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

Performance Data
````````````````

+----------------------------+------------------------+
| Topic                      | Best Kernel Throughput |
+============================+========================+
|  Compression               | 1.5 GB/s               |
+----------------------------+------------------------+
| Decompression              | 250 MB/s               |
+----------------------------+------------------------+
| Average Compression Ratio  | 2.78 (Silesia Corpus)  |
+----------------------------+------------------------+

