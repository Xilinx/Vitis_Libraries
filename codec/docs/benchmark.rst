.. 
   Copyright 2019 Xilinx, Inc.
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.


.. Project documentation master file, created by
   sphinx-quickstart on Thu Jun 20 14:04:09 2020.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

==========
Benchmark 
==========
    
.. _pictures:

Pictures & Performance
-----------

1. JPEG Decoder: This API supports the ‘Sequential DCT-based mode’ of ISO/IEC 10918-1 standard. It is a high-performance implementation based-on Xilinx HLS design methodolygy.
   It can process 1 Huffman token and create up to 8 DCT coeffiects within one cycle. 
   It is also an easy-to-use decoder as it can direct parser the JPEG file header without help of software functions.
2. Pik Encoder: This API is based on Google’s PIK, which was ‘chosen as the base framework for JPEG XL’. 
   The pikEnc is based on the ‘fast mode’ of PIK which can provide better encoding efficnty than most of other still image encoding methods.
   The pikEnc is based on Xilinx HLS design methodology and optimized for FPGA arthitecture. 
   It can proved higher throughput and lower latency compared to software-based solutions.
our commonly used pictures are listed in table below. 

.. table:: Table 1 Cosim benchmark for Huffman Decoder(L1)
    :align: center

    +--------------------+----------+-------------+----------------+-----------------+------------------+-----+----------+
    |   Pictures         |  Format  |    Size     | Compress ratio | cosim Freq(MHz) | input speed(MB/s)| QPS | time(ms) |
    +====================+==========+=============+================+=================+==================+=====+==========+
    |  lena_c_512.jpg    |    420   |   512*512   |  5.2           | 300             |  174             | 2288| 0.437    |
    +--------------------+----------+-------------+----------------+-----------------+------------------+-----+----------+
    |  t0.jpg            |    420   |   616*516   |  9.3           | 300             |  147             | 2890| 0.346    |
    +--------------------+----------+-------------+----------------+-----------------+------------------+-----+----------+
    |  android.jpg       |    420   |  960*1280   |  14.2          | 300             |  145             | 1125| 0.889    |
    +--------------------+----------+-------------+----------------+-----------------+------------------+-----+----------+
    |  offset.jpg        |    422   |  5184*3456  |  4.6           | 300             |  209             | 27  | 37.25    |
    +--------------------+----------+-------------+----------------+-----------------+------------------+-----+----------+
    |  hq.jpg            |    444   |  5760*3840  |  2.8           | 300             |  233             | 10  | 101.1    |
    +--------------------+----------+-------------+----------------+-----------------+------------------+-----+----------+
    |  iphone.jpg        |    420   |  3264*2448  |  5.4           | 300             |  213             | 96  | 10.47    |
    +--------------------+----------+-------------+----------------+-----------------+------------------+-----+----------+


.. table:: Table 2 On board benchmark for JPEG Decoder(L2)
    :align: center

    +--------------------+----------+-------------+----------------+-----------+------------------+-----+----------+
    |   Pictures         |  Format  |    Size     | Compress ratio | Freq(MHz) | input speed(MB/s)| QPS | time(ms) |
    +====================+==========+=============+================+===========+==================+=====+==========+
    |  lena_c_512.jpg    |    420   |   512*512   |  5.2           | 243       |  87              | 1148| 0.871    |
    +--------------------+----------+-------------+----------------+-----------+------------------+-----+----------+
    |  t0.jpg            |    420   |   616*516   |  9.3           | 243       |  66              | 1292| 0.774    |
    +--------------------+----------+-------------+----------------+-----------+------------------+-----+----------+
.. table:: Table 3 On board benchmark for Pik Eecoder(L2)
    :align: center

    +--------------------+------------------+-------------+-------------+-------------+------------+---------------------+------+
    |   Pictures         |      Size        | Kernel1(ms) | Kernel2(ms) | Kernel3(ms) | Freq(MHz)  | input speed(MPIX/s) | QPS  |
    +====================+==================+=============+=============+=============+============+=====================+======+
    |  lena_c_512.png    |     512*512      |    16       |      14     |      7      |     200    |      16.4           | 62.5 |
    +--------------------+------------------+-------------+-------------+-------------+------------+---------------------+------+
    |  lena_c_1024.png   |    1024*1024     |    52       |      48     |     24      |     200    |      20.2           | 19.2 |
    +--------------------+------------------+-------------+-------------+-------------+------------+---------------------+------+
    |  lena_c_2048.png   |    2048*2048     |    191      |      180    |     86      |     200    |      22.0           | 5.2  |
    +--------------------+------------------+-------------+-------------+-------------+------------+---------------------+------+

Resource Utilization
-----------

For representing the resource utilization in each benchmark, we separate the overall utilization into 2 parts, where P stands for the resource usage in
platform, that is those instantiated in static region of the FPGA card, as well as K represents those used in kernels (dynamic region). The input is
png, jpg, pik, e.g. format, and the target device is set to Alveo U200.

.. table:: Table 4 Resource Utilization
    :align: center

    +------------------------+----------+----------------+-------------+------------+------------+
    |    Architecture        |  Freq    |    LUT(P/K)    |  BRAM(P/K)  |  URAM(P/K) |  DSP(P/K)  |
    +========================+==========+================+=============+============+============+
    | JPEG Huffman Decoder   |  270MHz  |  108.1K/7.9K   |   178/5     |    0/0     |     4/12   |
    +------------------------+----------+----------------+-------------+------------+------------+
    |  JPEG Decoder          |  243MHz  |  108.1K/23.1K  |   178/28    |    0/0     |     4/39   |
    +------------------------+----------+----------------+-------------+------------+------------+
    |  PIK Encoder           |  300MHz  |  150.9K/439.4K |   338/62    |    0/16    |     7/0    |
    +------------------------+----------+----------------+-------------+------------+------------+

These are details for benchmark result and usage steps.

.. toctree::
   :maxdepth: 1

   benchmark/jpegHuffmanDecoderIP.rst
   benchmark/jpegDecoder.rst
   benchmark/pikEnc.rst
   

Test Overview
--------------

Here are benchmarks of the Vitis Codec Library using the Vitis environment and comparing with cpu(). 


.. _l2_vitis_codec:

Vitis Codec Library
~~~~~~~~~~~~~~~~~~~

* **Download code**

These graph benchmarks can be downloaded from `vitis libraries <https://github.com/Xilinx/Vitis_Libraries.git>`_ ``master`` branch.

.. code-block:: bash

   git clone https://github.com/Xilinx/Vitis_Libraries.git 
   cd Vitis_Libraries
   git checkout master
   cd codec 

* **Setup environment**

Specifying the corresponding Vitis, XRT, and path to the platform repository by running following commands.

.. code-block:: bash

   source <intstall_path>/installs/lin64/Vitis/2021.2/settings64.sh
   source /opt/xilinx/xrt/setup.sh
   export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
