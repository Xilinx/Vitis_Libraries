.. CompressionLib_Docs documentation master file, created by
   sphinx-quickstart on Thu Jun 20 14:04:09 2020.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

.. meta::
   :keywords: Vitis, Library, Data Compression, Xilinx, Zlib, LZ4, Snappy, ZLIB, Zstd, FPGA Benchmark, Compression Benchmark
   :description: This page provides benchmarking results of various Vitis Data Compression Applications. Results include throughput and FPGA resources.

==========
Benchmark
==========

Datasets
````````
Benchmark evaluation of compression performance is of reference `Silesia Corpus.
<http://sun.aei.polsl.pl/~sdeor/index.php?page=silesia>`__

Compression Performance
```````````````````````

The following table presents compression ratio (CR), compression kernel throughput, kernel clock frequency met and resource utilization when executed on Alveo U200 and is measured on Silesia Corpus compression benchmark.

+-----------------------------------------------------------------------+-----------+----------------------+-------------------+----------+---------+-------+-------+
| Architecture                                                          | Block Size|  Compression Ratio   |     Throughput    |  FMax    |  LUT    |  BRAM |  URAM |
+=======================================================================+===========+======================+===================+==========+=========+=======+=======+
| `Zstd Compress Quad Core <source/L2/zstd_quadcore_compress.html>`__   |   32KB    |        2.68          |      1.17 GB/s    |  284MHz  |   40K   |  79   |  37   |
+-----------------------------------------------------------------------+-----------+----------------------+-------------------+----------+---------+-------+-------+
| `GZip/Zlib 32KBMemory Mapped <source/L2/gzipc_block_mm.html>`__       |   32KB    |        2.70          |      2 GB/s       |  300MHz  |  57K    |  135  |  64   |
+-----------------------------------------------------------------------+-----------+----------------------+-------------------+----------+---------+-------+-------+
| `GZip 32KB Compress Stream <source/L2/gzipc.html>`__                  |   32KB    |        2.70          |      2 GB/s       |  300MHz  |  54K    |  141  |  64   |
+-----------------------------------------------------------------------+-----------+----------------------+-------------------+----------+---------+-------+-------+
| `Zlib 32KB Compress Stream <source/L2/zlibc.html>`__                  |   32KB    |        2.70          |      2 GB/s       |  300MHz  |  54K    |  128  |  64   |
+-----------------------------------------------------------------------+-----------+----------------------+-------------------+----------+---------+-------+-------+
| `GZip Fixed 32KB Compress Stream <source/L2/gzipc_static.html>`_      |   32KB    |        2.31          |      2 GB/s       |  300MHz  |  35K    |  45   |  64   |
+-----------------------------------------------------------------------+-----------+----------------------+-------------------+----------+---------+-------+-------+
| `Zlib Fixed 32KB Compress Stream <source/L2/zlibc_static.html>`__     |   32KB    |        2.31          |      2 GB/s       |  300MHz  |  35.7K  |  39   |  64   |
+-----------------------------------------------------------------------+-----------+----------------------+-------------------+----------+---------+-------+-------+
| `GZip 16KB Compress Stream <source/L2/gzipc_16KB.html>`_              |   16KB    |        2.62          |      2 GB/s       |  282MHz  |  58K    |  164  |  48   |
+-----------------------------------------------------------------------+-----------+----------------------+-------------------+----------+---------+-------+-------+
| `Zlib 16KB Compress Stream <source/L2/zlibc_16KB.html>`__             |   16KB    |        2.62          |      2 GB/s       |  300MHz  |  58K    |  160  |  48   |
+-----------------------------------------------------------------------+-----------+----------------------+-------------------+----------+---------+-------+-------+
| `GZip 8KB Compress Stream <source/L2/gzipc_8KB.html>`_                |   8KB     |        2.50          |      2 GB/s       |  300MHz  |  57.5K  |  100  |  48   |
+-----------------------------------------------------------------------+-----------+----------------------+-------------------+----------+---------+-------+-------+
| `Zlib 8KB Compress Stream <source/L2/zlibc_8KB.html>`__               |   8KB     |        2.50          |      2 GB/s       |  300MHz  |  57.4K  |  96   |  48   |
+-----------------------------------------------------------------------+-----------+----------------------+-------------------+----------+---------+-------+-------+
| `LZ4 Streaming <source/L2/lz4_compress_streaming.html>`_              |   64KB    |        2.13          |      290 MB/s     |  300MHz  |  3K     |  5    |  6    |
+-----------------------------------------------------------------------+-----------+----------------------+-------------------+----------+---------+-------+-------+
| `Snappy Streaming <source/L2/snappy_streaming.html>`_                 |   64KB    |        2.13          |      290 MB/s     |  300MHz  |  3K     |  4    |  6    |
+-----------------------------------------------------------------------+-----------+----------------------+-------------------+----------+---------+-------+-------+


* The amount of resources used indicate that we still have room on Alveo U200 to go for more compute units which can further improve the throughput.


De-Compression Performance
``````````````````````````

The following table presents decompression kernel throughput achieved with single and 8 engines, 
kernel clock frequency met and resource utilization when executed on Alveo U200.

+-------------------------------------------------------------------------------------+-------------------+----------+---------+-------+------+
| Architecture                                                                        |    Throughput     |  FMax    |  LUT    |  BRAM | URAM |         
+=====================================================================================+===================+==========+=========+=======+======+
| `LZ4 Streaming <source/L2/lz4_dec_streaming_parallelByte8.html>`__                  |     1.8  GB/s     |  300MHz  |  5.5K   |  0    |  4   |
+-------------------------------------------------------------------------------------+-------------------+----------+---------+-------+------+
| `Snappy Streaming <source/L2/snappy_dec_streaming_parallelByte8.html>`__            |     1.97 GB/s     |  300MHz  |  6.5K   |  0    |  4   |
+-------------------------------------------------------------------------------------+-------------------+----------+---------+-------+------+
| `GZip/Zlib Streaming <source/L2/gzip.html>`__                                       |     518  MB/s     |  283MHz  |  6.7K   |  8    |  0   |
+-------------------------------------------------------------------------------------+-------------------+----------+---------+-------+------+
| `ZStd Streaming <source/L2/zstdd_32KB.html>`__                                      |     658.86 MB/s   |  234MHz  |  22K    |  32   |  3   |
+-------------------------------------------------------------------------------------+-------------------+----------+---------+-------+------+
| `ZStd Full File Streaming <source/L2/zstdd_32KB.html>`__                            |     658.86 MB/s   |  234MHz  |  22K    |  32   |  3   |
+-------------------------------------------------------------------------------------+-------------------+----------+---------+-------+------+

* GZip/Zlib Streaming: Full standard support (Dynamic Huffman, Fixed Huffman and Stored Blocks supported).
* ZStd Streaming: Full Standard support with limited Window Size upto 128KB.


Test Overview
`````````````
Here are benchmarks of the Vitis Data Compression Library using the Vitis environment. 

