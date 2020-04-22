.. CompressionLib_Docs documentation master file, created by
   sphinx-quickstart on Thu Jun 20 14:04:09 2019.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

.. meta::
   :keywords: Vitis, Library, Data Compression, Xilinx, Zlib, LZ4, Snappy, ZLIB, FPGA Benchmark, Compression Benchmark
   :description: This page provides benchmarking results of various Vitis Data Compression Applications. Results include throughput and FPGA resources.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

=================
Benchmark Results
=================

Compression Performance
```````````````````````

Table below presents compression Ratio (CR), compression kernel throughput achieved with single and 8 engines, 
kernel clock frequency met and resource utilization when executed on Alveo U200.

Reported compression ratio is measured on Silesia Corpus compression benchmark.

+-----------------------------------------------------------------+----------------------+--------------+----------+---------+-------+-------+
| Architecture                                                    |  Compression Ratio   |  Throughput  |  FMax    |  LUT    |  BRAM |  URAM |
+=================================================================+======================+==============+==========+=========+=======+=======+
| LZ4 Streaming (Single Engine)                                   |        2.13          |   287 MB/s   |  300MHz  |  4.1K   |  4    |  6    |
+-----------------------------------------------------------------+----------------------+--------------+----------+---------+-------+-------+
| Snappy Streaming (Single Engine)                                |        2.13          |   260 MB/s   |  300MHz  |  2.9K   |  4    |  6    |
+-----------------------------------------------------------------+----------------------+--------------+----------+---------+-------+-------+
| LZ4 Memory Mapped (8 Engines with Data Movers)                  |        2.13          |   1.8 GB/s   |  300MHz  |  51.5K  |  58   |  48   |
+-----------------------------------------------------------------+----------------------+--------------+----------+---------+-------+-------+
| Snappy Memory Mapped (8 Engines with Data Movers)               |        2.13          |   1.5 GB/s   |  300MHz  |  53.37K |  50   |  48   |
+-----------------------------------------------------------------+----------------------+--------------+----------+---------+-------+-------+
| Zlib Memory Mapped (Dynamic Huffman, 8 Engines with Data Movers)|        2.74          |   1.4 GB/s   |  287MHz  |  132K   |  140  |  48   |
+-----------------------------------------------------------------+----------------------+--------------+----------+---------+-------+-------+


.. [*] The amount of resources used indicate that we still have room on Alveo U200 to go for more compute units which can further improve the throughput.


De-Compression Performance
``````````````````````````

Table below presents decompression kernel throughput achieved with single and 8 engines, 
kernel clock frequency met and resource utilization when executed on Alveo U200.

+----------------------------------------------------------------------+--------------+----------+---------+-------+------+
| Architecture                                                         |  Throughput  |  FMax    |  LUT    |  BRAM | URAM |           
+======================================================================+==============+==========+=========+=======+======+
| LZ4 Streaming (Single Engine and Datawidth: 32bit)                   |   797.5 MB/s |  258MHz  |  3.3K   |  0    |  8   |
+----------------------------------------------------------------------+--------------+----------+---------+-------+------+
| LZ4 Streaming (Single Engine and Datawidth: 64bit)                   |   1420 MB/s  |  253MHz  |  6.2K   |  0    |  4   |
+----------------------------------------------------------------------+--------------+----------+---------+-------+------+
| Snappy Streaming (Single Engine)                                     |   290 MB/s   |  300MHz  |  878    |  16   |  0   |
+----------------------------------------------------------------------+--------------+----------+---------+-------+------+
| Zlib Streaming (High Throughput, Datawidth: 64bit)                   |   1183 MB/s  |  240MHz  |  11.7K  |  3    |  2   |
+----------------------------------------------------------------------+--------------+----------+---------+-------+------+
| Zlib Streaming (High FMax, Datawidth: 32bit)                         |   745 MB/s   |  300MHz  |  9K     |  3    |  4   |
+----------------------------------------------------------------------+--------------+----------+---------+-------+------+
| LZ4 Memory Mapped (8 Engines with Data Movers)                       |   1.8 GB/s   |  300MHz  |  30.6K  |  146  |  0   |
+----------------------------------------------------------------------+--------------+----------+---------+-------+------+
| Snappy Memory Mapped (8 Engines with Data Movers)                    |   1.8 GB/s   |  300MHz  |  31.1K  |  146  |  0   |
+----------------------------------------------------------------------+--------------+----------+---------+-------+------+

.. [*] The amount of resources used indicate that we still have room on Alveo U200 to go for more compute units which can further improve the throughput.
.. [*] Zlib Streaming: Dynamic Huffman and Single Engine performance is provided   
