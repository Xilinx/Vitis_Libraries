.. CompressionLib_Docs documentation master file, created by
   sphinx-quickstart on Thu Jun 20 14:04:09 2020.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

.. meta::
   :keywords: Vitis, Library, Data Compression, Xilinx, Zlib, LZ4, Snappy, ZLIB, FPGA Benchmark, Compression Benchmark
   :description: This page provides benchmarking results of various Vitis Data Compression Applications. Results include throughput and FPGA resources.

=================
Benchmark Results
=================

Compression Performance
```````````````````````

The following table presents compression ratio (CR), compression kernel throughput achieved with single and 8 engines, 
kernel clock frequency met and resource utilization when executed on Alveo U200.

Reported compression ratio is measured on Silesia Corpus compression benchmark.

+-----------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+
| Architecture                                                    |  Compression Ratio   |  Best Throughput  |  FMax    |  LUT    |  BRAM |  URAM |
+=================================================================+======================+===================+==========+=========+=======+=======+
| LZ4 Streaming (Single Engine)                                   |        2.13          |      287 MB/s     |  300MHz  |  4.1K   |  4    |  6    |
+-----------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+
| Snappy Streaming (Single Engine)                                |        2.13          |      260 MB/s     |  300MHz  |  2.9K   |  4    |  6    |
+-----------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+
| LZ4 Memory Mapped (8 Engines with Data Movers)                  |        2.13          |      2.2 GB/s     |  295MHz  |  47K    |  56   |  48   |
+-----------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+
| Snappy Memory Mapped (8 Engines with Data Movers)               |        2.13          |      2.2 GB/s     |  300MHz  |  47K    |  48   |  48   |
+-----------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+
| Zlib Memory Mapped (Dynamic Huffman, 8 Engines with Data Movers)|        2.74          |      2 GB/s       |  284MHz  |  82.7K  |  105  |  48   |
+-----------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+
| GZip Memory Mapped (Dynamic Huffman, 8 Engines with Data Movers)|        2.74          |      2 GB/s       |  284MHz  |  82.7K  |  105  |  48   |
+-----------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+


.. [*] The amount of resources used indicate that we still have room on Alveo U200 to go for more compute units which can further improve the throughput.


De-Compression Performance
``````````````````````````

The following table presents decompression kernel throughput achieved with single and 8 engines, 
kernel clock frequency met and resource utilization when executed on Alveo U200.

+----------------------------------------------------------------------+-------------------+----------+---------+-------+------+
| Architecture                                                         |  Best Throughput  |  FMax    |  LUT    |  BRAM | URAM |           
+======================================================================+===================+==========+=========+=======+======+
| LZ4 Streaming (Single Engine and Datawidth: 64bit)                   |     1.42 GB/s     |  253MHz  |  6.2K   |  0    |  4   |
+----------------------------------------------------------------------+-------------------+----------+---------+-------+------+
| Snappy Streaming (Single Engine and Datawidth: 64bit)                |     1.58 GB/s     |  300MHz  |  7.8K   |  0    |  4   |
+----------------------------------------------------------------------+-------------------+----------+---------+-------+------+
| Zlib Streaming (High Throughput, Datawidth: 64bit)                   |     1.18 GB/s     |  227MHz  |  11.6K  |  3    |  2   |
+----------------------------------------------------------------------+-------------------+----------+---------+-------+------+
| GZip Streaming (High Throughput, Datawidth: 64bit)                   |     1.18 GB/s     |  227MHz  |  13.9K  |  4    |  2   |
+----------------------------------------------------------------------+-------------------+----------+---------+-------+------+
| LZ4 Memory Mapped (8 Engines with Data Movers)                       |     1.8  GB/s     |  300MHz  |  30.6K  |  146  |  0   |
+----------------------------------------------------------------------+-------------------+----------+---------+-------+------+
| Snappy Memory Mapped (8 Engines with Data Movers)                    |     1.8  GB/s     |  300MHz  |  31.1K  |  146  |  0   |
+----------------------------------------------------------------------+-------------------+----------+---------+-------+------+

.. [*] The amount of resources used indicate that we still have room on Alveo U200 to go for more compute units which can further improve the throughput.
.. [*] Zlib Streaming: Dynamic Huffman and Single Engine performance is provided   
.. [*] GZip Streaming: Full standard support (Dynamic Huffman, Fixed Huffman and Stored Blocks supported)
