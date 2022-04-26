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
    

Performance Summary for APIs
-----------

.. table:: Table 1 Summary table for performance and resources of APIs
    :align: center

+------------------------------+-------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
|  API                         | Type  | Input Description      | FPS   | MB/s   | MP/s  | Freq.  | LUT    | BRAM| URAM| DSP   |
+==============================+=======+========================+=======+========+=======+========+========+=====+=====+=======+
|  kernelJpegDecoderTop        | HW    | lena_c_512.jpg         |  1148 | 87.0   |       | 243MHz |  23.1k |  28 |   0 |   39  |
+------------------------------+-------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
|  jpegHuffmanDecoder          | cosim | lena_c_512.jpg         |  2288 |    174 |       | 270MHz |   7.9K |   5 |   0 |    2  |
+------------------------------+-------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+


These are details for benchmark result and usage steps.

.. toctree::
   :maxdepth: 1

   benchmark/jpegHuffmanDecoderIP.rst
   benchmark/jpegDecoder.rst
   

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

   source <intstall_path>/installs/lin64/Vitis/2022.1/settings64.sh
   source /opt/xilinx/xrt/setup.sh
   export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
