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

.. _example_l3:

=====================
L3 API example
=====================

**1. XFBLAS L3 compilation**

All examples provided here could be built with the following compilation steps:

.. code-block:: bash

  g++ -O0 -std=c++11 -fPIC -Wextra -Wall -Wno-ignored-attributes -Wno-unused-parameter -Wno-unused-variable -I$(XILINX_XRT)/include -I/include/sw -o example.exe example.cpp -L$(XILINX_XRT)/lib -lz -lstdc++ -lrt -pthread -lxrt_core -ldl -luuid

**2. XFBLAS L3 run**

Most of the examples could be run in machine with HW device installed with the following run steps. Detailed usages are also provided in each section.

.. code-block:: bash

  ./example.exe PATH_TO_XCLBIN/example.xclbin PATH_TO_XCLBIN/config_info.dat


**3. XFBLAS L3 test**

Please see `XFBLAS L3 test folder`_ for more test cases.

.. _XFBLAS L3 test folder: https://gitenterprise.xilinx.com/FaaSApps/xf_blas/tree/master/L3/tests

.. toctree::
   :maxdepth: 2
   
   L3_example_gemm.rst
