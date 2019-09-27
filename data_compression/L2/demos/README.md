## Demos on LZ Based Data Compression
Xilinx LZ data compression architecture is targeted for FPGAs.
It aims to provide high throughput. This architecture is developed and tested on Xilinx
Alveo board. Even though this architecture is designed for LZ4 application, but it
is generic enough to support various other LZ based data compression algorithms.

## Architecture 

Xilinx FPGA based LZ data-compression architecture contains multiple
compression engines which run concurrently to get higher throughput.
Each compression engine is designed to process 1byte/clock(@300MHz). So if design
contains N(8) compression engines, overall throughput will be Nx300MB/s
(8x300=2.4GB/s).  
This is a generic architecture to cover all LZ based algorithms (LZ77, LZ4, LZO
and Snappy). 

![LZx compress select](../../common/img/lzx_comp.png) <br />

Diagram above presents overview of LZ kernel architecture for compression and
similar architecture applies to decompression module as well. Input data is
divided into multiple blocks (64K default size) and send each block to individual
compression engine to compress concurrently. 
Each compression engine contains series of modules, which are transferring data to
next module using HLS stream. Each sub-module is designed to process 1byte/clock
so overall each engine will maintain a constant throughput of 1byte/clock.
Compression engine architecture is generic for all LZ based algorithms (LZ77, LZ4,
LZO, and snappy), only encoding module will be different based on targeting
algorithm.
Architecture contains multiple instances of compression engines which are
connected to mm2s(input unit) and s2mm(output unit). Input unit reads
uncompressed blocks from Global memory (512bit wide) and distributes across
various parallel compression engines. Output unit reads compressed block from
compression engine and write to global memory (512 bit wide). 

![LZx decompress select](../../common/img/lzx_decomp.png) <br />

Diagram above presents overview of LZ kernel architecture for decompression.
Each decompression engine contains series of modules, which transfer
data to next module using HLS stream. Similar to LZ compression mm2s(input
unit) and s2mm(output unit) are connected to decompression engines.

## Implemented Algorithms

Based on the presented architecture above we have implemented following compression algorithm.

* [LZ4 Compress/Decompress](./lz4)
* [Snappy Compress/Decompress](./snappy)