Vitis Data Compression Library
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* **Download code**

These data_compression benchmarks can be downloaded from `vitis libraries <https://github.com/Xilinx/Vitis_Libraries.git>`_ ``master`` branch.

.. code-block:: bash

   git clone https://github.com/Xilinx/Vitis_Libraries.git 
   cd Vitis_Libraries
   git checkout master
   cd data_compression

* **Setup environment**

Specifying the corresponding Vitis, XRT, and path to the platform repository by running following commands.

.. code-block:: bash

   source <Vitis_Intstalled_Path>/installs/lin64/Vitis/2021.2/settings64.sh
   source <Vitis_Installed_Path>/xbb/xrt/packages/setup.sh
   export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
   export LD_LIBRARY_PATH=$XILINX_VITIS/lib/lnx64.o/Default/:$LD_LIBRARY_PATH

* **Build Instructions**

Execute the following commands to compile and test run the applications.

.. code-block:: bash
      
   $ make run TARGET=hw

   hw: run on actual hardware

By default, the target device is set as Alveo U200. In order to target a different
device, use the  ``DEVICE`` argument. For example:

.. code-block:: bash

    make run TARGET=hw DEVICE=<new_device.xpfm>

.. NOTE::
   Build instructions explained in this section are common for all the
   applications to run on actual hardware. The generated executable names may differ.
