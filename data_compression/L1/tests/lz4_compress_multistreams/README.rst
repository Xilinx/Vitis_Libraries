.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

AMD LZ4 Multistream Compress HLS Test
========================================

**Description:** This is a L1 test design to validate the LZ4 compression module. It processes the data to and from the double-data rate (DDR) into multiple parallel streams that helps in processing 8x data and achieve higher performance.

**Top Function:** hls_lz4CompressMutipleStreams

Results
-------

======================== ========= ========= ===== ===== 
Module                   LUT       FF        BRAM  URAM 
lz4_compress_test        8.9K      14.4K     42    6 
======================== ========= ========= ===== ===== 