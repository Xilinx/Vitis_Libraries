.. CompressionLib_Docs documentation master file, created by
   sphinx-quickstart on Thu Jun 20 14:04:09 2019.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

=================
Benchmark Results
=================

Performance
```````````

Table below presents the comparison of LZ4 and Snappy throughput on CPU vs best end to end compression throughput achieved with two compute units when executed on Alveo U200.

+-----------------------------+---------------------------------+------------------+------------------+
| Compression Algorithm       | CPU Corei7 9700K CPU @4.9GHz    |   Alveo U200     |  Speed Up        |  
+=============================+=================================+==================+==================+
| LZ4 Default (v1.9.0)        |                780 MB/s         |   2.81 GB/s      |    3.60x         |
+-----------------------------+---------------------------------+------------------+------------------+
| Snappy 1.1.4                |                565 MB/s         |   2.5 GB/s       |    4.42x         |
+-----------------------------+---------------------------------+------------------+------------------+


Resource Utilization
````````````````````

Table below presents resource utilization of Xilinx LZ4 and Snappy compress kernels with 8 engines for two compute units on Alveo U200.

.. note:: The percentage of resources used clearly indicate that we can still have room on Alveo U200 to go for more compute units which can further improve the throughput.

+-----------------------------+-----------------+----------------+---------------+-------------+
| Compression Algorithm       |       LUT       |     REG        |    BRAM       |  URAM       |
+=============================+=================+================+===============+=============+
| LZ4 Default (v1.9.0)        | 103061 [10.34%] | 127686 [6.09%] | 116 [6.59%]   | 96 [10.00%] |  
+-----------------------------+-----------------+----------------+---------------+-------------+
| Snappy 1.1.4                | 105829 [10.62%] | 129695 [6.18%] | 292 [16.60%]  | 96 [10.00%] | 
+-----------------------------+-----------------+----------------+---------------+-------------+
