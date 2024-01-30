.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

Vitis Data Compression Library
==============================

The AMD Vitis™ Data Compression library is an open-sourced Vitis library written in C++ for accelerating data compression applications in a variety of use cases. The library covers two levels of acceleration: the module level and the predefined kernel level and will evolve to offer the third level as pure software APIs working with predefined hardware overlays.

*  L1: Module level; it provides an optimized hardware implementation of the core LZ based and data compression specific modules like lz4 compress and snappy compress.
*  L2: Kernel level; a demo on lz4, snappy, and zlib data compression algorithms are shown via the kernel which internally uses the optimized hardware modules.
*  L3: The software API level will wrap the details of offloading binary (overlay) and allow users to accelerate data compression tasks on Alveo cards without hardware development.

Advanced users can easily tailor, optimize, or combine with property logic at any level as all the kernel code is developed in HLS C++ with the permissive Apache 2.0 license. Demos of different data compression acceleration are also provided with the library for easy onboarding.

Requirements
------------

Software Platform
~~~~~~~~~~~~~~~~~

This library is designed to work with Vitis 2022.2 and later and therefore, inherits the system requirements of Vitis and XRT.

Supported operating systems are RHEL/CentOS 7.4, 7.5 and Ubuntu® 16.04.4 LTS, 18.04.1 LTS. With CentOS/RHEL 7.4 and 7.5, C++11/C++14 should be enabled via `devtoolset-6 <https://www.softwarecollections.org/en/scls/rhscl/devtoolset-6/>`__.

FPGA Accelerator Card
~~~~~~~~~~~~~~~~~~~~~

Hardware modules and kernels are designed to work with 16nm AMD Alveo™ cards.

* `Alveo U250 <https://www.xilinx.com/products/boards-and-kits/alveo/u250.html>`__
* `Alveo U200 <https://www.xilinx.com/products/boards-and-kits/alveo/u200.html>`__
* `Alveo U50 <https://www.xilinx.com/products/boards-and-kits/alveo/u50.html>`__
* `Versal AI VCK190 <https://www.xilinx.com/products/boards-and-kits/vck190.html>`__


Shell Environment
~~~~~~~~~~~~~~~~~

Set up the build environment using the Vitis and XRT scripts:

::

       $ source <install path>/Vitis/2021.2/settings64.sh
       $ source /opt/xilinx/xrt/setup.sh
       $ export PLATFORM_REPO_PATHS=/opt/xilinx/platforms

Setting the PLATFORM_REPO_PATHS to the installation folder of the platform files can enable makefiles in this library to use the PLATFORM variable as a pattern. Otherwise, the full path to the .xpfm file needs to be provided via the PLATFORM variable.

Source Files and Application Development
----------------------------------------

Vitis libraries are organized into L1, L2, and L3 folders with each relating to a different stage of application development.

**L1**: Makefiles and sources in L1 facilitate the HLS-based flow for quick checks. Tasks at this level include:

* Check the functionality of an individual kernel (C-simulation)
* Estimate the resource usage, latency, etc. (Synthesis)
* Run cycle accurate simulations (Co-simulation)
* Package as IP, and get final resource utilization/timing details (Export RTL)
       
	.. note:: Once RTL (or XO file after packaging IP) is generated, the AMD Vivado™ flow is invoked for the XCLBIN file generation if required.

**L2**: Makefiles and sources in L2 facilitate building the XCLBIN file from various sources (HDL, HLS, or XO files) of kernels with host code written in the OpenCL/XRT framework targeting a device. This flow supports:

* Software emulation to check the functionality
* Hardware emulation to check the register transfer level (RTL) level simulation
* Build and test on hardware

**L3**: Makefiles and sources in L3 demonstrate applications developed involving multiple kernels in a pipeline. These Makefiles can be used for executing tasks, as with the L2 Makefiles.

Benchmark Result
----------------

By offloading compression to FPGA, 19.3x speedup is achieved using a single GZIP compress kernel against single core CPU Zlib fast (1.2.11, -1) and a 2x speedup achieved using a single GZIP decompress kernel against a single core CPU Zlib fast (1.2.11, -1).

Dataset
~~~~~~~

The benchmark evaluation of compression performance is of reference `Silesia Corpus. <http://sun.aei.polsl.pl/~sdeor/index.php?page=silesia>`__.

Compression
~~~~~~~~~~~

The following table showcases the throughput details of compression for various Alveo accelerated data compression algorithms.

