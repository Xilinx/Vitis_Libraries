.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: BLAS, Library, Vitis BLAS Library, Vitis BLAS, level 3
   :description: Vitis BLAS library level 3 provides Python bindings that users could use Vitis BLAS libraries in Python.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _python_bindings_l3:

=====================
L3 Python Bindings
=====================

The AMD Vitis™ BLAS level 3 provides Python bindings that you can use Vitis BLAS libraries in Python. 

1. Introduction
================================

1.1 Set Python Environment
--------------------------------
Refer to :doc:`Python environment setup guide<pyenvguide>`.

1.2 Build the Shared Library 
-------------------------

L3 Python bindings use ctypes to wrap the L3 API functions in pure Python. To call these Python functions, you need to build the ``xfblas.so`` using the Makefile in ``L3/src/sw/python_api`` locally.

2. Using the Vitis BLAS L3 Python API
======================================

2.1 General Description
------------------------

This section describes how to use the Vitis BLAS library API level Python bindings. To use the library, you need to source PYTHONPATH to the directory of ``xfblas_L3.py``, and import ``xfblas_L3`` as xfblas at the beginning of the Python file.

2.1.1 Vitis BLAS Initialization
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To initialize the library, call the following two functions. 

.. code-block:: python

  import xfblas_L3 as xfblas
  args, xclbin_opts = xfblas.processCommandLine()
  xfblas.createGemm(args,xclbin_opts,1,0)

2.2 Vitis BLAS Helper Function Reference
-----------------------------------------

.. autoclass:: xfblas_L3.XFBLASManager
    :members:

2.3 Using Python APIs
-----------------------------------------

Refer to ``L3/src/sw/python_api/test_gemm.py`` for using Python APIs to test gemm. To run that case in hw, use the following steps:

- Build the shared library.
- Set PYTHONPATH.
- Find the path to the xclbin, and run the command:

.. code-block:: bash
  
  source /opt/xilinx/xrt/setup.sh
  cd L3/src/sw/python_api/
  make api
  export PYTHONPATH=./:../../../../L1/tests/sw/python/
  python test_gemm.py  --xclbin PATH_TO_GEMM_XCLBIN/blas.xclbin --cfg PATH_TO_GEMM_XCLBIN/config_info.dat --lib ./lib/xfblas.so
