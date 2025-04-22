.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _cg_test_l2:

*******************************
CG Kernel Test
*******************************

L2 CG kernels have been tested against the implementation in python. 
That is, a python based testing environment has been developed to generate random test inputs 
for each CG kernel, compute the golden reference, and finally compare the golden reference.
To run the testing process of L2 kernels, follow the steps below.

Set up Python environment
=============================
Follow the instructions described in :doc:`Python environment setup guide <../../pyenvguide>` 
to install anaconda3 and setup xf_hpc environment.
All testing should be run under xf_hpc environment.
Deactivate xf_hpc environment after testing.

Set up Vitis environment
=================================
Navigate to directory L2/tests, and change the setting of environment variable 
**TA_PATH** to point to the installation path of your AMD Vitis |trade| device, and run the following command to set up Vivado_hls environment.

.. code-block:: bash

   export XILINX_VITIS=${TA_PATH}/Vitis/2022.2
   export XILINX_VIVADO=${TA_PATH}/Vivado/2022.2
   source ${XILINX_VITIS}/settings64.sh

Test CG kernels
==============================
There are several pre-build L2 kernels and they can be tested individually. 
To launch the testing process, navigate to each testcase directory under **L2/tests/cgSolver/**, 
and enter the following command for hardware emulation, or
running on hardware. 

.. code-block:: bash

  make run TARGET=hw_emu/hw


GEMV-based CG solver
=======================

.. code-block:: bash

  make run TARGET=hw_emu

The above command tests and verifies forward kernel via Vitis hardware-emulation.
Once the emulations are passed, you can use the following command to build FPGA bitstream 
and launch the kernel on AMD Alveo |trade| U280 FPGA or Alveo U50 FPGA. 

.. code-block:: bash

  make build TARGET=hw
  make run TARGET=hw

The parameters listed in the following table can be configured with **make** command.

+----------------+----------------+---------------------------------------+
|  Parameter     |  Default Value |  Notes                                |
+================+================+=======================================+
|  CG_numChannels|   1/8/16       |  No. parallel HBM channels for matrix |
+----------------+----------------+---------------------------------------+

SPMV-based CG solver
=======================
.. code-block:: bash

  make run TARGET=hw_emu

The above command tests and verifies forward kernel via Vitis hardware-emulation.
Once the emulations are passed, you can use the following command to build FPGA bitstream 
and launch the kernel on Alveo U280 FPGA or Alveo U50 FPGA. 

.. code-block:: bash

  make build TARGET=hw
  make run TARGET=hw


.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
