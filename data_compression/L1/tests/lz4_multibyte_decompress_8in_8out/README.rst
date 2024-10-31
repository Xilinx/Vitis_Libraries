.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

AMD LZ4 Multibyte Decompress HLS Test
========================================

**Description:** Test design to validate an other variant implementation of LZ4 Multi Byte Decompress Module. The average throughput has been improved 11% on Silesia test with less area.

**Top Function:** lz4DecompressEngineRun

Results of resource usage
-------

======================== ========= ========= ===== ===== 
Module                   LUT       FF        BRAM  URAM 
lz4_decompress_test      5.1K      3.6K      0     2 
======================== ========= ========= ===== ===== 

Results of performance improvement
-------
======= ================= ======================
Silesia	compression ratio Throughput improvement
dickens	1.58			27%
mozilla	1.94			13%
mr	1.83			17%
nci	6.06			-4%
ooffice	1.42			14%
osdb	1.92			5%
reymont	2.08			32%
samba	2.80			7%
sao	1.07			-2%
webster	2.06			18%
xml	4.35			2%
x-ray	1.01			-1%
average	2.34			11%
======= ================= ======================
