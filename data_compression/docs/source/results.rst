.. CompressionLib_Docs documentation master file, created by
   sphinx-quickstart on Thu Jun 20 14:04:09 2019.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

=================
Benchmark Results
=================

Performance
```````````

Table below presents LZ4 and Snappy compression/decompression **best kernel throughput** achieved with **8 parallel engines** when executed on Alveo U200.

+-----------------------+------------------+-----------------+
| Data Compression      |   Compression    |  Decompression  |
+=======================+==================+=================+
| LZ4                   |   1.8 GB/s       |   1.8 GB/s      |
+-----------------------+------------------+-----------------+
| Snappy                |   1.5 GB/s       |   1.8 GB/s      |
+-----------------------+------------------+-----------------+

Table below presents LZ4 and Snappy compression/decompression **best streaming kernel throughput** achieved with **single engine** when executed on Alveo U200.

+-----------------------+------------------+-----------------+
| Data Compression      |   Compression    |  Decompression  |
+=======================+==================+=================+
| LZ4                   |   285 MB/s       |   290 MB/s      |
+-----------------------+------------------+-----------------+
| Snappy                |   260 MB/s       |   290 MB/s      |
+-----------------------+------------------+-----------------+

Compression Ratio
`````````````````

Table below presents LZ4 and Snappy compression ratio achieved.

+-----------------------+-----------------------+
| Data Compression      |   Compression Ratio   |
+=======================+=======================+
| LZ4                   |   2.13                |
+-----------------------+-----------------------+
| Snappy                |   2.15                |
+-----------------------+-----------------------+

Resource Utilization
````````````````````

Table below presents resource utilization of Xilinx LZ4 and Snappy compress/decompress kernels with 8 engines on Alveo U200.

.. note:: The amount of resources used indicate that we still have room on Alveo U200 to go for more compute units which can further improve the throughput.

+-------------------+------------+----------+---------+-----------+
| LZ4               |   LUT      |   REG    |  BRAM   |  URAM     |
+===================+============+==========+=========+===========+
| Compression       |  51.5K     |  63.8K   |   58    |   48      |  
+-------------------+------------+----------+---------+-----------+
| Decompression     |  30.6K     |  39.5K   |   146   |   0       |  
+-------------------+------------+----------+---------+-----------+


+-------------------+------------+----------+---------+-----------+
| Snappy            |   LUT      |   REG    |  BRAM   |  URAM     |
+===================+============+==========+=========+===========+
| Compression       |  52.7K     |  64.9K   |  146    |   48      |  
+-------------------+------------+----------+---------+-----------+
| Decompression     |  30.7K     |  39.5K   |  146    |   0       |  
+-------------------+------------+----------+---------+-----------+


Table below presents resource utilization of Xilinx LZ4 and Snappy compress/decompress **streaming kernels** with single engine on Alveo U200.

+-------------------+------------+----------+---------+-----------+
| LZ4 Streaming     |   LUT      |   REG    |  BRAM   |  URAM     |
+===================+============+==========+=========+===========+
| Compression       |  4.5K      |  3.8K    |   16    |   6       |  
+-------------------+------------+----------+---------+-----------+
| Decompression     |   809      |  1K      |   16    |   0       |  
+-------------------+------------+----------+---------+-----------+

+-------------------+------------+----------+---------+-----------+
| Snappy Streaming  |   LUT      |   REG    |  BRAM   |  URAM     |
+===================+============+==========+=========+===========+
| Compression       |  4.5K      |  3.8K    |   16    |   6       |  
+-------------------+------------+----------+---------+-----------+
| Decompression     |   809      |  1K      |   16    |   0       |  
+-------------------+------------+----------+---------+-----------+
