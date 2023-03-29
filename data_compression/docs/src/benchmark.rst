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

+------------------------------------------------------------------------+-------------+----------------------+-------------------+----------+---------+-------+-------+
| Architecture                                                           | Block Size  |   Compression Ratio  |     Throughput    |  FMax    |  LUT    |  BRAM |  URAM |
+========================================================================+=============+======================+===================+==========+=========+=======+=======+
| `LZ4 Streaming <L2/tests/lz4_compress_streaming>`_                     |     32KB    |        2.13          |      290 MB/s     |  300MHz  |  3K     |  5    |  6    |
+------------------------------------------------------------------------+-------------+----------------------+-------------------+----------+---------+-------+-------+
| `Snappy Streaming <L2/demos/snappy_streaming>`_                        |     32KB    |        2.13          |      290 MB/s     |  300MHz  |  3K     |  4    |  6    |
+------------------------------------------------------------------------+-------------+----------------------+-------------------+----------+---------+-------+-------+
| `GZip/Zlib 32KB Memory Mapped <L2/tests/gzipc_block_mm>`_              |     32KB    |        2.70          |      2 GB/s       |  290MHz  |  53K    |  140  |  64   |
+------------------------------------------------------------------------+-------------+----------------------+-------------------+----------+---------+-------+-------+
| `GZip 32KB Compress Stream <L2/tests/gzipc>`_                          |     32KB    |        2.70          |      2 GB/s       |  300MHz  |  57K    |  142  |  64   |
+------------------------------------------------------------------------+-------------+----------------------+-------------------+----------+---------+-------+-------+
| `GZip 16KB Compress Stream <L2/tests/gzipc_16KB>`_                     |     32KB    |        2.62          |      2 GB/s       |  292MHz  |  62K    |  175  |  48   |
+------------------------------------------------------------------------+-------------+----------------------+-------------------+----------+---------+-------+-------+
| `GZip 8KB Compress Stream <L2/tests/gzipc_8KB>`_                       |     32KB    |        2.50          |      2 GB/s       |  300MHz  |  61K    |  111  |  48   |
+------------------------------------------------------------------------+-------------+----------------------+-------------------+----------+---------+-------+-------+
| `GZip Fixed 32KB Compress Stream <L2/tests/gzipc_static>`_             |     16KB    |        2.31          |      2 GB/s       |  300MHz  |  39K    |  53   |  64   |
+------------------------------------------------------------------------+-------------+----------------------+-------------------+----------+---------+-------+-------+
| `Zlib 32KB Compress Stream <L2/tests/zlibc>`_                          |     16KB    |        2.70          |      2 GB/s       |  300MHz  |  57K    |  131  |  64   |
+------------------------------------------------------------------------+-------------+----------------------+-------------------+----------+---------+-------+-------+
| `Zlib 16KB Compress Stream <L2/tests/zlibc_16KB>`_                     |     8KB     |        2.62          |      2 GB/s       |  300MHz  |  62K    |  165  |  48   |
+------------------------------------------------------------------------+-------------+----------------------+-------------------+----------+---------+-------+-------+
| `Zlib 8KB Compress Stream <L2/tests/zlibc_8KB>`_                       |     8KB     |        2.50          |      2 GB/s       |  300MHz  |  61K    |  101  |  48   |
+------------------------------------------------------------------------+-------------+----------------------+-------------------+----------+---------+-------+-------+
| `Zlib Fixed 32KB Compress Stream <L2/tests/zlibc_static>`_             |     64KB    |        2.31          |      2 GB/s       |  300MHz  |  39K    |  43   |  64   |
+------------------------------------------------------------------------+-------------+----------------------+-------------------+----------+---------+-------+-------+
| `Zstd Compress Quad Core <L2/tests/zstd_quadcore_compress>`_           |     64KB    |        2.68          |     1.17 GB/s     |  275MHz  |  44K    |  94   |  37   |
+------------------------------------------------------------------------+-------------+----------------------+-------------------+----------+---------+-------+-------+


* The amount of resources used indicate that we still have room on Alveo U200 to go for more compute units which can further improve the throughput.


De-Compression Performance
``````````````````````````

The following table presents decompression kernel throughput achieved with single and 8 engines, 
kernel clock frequency met and resource utilization when executed on Alveo U200.

+----------------------------------------------------------------------+-------------------+----------+---------+-------+------+
| Architecture                                                         |    Throughput     |  FMax    |  LUT    |  BRAM | URAM |           
+======================================================================+===================+==========+=========+=======+======+
| `LZ4 Streaming <L2/tests/lz4_dec_streaming_parallelByte8>`_          |     1.8  GB/s     |  292MHz  |  11K    |  15   |  2   |
+----------------------------------------------------------------------+-------------------+----------+---------+-------+------+
| `Snappy Streaming <L2/tests/snappy_dec_streaming_parallelByte8>`_    |     1.97 GB/s     |  300MHz  |  12K    |  15   |  2   |
+----------------------------------------------------------------------+-------------------+----------+---------+-------+------+
| `GZip/Zlib Streaming <L2/demos/gzip>`_                               |     518  MB/s     |  283MHz  |  6.7K   |  8    |  0   |
+----------------------------------------------------------------------+-------------------+----------+---------+-------+------+
| `ZStd Streaming <L2/tests/zstdd_32KB>`_                              |   658.86 MB/s     |  240MHz  |  23K    |  34   |  3   |
+----------------------------------------------------------------------+-------------------+----------+---------+-------+------+

* GZip/Zlib Streaming: Full standard support (Dynamic Huffman, Fixed Huffman and Stored Blocks supported).
* ZStd Streaming: Full Standard support with limited Window Size upto 128KB.


Test Overview
`````````````
Here are benchmarks of the Vitis Data Compression Library using the Vitis environment. 

Vitis Data Compression Library
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* **Download code**

These data_compression benchmarks can be downloaded from `vitis libraries <https://github.com/Xilinx/Vitis_Libraries.git>`_ ``main`` branch.

.. code-block:: bash

   git clone https://github.com/Xilinx/Vitis_Libraries.git 
   cd Vitis_Libraries
   git checkout master
   cd data_compression

* **Setup environment**

Specifying the corresponding Vitis, XRT, and path to the platform repository by running following commands.

.. code-block:: bash

   source <Vitis_Intstalled_Path>/installs/lin64/Vitis/2022.2/settings64.sh
   source <Vitis_Installed_Path>/xbb/xrt/packages/setup.sh
   export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
   export LD_LIBRARY_PATH=$XILINX_VITIS/lib/lnx64.o/Default/:$LD_LIBRARY_PATH

* **Build Instructions**

Execute the following commands to compile and test run the applications.

.. code-block:: bash
      
   $ make run TARGET=hw

   hw: run on actual hardware

By default, the target device is set as Alveo U200. In order to target a different
device, use the  ``PLATFORM`` argument. For example:

.. code-block:: bash

    make run TARGET=hw PLATFORM=<new_device.xpfm>

.. NOTE::
   Build instructions explained in this section are common for all the
   applications to run on actual hardware. The generated executable names may differ.
