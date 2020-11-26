Vitis Data Compression Library
==============================

Vitis Data Compression library is an open-sourced Vitis library written
in C++ for accelerating data compression applications in a variety of
use cases. The library covers two levels of acceleration: the module level
and the pre-defined kernel level, and will evolve to offer the third
level as pure software APIs working with pre-defined hardware overlays.

-  L1: Module level, it provides optimized hardware implementation of
   the core LZ based and data compression specific modules like lz4
   compress and snappy compress.
-  L2: Kernel level, a demo on lz4, snappy and zlib data compression
   algorithms are shown via kernel which internally uses the optimized
   hardware modules.
-  L3: The software API level will wrap the details of offloading
   acceleration with prebuilt binary (overlay) and allow users to
   accelerate data compression tasks on Alveo cards without hardware
   development.

Advanced users can easily tailor, optimize or
combine with property logic at any levels as all the kernel code is developed in HLS C++ with the permissive
Apache 2.0 license. Demos of different data
compression acceleration are also provided with the library for easy
on-boarding.

Requirements
------------

Software Platform
~~~~~~~~~~~~~~~~~

This library is designed to work with Vitis 2020.2 and later, and
therefore inherits the system requirements of Vitis and XRT.

Supported operating systems are RHEL/CentOS 7.4, 7.5 and Ubuntu 16.04.4
LTS, 18.04.1 LTS. With CentOS/RHEL 7.4 and 7.5, C++11/C++14 should be
enabled via
`devtoolset-6 <https://www.softwarecollections.org/en/scls/rhscl/devtoolset-6/>`__.

PCIE Accelerator Card
~~~~~~~~~~~~~~~~~~~~~

Hardware modules and kernels are designed to work with 16nm Alveo cards.
\* `Alveo
U280 <https://www.xilinx.com/products/boards-and-kits/alveo/u280.html#gettingStarted>`__
\* `Alveo
U250 <https://www.xilinx.com/products/boards-and-kits/alveo/u250.html#gettingStarted>`__
\* `Alveo
U200 <https://www.xilinx.com/products/boards-and-kits/alveo/u200.html#gettingStarted>`__

Shell Environment
~~~~~~~~~~~~~~~~~

Setup the build environment using the Vitis and XRT scripts:

::

       $ source <install path>/Vitis/2020.2/settings64.sh
       $ source /opt/xilinx/xrt/setup.sh
       $ export PLATFORM_REPO_PATHS=/opt/xilinx/platforms

Setting the PLATFORM_REPO_PATHS to installation folder of platform files
can enable makefiles in this library to use DEVICE variable as a
pattern. Otherwise, full path to .xpfm file needs to be provided via
DEVICE variable.

Benchmark Result
----------------

By offloading compression to FPGA, we achieved 3.6x speedup against
single threaded LZ4 default (v1.9.0) and a 4.4x speedup against single
core Snappy (v1.1.4). Benchmark evaluation of compression performance is
of reference Silesia Corpus.

LICENSE
-------

Licensed using the `Apache 2.0
license. <https://www.apache.org/licenses/LICENSE-2.0>`__

::

   Copyright 2020 Xilinx, Inc.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
   Copyright 2020 Xilinx, Inc.

Contribution/Feedback
---------------------

Welcome! Guidelines to be published soon.
