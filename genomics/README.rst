Vitis Genomics Library
==============================

Vitis Genomics library is an open-sourced Vitis library written
in C++ for accelerating Genomics applications in a variety of
use cases. The library covers two levels of acceleration: the module level
and the pre-defined kernel level, and will evolve to offer the third
level as pure software APIs working with pre-defined hardware overlays.

-  L1: Module level, it provides optimized hardware implementation of
   the core genomics based modules like Smithwaterman, SMEM, PairHMM.

-  L2: Kernel level, a demo on Smithwaterman, SMEM, PairHMM is shown 
   via kernel which internally uses the optimized hardware module.

-  L3: The software API level will wrap the details of offloading
   acceleration with prebuilt binary (overlay) and allow users to
   accelerate genomics tasks on Alveo cards without hardware
   development.

Advanced users can easily tailor, optimize or
combine with property logic at any levels as all the kernel code is developed in HLS C++ with the permissive
Apache 2.0 license. Demos of different data
genomics acceleration are also provided with the library for easy
on-boarding.

Requirements
------------

Software Platform
~~~~~~~~~~~~~~~~~

This library is designed to work with Vitis 2021.2 and later, and
therefore inherits the system requirements of Vitis and XRT.

Supported operating systems are RHEL/CentOS 7.4, 7.5 and Ubuntu 16.04.4
LTS, 18.04.1 LTS. With CentOS/RHEL 7.4 and 7.5, C++11/C++14 should be
enabled via
`devtoolset-6 <https://www.softwarecollections.org/en/scls/rhscl/devtoolset-6/>`__.

FPGA Accelerator Card
~~~~~~~~~~~~~~~~~~~~~

Hardware modules and kernels are designed to work with 16nm Alveo cards.

* `Alveo U250 <https://www.xilinx.com/products/boards-and-kits/alveo/u250.html>`__

* `Alveo U200 <https://www.xilinx.com/products/boards-and-kits/alveo/u200.html>`__


Shell Environment
~~~~~~~~~~~~~~~~~

Setup the build environment using the Vitis and XRT scripts:

::

       $ source <install path>/Vitis/2021.2/settings64.sh
       $ source /opt/xilinx/xrt/setup.sh
       $ export PLATFORM_REPO_PATHS=/opt/xilinx/platforms

Setting the PLATFORM_REPO_PATHS to installation folder of platform files
can enable makefiles in this library to use DEVICE variable as a
pattern. Otherwise, full path to .xpfm file needs to be provided via
DEVICE variable.

Benchmark Result
----------------



Dataset
~~~~~~~
Benchmark evaluation of genomics algorithm performance is with random data.



Genomics
~~~~~~~~

Tables below showcases throughput details of genomics for various Alveo accelerated genomics algorithms.

+--------------------------------------------------------------+-------------------+----------+---------+-------+-------+-------+
| Architecture                                                 |     Throughput    |  FMax    |  LUT    |  BRAM |  URAM |  DSP  |
+==============================================================+===================+==========+=========+=======+=======+=======+
| `Smithwaterman Algorithm <L2/demos/smithwaterman>`_          |      267GCUPS     |  300MHz  |  172K   |   2   |   0   |   0   |
+--------------------------------------------------------------+-------------------+----------+---------+-------+-------+-------+
| `PairHMM Algorithm <L2/tests/pairhmm>`_                      |      40GCUPS      |  237MHz  |  231K   |  235  |  12   |  1610 |
+--------------------------------------------------------------+-------------------+----------+---------+-------+-------+-------+
| `SMEM Algorithm <L2/tests/smem>`_                            |      2GB/s        |  300MHz  |  46K    |  70   |  70   |   0   |
+--------------------------------------------------------------+-------------------+----------+---------+-------+-------+-------+                                

                                                                                           

LICENSE
-------

Licensed using the `Apache 2.0
license. <https://www.apache.org/licenses/LICENSE-2.0>`__

::

   Copyright 2022 Xilinx, Inc.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
   Copyright 2022 Xilinx, Inc.

Contribution/Feedback
---------------------

Welcome! Guidelines to be published soon.
