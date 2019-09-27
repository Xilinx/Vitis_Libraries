.. CompressionLib_Docs documentation master file, created by
   sphinx-quickstart on Thu Jun 20 14:04:09 2019.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

=================
Benchmark Results
=================

Compression Performance
```````````````````````

Table below presents LZ4 and Snappy Compression Ratio (CR), compression **best kernel throughput** achieved with **8 parallel engines**, 
kernel clock frequency met and resource utilization when executed on Alveo U200.


.. note:: The amount of resources used indicate that we still have room on Alveo U200 to go for more compute units which can further improve the throughput.

+--------------------------------------------------+-------+--------------+----------+---------+-------+-------+
|                   Architecture                   |  CR   |  Throughput  |  FMax    |  LUT    |  BRAM |  URAM |             
+==================================================+=======+==============+==========+=========+=======+=======+
| LZ4 Streaming (Single Engine)                    | 2.13  |   285 MB/s   |  300MHz  |  4.5K   |  16   |  6    |
+--------------------------------------------------+-------+--------------+----------+---------+-------+-------+
| Snappy Streaming(Single Engine)                  | 2.13  |   260 MB/s   |  270MHz  |  4.5K   |  16   |  6    |
+--------------------------------------------------+-------+--------------+----------+---------+-------+-------+
| LZ4 Memory Mapped (8 Engines with Data Movers)   | 2.13  |   1.8 GB/s   |  300MHz  |  51.5K  |  58   |  48   |
+--------------------------------------------------+-------+--------------+----------+---------+-------+-------+
| Snappy Memory Mapped (8 Engines with Data Movers)| 2.13  |   1.5 GB/s   |  270MHz  |  52.7K  |  58   |  48   |
+--------------------------------------------------+-------+--------------+----------+---------+-------+-------+


De-Compression Performance
``````````````````````````

Table below presents LZ4 and Snappy decompression **best kernel throughput** achieved with **8 parallel engines**, 
kernel clock frequency met and resource utilization when executed on Alveo U200.


.. note:: The amount of resources used indicate that we still have room on Alveo U200 to go for more compute units which can further improve the throughput.

+--------------------------------------------------+--------------+----------+---------+-------+
|                   Architecture                   |  Throughput  |  FMax    |  LUT    |  BRAM |             
+==================================================+==============+==========+=========+=======+
| LZ4 Streaming (Single Engine)                    |   292 MB/s   |  300MHz  |  809    |  16   |
+--------------------------------------------------+--------------+----------+---------+-------+
| Snappy Streaming(Single Engine)                  |   290 MB/s   |  300MHz  |  809    |  16   |
+--------------------------------------------------+--------------+----------+---------+-------+
| LZ4 Memory Mapped (8 Engines with Data Movers)   |   1.8 GB/s   |  300MHz  |  30.6K  |  146  |
+--------------------------------------------------+--------------+----------+---------+-------+
| Snappy Memory Mapped (8 Engines with Data Movers)|   1.8 GB/s   |  300MHz  |  30.7K  |  146  |
+--------------------------------------------------+--------------+----------+---------+-------+
