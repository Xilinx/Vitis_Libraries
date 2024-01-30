.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

===================================
AMD GZip-Streaming Decompression
===================================

The GZip example resides in the ``L2/tests/gzip_p2p_decompress`` directory. 

Follow the build instructions to generate the host executable and binary.

The binary host file generated is named "**xil_gzip**", and it is present in the ``./build`` directory.

Results
-------

Resource Utilization 
~~~~~~~~~~~~~~~~~~~~

The following table presents the resource utilization of an AMD Zlib Decompress Streaming kernel. 

========== ===== ====== ==== ===== ===== ======
Flow       LUT   LUTMem REG  BRAM  URAM  Fmax
========== ===== ====== ==== ===== ===== ======
DeCompress 12.3K  226   8.4K   3    2    188MHz
========== ===== ====== ==== ===== ===== ======

Performance Data
~~~~~~~~~~~~~~~~

The following table presents the best kernel throughput achieved for a single compute unit (Single Engine). 

============================= =========================
Topic                         Results
============================= =========================
Best Kernel Throughput        442.48 Mb/s
============================= =========================

.. note:: The overall throughput can still be increased with multiple compute units.


Executable Usage:

1. To execute a single file for decompression: ``./build_dir.<TARGET>/ -dx ./build_dir.<TARGET>/xclbin_<xsa_name>_<TARGET mode>/decompress_stream.xclbin -d <compressed file_name>``

The usage of the generated executable is as follows:

.. code-block:: bash
 
   Usage: application.exe -[-h-d]
        --help,                 -h      Print Help Options   Default: [false]
        --decompress,           -d      Decompress
