====================
Zlib (SO) Demo
====================

Infrastructure
--------------

List below presents infrastructure required to build & deploy this demo.
Mandatory requirements are marked accordingly in order to get this demo working in
deployment environment. Vitis is required only for development.

    ``Vitis: 2022.1``
    
    ``SHELL: u50_gen3x16_xdma``

Setup
-----

``source scripts/setup.csh``

Build
-----

Emulation:

``make run TARGET=<sw_emu/hw_emu> PLATFORM=< absolute path to u50 xpfm >``

Hardware:

``make all TARGET=hw PLATFORM=< absolute path to u50 xpfm >``
    
Application Usage
-----------------

**Compression**     -->  ``./xzlib input_file``

**Decompression**   -->  ``./xzlib -d input_file.zlib``

**Test Flow**       -->  ``./xzlib -t input_file`` 

**No Acceleration** -->  ``./xzlib -t input_file -n 0`` 

**Help**           -->  ``./xzlib -h``

**Regression**     --> Refer ``run.sh`` script to understand usage of various options provided with ``xzlib`` utility. 


Note: By default host executable is built for both compress/decompress on FPGA.
