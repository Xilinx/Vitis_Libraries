.. CompressionLib_Docs documentation master file, created by
   sphinx-quickstart on Thu Jun 20 14:04:09 2019.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

=================
Benchmark Results
=================

Compression Performance
```````````````````````

Table below presents LZ4/Snappy/Zlib Compression Ratio (CR), compression kernel throughput achieved with single and 8 engines, 
kernel clock frequency met and resource utilization when executed on Alveo U200.
Reported compression ratio is measured on Silesia Corpus compression benchmark.

+---------------------------------------------------+----------------------+--------------+----------+---------+-------+-------+
| Architecture                                      |  Compression Ratio   |  Throughput  |  FMax    |  LUT    |  BRAM |  URAM |             
+===================================================+======================+==============+==========+=========+=======+=======+
| LZ4 Streaming (Single Engine)                     |        2.13          |   287 MB/s   |  300MHz  |  4.5K   |  16   |  6    |
+---------------------------------------------------+----------------------+--------------+----------+---------+-------+-------+
| Snappy Streaming (Single Engine)                  |        2.13          |   260 MB/s   |  270MHz  |  4.5K   |  16   |  6    |
+---------------------------------------------------+----------------------+--------------+----------+---------+-------+-------+
| LZ4 Memory Mapped (8 Engines with Data Movers)    |        2.13          |   1.8 GB/s   |  300MHz  |  51.5K  |  58   |  48   |
+---------------------------------------------------+----------------------+--------------+----------+---------+-------+-------+
| Snappy Memory Mapped (8 Engines with Data Movers) |        2.13          |   1.5 GB/s   |  270MHz  |  52.7K  |  58   |  48   |
+---------------------------------------------------+----------------------+--------------+----------+---------+-------+-------+
| Zlib Memory Mapped (8 Engines with Data Movers)   |        2.78          |   1.5 GB/s   |  244MHz  |  165K   |  175  |  48   |
+---------------------------------------------------+----------------------+--------------+----------+---------+-------+-------+


.. [*] The amount of resources used indicate that we still have room on Alveo U200 to go for more compute units which can further improve the throughput.


De-Compression Performance
``````````````````````````

Table below presents LZ4 decompression kernel throughput achieved with single and 8 engines, 
kernel clock frequency met and resource utilization when executed on Alveo U200.

+---------------------------------------------------------+--------------+----------+---------+-------+
| Architecture                                            |  Throughput  |  FMax    |  LUT    |  BRAM |             
+=========================================================+==============+==========+=========+=======+
| LZ4 Streaming (Single Engine)                           |   293 MB/s   |  300MHz  |  809    |  16   |
+---------------------------------------------------------+--------------+----------+---------+-------+
| Snappy Streaming (Single Engine)                        |   290 MB/s   |  300MHz  |  809    |  16   |
+---------------------------------------------------------+--------------+----------+---------+-------+
| LZ4 Memory Mapped (8 Engines with Data Movers)          |   1.8 GB/s   |  300MHz  |  30.6K  |  146  |
+---------------------------------------------------------+--------------+----------+---------+-------+
| Snappy Memory Mapped (8 Engines with Data Movers)       |   1.8 GB/s   |  300MHz  |  30.6K  |  146  |
+---------------------------------------------------------+--------------+----------+---------+-------+
| Zlib Memory Mapped (Single Engine with Data Movers)     |   250 MB/s   |  297MHz  |  13K    |  42   |
+---------------------------------------------------------+--------------+----------+---------+-------+

.. [*] The amount of resources used indicate that we still have room on Alveo U200 to go for more compute units which can further improve the throughput.
