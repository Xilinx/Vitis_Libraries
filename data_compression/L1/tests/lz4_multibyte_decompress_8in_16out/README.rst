.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

AMD LZ4 Multibyte Decompress HLS Test
========================================

**Description:** Test design to validate an other variant implementation of LZ4 Multi Byte Decompress Module for decoding lz4 files with high compression ratio. For standard classical sequences, the maximum performance improvement can reach 48%, and the average performance improvement can also reach 16%.

**Top Function:** lz4DecompressEngineRun

Results of resource usage
-------

======================== ========= ========= ===== ===== 
Module                   LUT       FF        BRAM  URAM 
lz4_decompress_test      6.7K      4.5K      0     4 
======================== ========= ========= ===== ===== 

Results of performance improvement
-------

======= ================= ======================
Silesia	compression ratio	Throughput improvement
dickens	1.58			18%
mozilla	1.94			19%
mr	1.83			21%
nci	6.06			48%
ooffice	1.42			13%
osdb	1.92			2%
reymont	2.08			22%
samba	2.80			18%
sao	1.07			-5%
webster	2.06			12%
xml	4.35			28%
x-ray	1.01			-4%
average	2.34			16%
======= ================= ======================