+------------------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+
| Architecture                                                           |  Compression Ratio   |     Throughput    |  FMax    |  LUT    |  BRAM |  URAM |
+========================================================================+======================+===================+==========+=========+=======+=======+
| `LZ4 Streaming <L2/tests/lz4_compress_streaming>`_                     |        2.13          |      290 Mb/s    |  300 MHz  |  3K     |  5    |  6    |
+------------------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+
| `Snappy Streaming <L2/demos/snappy_streaming>`_                        |        2.13          |      290 Mb/s     |  300 MHz  |  3K     |  4    |  6   |
+------------------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+
| `GZip/Zlib 32KB Memory Mapped <L2/tests/gzipc_block_mm>`_              |        2.70          |      2 Gb/s       |  290 MHz  |  53K    |  140  |  64  |
+------------------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+
| `GZip 32KB Compress Stream <L2/tests/gzipc>`_                          |        2.70          |      2 Gb/s       |  30 MHz  |  57K    |  142  |  64   |
+------------------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+
| `GZip 16KB Compress Stream <L2/tests/gzipc_16KB>`_                     |        2.62          |      2 Gb/s       |  292 MHz  |  62K    |  175  |  48  |
+------------------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+
| `GZip 8KB Compress Stream <L2/tests/gzipc_8KB>`_                       |        2.50          |      2 Gb/s       |  300 MHz  |  61K    |  111  |  48  |
+------------------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+
| `GZip Fixed 32KB Compress Stream <L2/tests/gzipc_static>`_             |        2.31          |      2 Gb/s       |  300 MHz  |  39K    |  53   |  64  |
+------------------------------------------------------------------------+----------------------+-------------------+----- -----+---------+-------+-------+
| `Zlib 32KB Compress Stream <L2/tests/zlibc>`_                          |        2.70          |      2 Gb/s       |  300MHz  |  57K    |  131  |  64   |
+------------------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+
| `Zlib 16KB Compress Stream <L2/tests/zlibc_16KB>`_                     |        2.62          |      2 Gb/s       |  300 MHz  |  62K    |  165  |  48  |
+------------------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+
| `Zlib 8KB Compress Stream <L2/tests/zlibc_8KB>`_                       |        2.50          |      2 Gb/s       |  300 MHz  |  61K    |  101  |  48  |
+------------------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+
| `Zlib Fixed 32KB Compress Stream <L2/tests/zlibc_static>`_             |        2.31          |      2 Gb/s       |  300 MHz  |  39K    |  43   |  64  |
+------------------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+
| `Zstd Compress Quad Core <L2/tests/zstd_quadcore_compress>`_           |        2.68          |     1.17 Gb/s     |  275 MHz  |  44K    |  94   |  37  |
+------------------------------------------------------------------------+----------------------+-------------------+----------+---------+-------+-------+

* GZip/Zlib Memory Mapped and GZip/Zlib Compress Stream: Supports Dynamic Huffman.

Decompression
~~~~~~~~~~~~~

The following table showcases the throughput details of decompression for various Alveo accelerated data compression algorithms.

+----------------------------------------------------------------------+-------------------+----------+---------+-------+------+
| Architecture                                                         |    Throughput     |  FMax    |  LUT    |  BRAM | URAM |           
+======================================================================+===================+==========+=========+=======+======+
| `LZ4 Streaming <L2/tests/lz4_dec_streaming_parallelByte8>`_          |     1.8  Gb/s     |  292 MHz  |  11K    |  15   |  2   |
+----------------------------------------------------------------------+-------------------+----------+---------+-------+------+
| `Snappy Streaming <L2/tests/snappy_dec_streaming_parallelByte8>`_    |     1.97 Gb/s     |  300 MHz  |  12K    |  15   |  2   |
+----------------------------------------------------------------------+-------------------+----------+---------+-------+------+
| `GZip/Zlib Streaming <L2/demos/gzip>`_                               |     518  Mb/s     |  283 MHz  |  6.7K   |  8    |  0   |
+----------------------------------------------------------------------+-------------------+----------+---------+-------+------+
| `ZStd Streaming <L2/tests/zstdd_32KB>`_                              |   658.86 Mb/s     |  240 MHz  |  23K    |  34   |  3   |
+----------------------------------------------------------------------+-------------------+----------+---------+-------+------+

* GZip/Zlib Streaming: Full standard support (Dynamic Huffman, Fixed Huffman, and Stored Blocks supported).
* ZStd Streaming: Full Standard support with limited Window size up to 128 KB.

Contribution/Feedback
---------------------

Welcome! Guidelines to be published soon.