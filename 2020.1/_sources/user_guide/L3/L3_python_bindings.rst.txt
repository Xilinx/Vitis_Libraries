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

.. meta::
   :keywords: BLAS, Library, Vitis BLAS Library, Vitis BLAS, level 3
   :description: Vitis BLAS library level 3 provides Python bindings that users could use Vitis BLAS libraries in Python.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _python_bindings_l3:

=====================
L3 Python bindings
=====================
Vitis BLAS level 3 provides Python bindings that users could use Vitis BLAS libraries in Python. Examples of using Vitis BLAS Python bindings in keras to do classification are also available to try. Applications for using Python API could be found in :doc:`L3 API application <L3_application>`.

1. Introduction
================================

1.1 Set Python Environment
--------------------------------
Please refer to :doc:`Python environment setup guide<../L1/pyenvguide>`.

1.2 Build shared library 
-------------------------
L3 Python bindings use ctypes to wrap the L3 API functions in pure Python. In order to call these Python functions, users need to build the xfblas.so by Makefile in L3/src/sw/python_api locally.

2. Using the Vitis BLAS L3 Python API
==================================

2.1 General description
------------------------
This section describes how to use the Vitis BLAS library API level Python bindings. To use the library, users need to source PYTHONPATH to the directory of xfblas_L3.py and import xfblas_L3 as xfblas at the beginning of the Python file.

2.1.1 Vitis BLAS initialization
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To initialize the library, call the following two functions. 

.. code-block:: python

  import xfblas_L3 as xfblas
  args, xclbin_opts = xfblas.processCommandLine()
  xfblas.createGemm(args,xclbin_opts,1,0)

2.2 Vitis BLAS Helper Function Reference
-------------------------------------

.. autoclass:: xfblas_L3.XFBLASManager
    :members:
