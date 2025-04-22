.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _fcn_test_l2:

*******************************
FCN Kernel Test
*******************************

L2 FCN kernels have been tested against the implementation in C++ on host side.

Test FCN kernels
==============================
There are two pre-build L2 FCN kernels, one with 1 FCN CU (Compute Unit) and the other one with 4 FCN CUs. 
Both of the two kernels can be tested individually. 
To launch the testing process, navigate to each testcase directory under **L2/tests/hw/mlp/**. 

.. code-block:: bash

  make run TARGET=hw_emu

The above command tests and verifies the FCN kernel via AMD Vitis |trade| hardware-emulation.
Once the emulation is passed, you can use the following command to build FPGA bitstream 
and launch the kernel on AMD Alveo |trade| U250/U50 FPGA card. 

.. code-block:: bash

  make build TARGET=hw
  make run TARGET=hw

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
