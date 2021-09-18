.. CompressionLib_Docs documentation master file, created by
   sphinx-quickstart on Thu Jun 20 14:04:09 2020.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

.. meta::
   :keywords: Vitis, Library, Data Compression, Xilinx, Zlib, LZ4, Snappy, ZLIB, Zstd, FPGA Benchmark, Compression Benchmark
   :description: This page provides benchmarking results of various Vitis Data Compression Applications. Results include throughput and FPGA resources.

=================
Benchmark Results
=================

Datasets
````````
Benchmark evaluation of compression performance is of reference `Silesia Corpus.
<http://sun.aei.polsl.pl/~sdeor/index.php?page=silesia>`__

Compression Performance
```````````````````````

The following table presents compression ratio (CR), compression kernel throughput achieved with single and 8 engines, 
kernel clock frequency met and resource utilization when executed on Alveo U200.

Reported compression ratio is measured on Silesia Corpus compression benchmark.

+---------------------------------------------------------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+
| Architecture                                                                                                  |  Compression Ratio   |     Throughput    |  FMax    |  LUT    |  BRAM |  URAM |
+===============================================================================================================+======================+===================+==========+=========+=======+=======+
| `LZ4 Streaming <L2/lz4_compress_streaming.html>`__ (Single Engine and Datawidth: 8bit)                        |        2.13          |      290 MB/s     |  300MHz  |  3.2K   |  5    |  6    |
+---------------------------------------------------------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+
| `Snappy Streaming <L2/snappy_streaming.html>`__ (Single Engine and Datawidth: 8bit)                           |        2.13          |      290 MB/s     |  300MHz  |  3.1K   |  4    |  6    |
+---------------------------------------------------------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+
| `LZ4 Memory Mapped <L2/lz4_compress.html>`__ (8 Engines with Data Movers)                                     |        2.13          |      2.2 GB/s     |  295MHz  |  47K    |  56   |  48   |
+---------------------------------------------------------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+
| `Snappy Memory Mapped <L2/snappy_compress.html>`__ (8 Engines with Data Movers)                               |        2.13          |      2.2 GB/s     |  300MHz  |  47K    |  48   |  48   |
+---------------------------------------------------------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+
| `GZip/Zlib Memory Mapped <L2/gzipc_block_mm.html>`__ (Dynamic Huffman, 8 Engines with Data Movers)            |        2.67          |      2 GB/s       |  285MHz  |  53.8K  |  75   |  72   |
+---------------------------------------------------------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+
| `GZip/Zlib Compress Stream <L2/gzipc.html>`__ (Dynamic Huffman, 8 Engines with Data Movers)                   |        2.67          |      2 GB/s       |  285MHz  |  49.7K  |  67   |  72   |
+---------------------------------------------------------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+
| `GZip/Zlib Fixed Huffman File Compress Stream <L2/gzipc_static.html>`__                                       |        2.25          |      2 GB/s       |  285MHz  |  34.6K  |  48   |  64   |
+---------------------------------------------------------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+


.. [*] The amount of resources used indicate that we still have room on Alveo U200 to go for more compute units which can further improve the throughput.


De-Compression Performance
``````````````````````````

The following table presents decompression kernel throughput achieved with single and 8 engines, 
kernel clock frequency met and resource utilization when executed on Alveo U200.

+-------------------------------------------------------------------------------------------------------------------------+-------------------+----------+---------+-------+------+
| Architecture                                                                                                            |    Throughput     |  FMax    |  LUT    |  BRAM | URAM |           
+=========================================================================================================================+===================+==========+=========+=======+======+
| `LZ4 Streaming <L2/lz4_dec_streaming_parallelByte8.html>`__ (Single Engine and Datawidth: 64bit)                        |     1.8  GB/s     |  300MHz  |  7.2K   |  0    |  4   |
+-------------------------------------------------------------------------------------------------------------------------+-------------------+----------+---------+-------+------+
| `Snappy Streaming <L2/snappy_dec_streaming_parallelByte8.html>`__ (Single Engine and Datawidth: 64bit)                  |     1.97 GB/s     |  300MHz  |  8.8K   |  0    |  4   |
+-------------------------------------------------------------------------------------------------------------------------+-------------------+----------+---------+-------+------+
| `GZip/Zlib Streaming <L2/gzip_decompress.html>`__ (High Throughput, Datawidth: 64bit)                                   |     450  MB/s     |  252MHz  |  11.3K  |  6    |  3   |
+-------------------------------------------------------------------------------------------------------------------------+-------------------+----------+---------+-------+------+
| `ZStd Streaming <L2/zstd_decompress.html>`__ (Single Engine and Datawidth: 32bit)                                       |     463  MB/s     |  232MHz  |  18K    |  52   |  4   |
+-------------------------------------------------------------------------------------------------------------------------+-------------------+----------+---------+-------+------+
| `ZStd Full File Streaming <L2/zstd_decompress.html>`__ (Single Engine with Datawidth: 32bit)                            |     463  MB/s     |  232MHz  |  22K    |  52   |  4   |
+-------------------------------------------------------------------------------------------------------------------------+-------------------+----------+---------+-------+------+

.. [*] 1. The amount of resources used indicate that we still have room on Alveo U200 to go for more compute units which can further improve the throughput. 
      2. GZip/Zlib Streaming: Full standard support (Dynamic Huffman, Fixed Huffman and Stored Blocks supported).
      3. ZStd Streaming: Full Standard support with limited Window Size upto 128KB.


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

   source <Vitis_Intstalled_Path>/installs/lin64/Vitis/2020.2/settings64.sh
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
