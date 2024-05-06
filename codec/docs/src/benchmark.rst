.. 
Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
SPDX-License-Identifier: X11


.. Project documentation master file, created by
   sphinx-quickstart on Thu Jun 20 14:04:09 2020.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

==========
Benchmark 
==========
    

Performance Summary for APIs
-----------

.. table:: Table 1 Summary table for performance and resources of APIs
    :align: center

+------------------------------+-------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
|  API                         | Type  | Input Description      | FPS   | MB/s   | MP/s  | Freq.  | LUT    | BRAM| URAM| DSP   |
+==============================+=======+========================+=======+========+=======+========+========+=====+=====+=======+
|  pikEncKernel1Top            | HW    | lena_c_512.jpg         |  62.5 |        |  16.4 | 200MHz |  97.4k |  25 |  93 |  568  |
+------------------------------+-------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
|  pikEncKernel2Top            | HW    | lena_c_512.jpg         |  62.5 |        |  16.4 | 200MHz | 262.5k | 411 | 252 | 1614  |
+------------------------------+-------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
|  pikEncKernel3Top            | HW    | lena_c_512.jpg         |  62.5 |        |  16.4 | 200MHz |  90.0k | 178 | 128 |  216  |
+------------------------------+-------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
|  pikEncKernel1Top            | HW    | lena_c_2048.png        |   5.2 |        |    22 | 200MHz |  97.4k |  25 |  93 |  568  |
+------------------------------+-------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
|  pikEncKernel2Top            | HW    | lena_c_2048.png        |   5.2 |        |    22 | 200MHz | 262.5k | 411 | 252 | 1614  |
+------------------------------+-------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
|  pikEncKernel3Top            | HW    | lena_c_2048.png        |   5.2 |        |    22 | 200MHz |  90.0k | 178 | 128 |  216  |
+------------------------------+-------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
|  kernelJpegDecoderTop        | HW    | lena_c_512.jpg         |  1148 | 87.0   |       | 243MHz |  23.1k |  28 |   0 |   39  |
+------------------------------+-------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
|  resizeTop(NP=8)             | HW    | 7680*4320 to 512*512   |  79.7 | 2644.3 |       | 341MHz |  15.0k |  29 |   0 |  168  |
+------------------------------+-------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
|  resizeTop(NP=8)             | HW    | 7680*4320 to 1920*1080 |  80.5 | 2670.8 |       | 341MHz |  15.0k |  29 |   0 |  168  |
+------------------------------+-------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
|  webp_IntraPredLoop2_NoOut_1 | HW    | lena_c_512.png         |       | 127.17 |       | 250MHz |  52.9k |  72 |  10 |  410  |
+------------------------------+-------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
|  webp_2_ArithmeticCoding_1   | HW    | lena_c_512.png         |       | 127.17 |       | 250MHz |  15.9k | 157 |   0 |    4  |
+------------------------------+-------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
|  webp_IntraPredLoop2_NoOut_1 | HW    | 1920x1080.png          |       | 172.54 |       | 250MHz |  52.9k |  72 |  10 |  410  |
+------------------------------+-------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
|  webp_2_ArithmeticCoding_1   | HW    | 1920x1080.png          |       | 172.54 |       | 250MHz |  15.9k | 157 |   0 |    4  |
+------------------------------+-------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
|  JxlEnc_ans_clusterHistogram | HW    | lena_c_512.png         |       |        |  56.9 | 291MHz |  38.5K |  70 |  28 |   51  |
+------------------------------+-------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
|  JxlEnc_lossy_enc_compute    | HW    | lena_c_512.png         |       |        |  72.2 | 260MHz | 121.7K | 364 |  53 |  498  |
+------------------------------+-------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
|  JxlEnc_ans_initHistogram    | HW    | lena_c_512.png         |       |        |  43.2 | 289MHz |  39.3K |  50 |  41 |   95  |
+------------------------------+-------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
|  JxlEnc_ans_clusterHistogram | HW    | hq_2Kx2K.png           |       |        | 101.9 | 291MHz |  38.5K |  70 |  28 |   51  |
+------------------------------+-------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
|  JxlEnc_lossy_enc_compute    | HW    | hq_2Kx2K.png           |       |        |  83.3 | 260MHz | 121.7K | 364 |  53 |  498  |
+------------------------------+-------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
|  JxlEnc_ans_initHistogram    | HW    | hq_2Kx2K.png           |       |        |  52.9 | 289MHz |  39.3K |  50 |  41 |   95  |
+------------------------------+-------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
|  jpegHuffmanDecoder          | cosim | lena_c_512.jpg         |  2288 |    174 |       | 270MHz |   7.9K |   5 |   0 |    2  |
+------------------------------+-------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+


These are details for benchmark result and usage steps.

.. toctree::
   :maxdepth: 1

   JPEG Huffman Decoder <benchmark/jpegHuffmanDecoderIP.rst>
   JPEG Decoder <benchmark/jpegDecoder.rst>
   PIK Encoder <benchmark/pikEnc.rst>
   Resize <benchmark/resize.rst>
   Webp Encoder <benchmark/webpEnc.rst>
   JXL Encoder <benchmark/jxlEnc.rst>

Test Overview
--------------

Here are benchmarks of the Vitis Codec Library using the Vitis environment and comparing with cpu(). 


.. _l2_vitis_codec:


* **Download code**

These graph benchmarks can be downloaded from `vitis libraries <https://github.com/Xilinx/Vitis_Libraries.git>`_ ``main`` branch.

.. code-block:: bash

   git clone https://github.com/Xilinx/Vitis_Libraries.git 
   cd Vitis_Libraries
   git checkout main
   cd codec 

* **Setup environment**

Specifying the corresponding Vitis, XRT, and path to the platform repository by running following commands.

.. code-block:: bash

   source <intstall_path>/installs/lin64/Vitis/2022.1/settings64.sh
   source /opt/xilinx/xrt/setup.sh
   export PLATFORM_REPO_PATHS=/opt/xilinx/platforms

.. Copyright © 2020–2023 Advanced Micro Devices, Inc
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.