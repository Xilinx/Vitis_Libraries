.. _release_note:

Release Note
============

.. toctree::
   :hidden
   :maxdepth: 1
2022.2
------

All Compression and Decompression applications are migrated to Vitis 2022.2 version.

2022.1
------

Following is the 2022.1 release notes.

* **ZLIB Compression Improvement**
    - Reduced TreeGen Initial Interval < 1K to reduce overall resource utilization for 8KB octa core compression.
    - Customized Octa-Core compression for 8KB solution ( Reduced Booster Window 8KB)
    - Static IP customized.
    - Improved Compression IP Timing for Versal and achieved > 250MHz.
    - Provided Memory Mapped GZIP File Decompression.

* **ZLIB Decompression Improvement**
    - Customized IP for 8KB file size.
    - Added ADL32 and provided uncompressed size in TUSER.
    - Provided Quad-Core Decompress solution for 32KB and 8KB file size to achieve 4x throughput (upto 2GB/s).

2021.2
------

Following is the 2021.2 release notes.

* **ZSTD Quad-Core Compression**
   Created ZSTD Multi-Core architecture to provide high throughput for single file compression. 
   Using Zstd Quad core solution, user can get throughput > 1 GB/s. 

* **Zstd Decompress Improvement**
   ZSTD Decompress optimized in this release. Overall resource is reduced to 19.6K 
   and achieve 20% higher throughput compare to previous release.

* **GZIP Decompress Improvement**
   Re-architected GZIP Decompress cores to reduce resource to 6.9K 
   and better throughput compare to previous release. 
   With this new latency overall IP latency is also reduced to ~1.5K cycle. 
   Provided ZLIB decompression containing ADLR32 Checksum to catch any error in input file. 
   Added functionality to provide uncompressed size in output stream port TUSER (incase end application needs to know uncompressed size).

* **GZIP Compression Improvement**
   - Created various ZLIB/GZIP Octa-Core Compression Kernels for different block sizes (8KB, 16KB, 32KB) 
     and achieved > 2GB/s throughput for all variants. Updated IP core to provided compressed size in output axis
     stream TUSER port (incase any application needs compressed size). 
   - Huffman TreeGen latency is reduced significantly < 1K, as a result, for multi-core architectures (Octa-core), 
     a single Treegen is required. This reduce the resource requirement signficantly down for 8KB and 16KB blocksize 
     compression core compare to previous release solution. 
   - Compression ratio is improved from 2.67 to 2.7 for Silesia Fileset for 32KB bloksize. 

* **Snappy/LZ4 Decompress Improvement**
   Optimized Snappy and LZ4 Decompress throughput. 
   

2021.1
------

Following is the 2021.1 release notes.

* **GZIP Multi Core Compression**
   New GZIP Multi-Core Compress Streaming Accelerator which is purely stream only
   solution (free running kernel), it comes with many variant of different block
   size support of 4KB, 8KB, 16KB and 32KB. 

* **Facebook ZSTD Compression Core**
   New Facebook ZSTD Single Core Compression accelerator with block size 32KB.
   Multi-cores ZSTD compression is in progress (for higher throughput).

* **GZIP Low Latency Decompression**
   A new version of GZIP decompress with improved latency for each block, lesser
   resources (35% lower LUT, 83% lower BRAM) and improved FMax.

* **ZLIB Whole Application Acceleration using U50**
   L3 GZIP solution for U50 Platform, containing 6 Compression core to saturate
   full PCIe bandwidth. It is provided with Efficient GZIP SW Solution to
   accelerate CPU libz.so library which provide seamless Inflate and deflate API
   level integration to end customer software without recompiling. 

* **Versal Platform Supports**


2020.2
------

Following is the 2020.2 release notes.

* **LIBZ Library Acceleration using U50** 

  - Achieved seamless acceleration of libz standard APIs (deflate, compress2 and
    uncompress)
  - Ready to use libz.so library to accelerate any host code without any code change 
  - Provided xzlib standalone executable for for both gzip/zlib compress &
    decompress

* **New ZSTD Decompression**
   Facebook ZSTD decompression implemented

* **New Snappy Dual Core Kernel**
   Google snappy Dual Core Decompression to get 2x throughput for single file
   decompress.
* **New GZIP Compress Kernel**
   Implemented new GZIP Quad Core Compress Kernel (in build , LZ77 , TreeGen,
   Huffman encoder). Reduced overall resource >20%, and reduce 50% DDR bandwidth
   requirement. 
* **New GZIP Compress Streaming Kernel**
   Implemented fully standard compliance GZIP(include header & footer) streaming
   free running kernels.
* **GZIP/ZLIB L3 Application on U50**
   Provided GZIP/ZLIB Application in L3 , optimized for U50 (HBM) and U250. Single
   xclbin supports both zlib & gzip format for compress and uncompress
* **Porting Library to U50**
   Most of Library functions (LZ4, Snappy, GZIP, ZLIB) ported to U50 platform.
* **Low Latency GZIP/ZLIB Decompress**
   Reduced Decompression initial latency from 5K to 2.5K (latency improvement for
   for small block size 4KB/8KB/16KB) 
