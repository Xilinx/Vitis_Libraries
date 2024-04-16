.. 
   Copyright (C) 2019-2023, Advanced Micro Devices, Inc.
  
`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. Project documentation master file, created by
   sphinx-quickstart on Thu Jun 20 14:04:09 2020.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

==========
Benchmark 
==========
    
.. _datasets:

Datasets
-----------

The dataset used in the benchmark can be downloaded from https://sparse.tamu.edu. 


Performance
-----------

For representing the resource utilization in each benchmark, we separate the overall utilization into two parts, where P stands for the resource usage in
platform, that is, those instantiated in static region of the FPGA card, as well as K represent those used in kernels (dynamic region). The target device is set to AMD Alveo |trade| U280.

.. table:: Table 1 Performance on FPGA
    :align: center

    +-----------------+-------------+--------------+----------+---------------------+------------+------------+------------+
    | Architecture    |    Dataset  |  Latency(ms) |  Timing  |   LUT(P/K)          |  BRAM(P/K) |  URAM(P/K) |  DSP(P/K)  |
    +=================+=============+==============+==========+=====================+============+============+============+
    | SPMV (U280)     |  nasa2910   |  0.0512565   |  256MHz  |  165.475K/220.98K   |   323/211  |    64/64   |    4/900   |
    +-----------------+-------------+--------------+----------+---------------------+------------+------------+------------+

These are the details for benchmark results and usage steps.

.. toctree::
   :maxdepth: 1

   SPMV (Double precision) <benchmark/spmv_double.rst>



Test Overview
--------------

Here are the benchmarks of the AMD Vitis |trade| Sparse Library using the Vitis environment. 

.. _l2_vitis_sparse:

Vitis Sparse Library
~~~~~~~~~~~~~~~~~~~

* **Download code**

These sparse benchmarks can be downloaded from `vitis libraries <https://github.com/Xilinx/Vitis_Libraries.git>`_ ``main`` branch.

.. code-block:: bash

   git clone https://github.com/Xilinx/Vitis_Libraries.git 
   cd Vitis_Libraries
   git checkout main
   cd sparse

* **Setup environment**

Specifying the corresponding Vitis, XRT, and path to the platform repository by running following commands.

.. code-block:: bash

   source <intstall_path>/Vitis/2022.2/settings64.sh
   source /opt/xilinx/xrt/setup.sh
   export PLATFORM_REPO_PATHS=/opt/xilinx/platforms

Python3 environment: Follow the steps as per https://docs.xilinx.com/r/en-US/Vitis_Libraries/blas/user_guide/L1/pyenvguide.html to set up the Python3 environment.

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim: