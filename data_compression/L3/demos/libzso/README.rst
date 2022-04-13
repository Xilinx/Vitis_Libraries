====================
Zlib (SO) Demo
====================

Infrastructure
--------------

List below presents infrastructure required to build & deploy this demo.
Mandatory requirements are marked accordingly in order to get this demo working in
deployment environment. Vitis is required only for development.

    ``Vitis: 2022.1_released (Only for Developers)``
    
    ``XRT: 2022.1_PU1 (Mandatory)``
    
    ``SHELL: u50_gen3x16_xdma_201920_3 (Mandatory)``
    
    
Application Usage
-----------------

**Compression**     -->  ``./xzlib input_file``

**Decompression**   -->  ``./xzlib -d input_file.zlib``

**Test Flow**       -->  ``./xzlib -t input_file`` 

**No Acceleration** -->  ``./xzlib -t input_file -n 0`` 

**Help**           -->  ``./xzlib -h``

**Regression**     --> Refer ``run.sh`` script to understand usage of various options provided with ``xzlib`` utility. 


Note: By default host executable is built for both compress/decompress on FPGA.
