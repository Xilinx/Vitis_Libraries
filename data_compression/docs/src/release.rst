.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _release_note:

Release Note
============

.. toctree::
   :hidden
   :maxdepth: 1

2024.2
------

The Lz4 decompression APIs have been enhanced for improving performance and for eliminating potential issue for extreme corner cases. The new-added API lz4DecompressEngine_NinMout has configurable output-port width and can provide up-to 48% throughput improvement.


2022.2
------

All Compression and Decompression applications are migrated to the AMD Vitis™ 2022.2 version.

2022.1
------

Following is the 2022.1 release notes.

* **ZLIB Compression Improvement**
    - Reduced TreeGen Initial Interval < 1K to reduce overall resource utilization for 8 KB octa core compression.
    - Customized Octa-Core compression for the 8 KB solution ( Reduced Booster Window 8 KB).
    - Static IP customized.
    - Improved Compression IP Timing for AMD Versal™ adaptive SoCs and achieved > 250 MHz.
    - Provided Memory Mapped GZIP file decompression.

* **ZLIB Decompression Improvement**
    - Customized IP for a 8 KB file size.
    - Added ADL32 and provided uncompressed size in TUSER.
    - Provided Quad-Core Decompress solution for the 32 KB and 8 KB file size to achieve 4x throughput (upto 2 Gb/s).

2021.2
------

Following is the 2021.2 release notes.

* **ZSTD Quad-Core Compression**
   Created ZSTD Multi-Core architecture to provide high throughput for single file compression. Using the Zstd Quad core solution, you can get a throughput of > 1 Gb/s. 

* **Zstd Decompress Improvement**
   ZSTD Decompress optimized in this release. The overall resource is reduced to 19.6K and achieves 20% higher throughput compared to the previous release.

* **GZIP Decompress Improvement**
   - Rearchitected GZIP Decompress cores to reduce resource to 6.9K and better throughput compared to the previous release. With this new latency, the overall IP latency is also reduced to a ~1.5K cycle. Provided a - ZLIB decompression containing ADLR32 Checksum to catch any error in the input file. 
   - Added functionality to provide uncompressed size in the output stream port TUSER (in case the end application needs to know the uncompressed size).

* **GZIP Compression Improvement**
   - Created various ZLIB/GZIP Octa-Core Compression Kernels for different block sizes (8 KB, 16 KB, 32 KB) and achieved > 2 Gb/s throughput for all variants. Updated the IP core to provide the compressed size in the output axis stream TUSER port (in case any application needs the compressed size). 
   - Huffman TreeGen latency is reduced significantly < 1K, as a result, for multi-core architectures (Octa-core), a single Treegen is required. This reduces the resource requirement signficantly down for 8 KB and 16 KB blocksize compression cores compared to the previous release solution. 
   - Compression ratio is improved from 2.67 to 2.7 for Silesia Fileset for 32 KB blocksize. 

* **Snappy/LZ4 Decompress Improvement**
   - Optimized Snappy and LZ4 Decompress throughput. 
   

2021.1
------

The following is the 2021.1 release notes.

* **GZIP Multi Core Compression**
   - New GZIP Multi-Core Compress Streaming Accelerator which is a purely stream only solution (free running kernel); it comes with many variants of different block size support of 4 KB, 8 KB, 16 KB, and 32 KB. 

* **Facebook ZSTD Compression Core**
   - New Facebook ZSTD Single Core Compression accelerator with block size 32 KB.
   - Multi-cores ZSTD compression is in progress (for higher throughput).

* **GZIP Low Latency Decompression**
   - A new version of GZIP decompress with improved latency for each block, lesser resources (35% lower LUT, 83% lower block RAM), and improved FMax.

* **ZLIB Whole Application Acceleration using U50**
   L3 GZIP solution for the Alveo U50 Platform, containing six Compression cores to saturate the full PCIe bandwidth. It is provided with an Efficient GZIP SW Solution to accelerate the CPU libz.so library which provides seamless inflate and deflate API level integration to end the customer software without recompiling. 

* **Versal Platform Support**


2020.2
------

The following is the 2020.2 release notes.

* **LIBZ Library Acceleration using U50** 

  - Achieved seamless acceleration of libz standard APIs (deflate, compress2, and uncompress).
  - Ready to use libz.so library to accelerate any host code without any code change. 
  - Provided an xzlib standalone executable for for both gzip/zlib compress and decompress.

* **New ZSTD Decompression**
   Facebook ZSTD decompression implemented.

* **New Snappy Dual Core Kernel**
   Google snappy Dual Core Decompression to get 2x the throughput for a single file decompress.
* **New GZIP Compress Kernel**
   Implemented a new GZIP Quad Core Compress Kernel (in build, LZ77, TreeGen, and Huffman encoder). Reduced the overall resource >20% and reduced 50% DDR bandwidth requirement. 
* **New GZIP Compress Streaming Kernel**
   Implemented fully standard compliance GZIP (include header & footer) streaming free running kernels.
* **GZIP/ZLIB L3 Application on U50**
   Provided GZIP/ZLIB Application in L3, optimized for the Alveo U50 (HBM) and U250. A single xclbin supports both zlib & gzip format for compress and uncompress.
* **Porting Library to U50**
   Most of Library functions (LZ4, Snappy, GZIP, ZLIB) ported to the Alveo U50 platform.
* **Low Latency GZIP/ZLIB Decompress**
   Reduced Decompression initial latency from 5K to 2.5K (latency improvement for for small block size 4 KB/8 KB/16 KB).
