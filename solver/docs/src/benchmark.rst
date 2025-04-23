
.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis, Solver, Library, Vitis Solver Library, quality, performance
   :description: Vitis Solver Library quality and performance results.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _benchmark:

==========
Benchmark 
==========

Datasets
---------

The row number and column number of matrix are assigned as input arguments. The matrix is then generated randomly.

Performance
-----------

To represent the resource utilization in each benchmark, separate the overall utilization into two parts, where P stands for the resource usage in platform, that is those instantiated in static region of the FPGA card. K represents those used in kernels (dynamic region). The input is matrix, and the target device is set to AMD Alveo |trade| U250.

.. table:: Performance for processing solver on FPGA
    :align: center

    +----------------+---------------+----------+--------------+----------+----------------+-------------+------------+------------+
    | Architecture   |  Matrix_Size  |  Unroll  |  Latency(s)  |  Timing  |    LUT(P/K)    |  BRAM(P/K)  |  URAM(P/K) |  DSP(P/K)  |
    +================+===============+==========+==============+==========+================+=============+============+============+
    | GESVDJ (U250)  |    512x512    |    16    |    25.94     |  300MHz  |  108.1K/21.1K  |   178/127   |    0/20    |     4/2    |
    +----------------+---------------+----------+--------------+----------+----------------+-------------+------------+------------+
    | GESVJ (U250)   |    512x512    |     8    |    1.811     |  280MHz  |  101.7K/101.5K |   165/387   |    0/112   |     4/3    |
    +----------------+---------------+----------+--------------+----------+----------------+-------------+------------+------------+
    | GTSV (U250)    |    512x512    |    16    |    3.484     |  275MHz  |  101.7K/160.5K |  165/523.5  |    0/110   |     4/6    |
    +----------------+---------------+----------+--------------+----------+----------------+-------------+------------+------------+


Following are the details for benchmark result and usage steps.

.. toctree::
   :maxdepth: 1

   ../guide_L2/Benchmark/gesvdj.rst
   ../guide_L2/Benchmark/gesvj.rst
   ../guide_L2/Benchmark/gtsv.rst

Test Overview
--------------

Here are the benchmarks of the AMD Vitis |trade| Solver Library using the Vitis environment. 


.. _l2_vitis_solver:


* **Download code**

These solver benchmarks can be downloaded from `vitis libraries <https://github.com/Xilinx/Vitis_Libraries.git>`_ ``main`` branch.

.. code-block:: bash

   git clone https://github.com/Xilinx/Vitis_Libraries.git 
   cd Vitis_Libraries
   git checkout main
   cd solver 

* **Setup environment**

Specifying the corresponding Vitis, XRT, and path to the platform repository by running the following commands.

.. code-block:: bash

   source /opt/xilinx/2025.1/Vitis/settings64.sh
   source /opt/xilinx/xrt/setup.sh
   export PLATFORM_REPO_PATHS=/opt/xilinx/platforms

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
