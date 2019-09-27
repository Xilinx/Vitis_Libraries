.. CompressionLib_Docs documentation master file, created by
   sphinx-quickstart on Thu Jun 20 14:04:09 2019.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

=================
Benchmark Results
=================

Compression Performance
```````````````````````

Table below presents LZ4 Compression Ratio (CR), compression kernel throughput achieved with single and 8 engines, 
kernel clock frequency met and resource utilization when executed on Alveo U200.


.. note:: The amount of resources used indicate that we still have room on Alveo U200 to go for more compute units which can further improve the throughput.

+----------------------+----------------------+--------------+----------+---------+-------+-------+
| Architecture         |  Compression Ratio   |  Throughput  |  FMax    |  LUT    |  BRAM |  URAM |             
+======================+======================+==============+==========+=========+=======+=======+
| LZ4 Streaming*       |        2.13          |   287 MB/s   |  300MHz  |  4.5K   |  16   |  6    |
+----------------------+----------------------+--------------+----------+---------+-------+-------+
| LZ4 Memory Mapped**  |        2.13          |   1.8 GB/s   |  300MHz  |  51.5K  |  58   |  48   |
+----------------------+----------------------+--------------+----------+---------+-------+-------+

*  Single Engine 
** 8 Engines with Data Movers


De-Compression Performance
``````````````````````````

Table below presents LZ4 decompression kernel throughput achieved with single and 8 engines, 
kernel clock frequency met and resource utilization when executed on Alveo U200.


.. note:: The amount of resources used indicate that we still have room on Alveo U200 to go for more compute units which can further improve the throughput.

+----------------------+--------------+----------+---------+-------+
| Architecture         |  Throughput  |  FMax    |  LUT    |  BRAM |             
+======================+==============+==========+=========+=======+
| LZ4 Streaming*       |   293 MB/s   |  300MHz  |  809    |  16   |
+----------------------+--------------+----------+---------+-------+
| LZ4 Memory Mapped**  |   1.8 GB/s   |  300MHz  |  30.6K  |  146  |
+----------------------+--------------+----------+---------+-------+

*  Single Engine 
** 8 Engines with Data Movers
