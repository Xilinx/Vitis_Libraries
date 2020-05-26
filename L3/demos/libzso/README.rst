====================
Zlib (SO) Demo
====================

This demo presents usage and generation of libz.so (FPGA port).
This demo support Alveo U200, Alveo U280, Alveo U50 (2020.1 Vitis)

Build Instructions
-------------------

.. highlight:: bash

    make run TARGET=sw_emu --> Software Emulation 
    make run TARGET=hw_emu --> Hardware Emulation

    NOTE: This command builds/runs for sample data set. 

Application Usage
------------------

Following instructions helps in testing and validation.

Copy ``libz.so`` to current directory
from ``L3/demos/libzso_app/build/`` directory. In case of make run
this step is automated.


Environment Setup:
~~~~~~~~~~~~~~~~~

.. highlight:: bash

``export LD_LIBRARY_PATH=$(PWD):$(LD_LIBRARY_PATH)``

``export LD_LIBRARY_PATH=$(PWD):$(LD_LIBRARY_PATH)`` 

``export XILINX_LIBZ_XCLBIN=<path to xclbin>``

``source /opt/Xilinx/Vitis/2020.1/settings64.sh``

``source /opt/Xilinx/xbb/xrt/packages/setup.sh``


*It is must to set the LD_LIBRARY_PATH to environment variable
to the directory where libz.so inorder to use FPGA flow.

  
Compression:
~~~~~~~~~~~~

``./build/zlib_so.exe -c <input_file>``

Decompression:
~~~~~~~~~~~~~~

``./build/zlib_so.exe -d <input_file.zlib>``

Validate (Both flows):
~~~~~~~~~~~~~~~~~~~~~

This command does compress/decompress and
compares results.


``./build/zlib_so.exe -v <input_file>``


Help Section:
~~~~~~~~~~~~~

``./build/zlib_so.exe -h``
