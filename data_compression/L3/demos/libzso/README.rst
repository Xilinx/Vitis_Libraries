.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

====================
Zlib (SO) Demo
====================

Infrastructure
--------------

The following list presents the infrastructure required to build and deploy this demo. Mandatory requirements are marked accordingly to get this demo working in a
deployment environment. AMD Vitis™ is required only for development.

    ``Vitis: 2022.1``
    
    ``SHELL: u50_gen3x16_xdma``

Setup
-----

``source scripts/setup.csh``

Build
-----

Emulation:

``make run TARGET=hw_emu PLATFORM=< absolute path to u50 xpfm >``

Hardware:

``make all TARGET=hw PLATFORM=< absolute path to u50 xpfm >``
    
Application Usage
-----------------

**Compression**     -->  ``./xzlib input_file``

**Decompression**   -->  ``./xzlib -d input_file.zlib``

**Test Flow**       -->  ``./xzlib -t input_file`` 

**No Acceleration** -->  ``./xzlib -t input_file -n 0`` 

**Help**           -->  ``./xzlib -h``

**Regression**     --> Refer the ``run.sh`` script to understand the usage of the various options provided with the ``xzlib`` utility. 

.. note:: By default, the host executable is built for both compress/decompress on a FPGA.
