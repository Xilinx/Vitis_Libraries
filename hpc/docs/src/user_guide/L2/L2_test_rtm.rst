.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _rtm_test_l2:

*******************************
RTM Kernel Test
*******************************

L2 RTM kernels have been tested against the implementation in python. 
That is, a python based testing environment has been developed to generate random test inputs 
for each RTM kernel, compute the golden reference, and finally compare the golden reference.
To run the testing process of L2 kernels, follow the steps below.

Set up Python environment
=============================
Follow the instructions described in :doc:`Python environment setup guide <../../pyenvguide>` 
to install anaconda3 and setup xf_hpc environment.
All testing should be run under xf_hpc environment.
Deactivate xf_hpc environment after testing.

Set up Vitis environment
=================================
Navigate to the directory L2/tests, and change the setting of environment variable 
**TA_PATH** to point to the installation path of your AMD Vitis |trade| device, and run following command to set up Vivado_hls environment.

.. code-block:: bash

   export XILINX_VITIS=${TA_PATH}/Vitis/2022.2
   export XILINX_VIVADO=${TA_PATH}/Vivado/2022.2
   source ${XILINX_VITIS}/settings64.sh

Test RTM kernels
==============================
There are several pre-build L2 kernels and they can be tested individually. 
To launch the testing process, navigate to each testcase directory under **L2/tests/hw**, 
and enter the following command for hardware emulation, or
running on hardware. 

.. code-block:: bash

  make run TARGET=hw_emu/hw


Test 2D RTM
=======================

Forward kernel
--------------------------------

.. code-block:: bash

  make run TARGET=hw_emu

The above command tests and verifies forward kernel via Vitis hardware-emulation.
Once the emulation is passed, you can use the following command to build FPGA bitstream 
and launch the kernel on AMD Alveo |trade| U280 FPGA. 

.. code-block:: bash

  make build TARGET=hw
  make run TARGET=hw

The parameters listed in the following table can be configured with **make** command.
Notice that **RTM_time** must be multiple of **RTM_numFSMs**.

.. table:: Parameters with make command 
    :align: center

    +----------------+----------------+------------------------------------+
    |  Parameter     |  Default Value |  Notes                             |
    +================+================+====================================+
    |  RTM_maxDim    |   1282         |  Compile time: One dimmention limit|
    +----------------+----------------+------------------------------------+
    |  RTM_NXB       |   40           |  Compile time: Boundary width      |
    +----------------+----------------+------------------------------------+
    |  RTM_NZB       |   40           |  Compile time: Boundary height     |
    +----------------+----------------+------------------------------------+
    |  NUM_numFSMs   |   2            |  Compile time: No.stream module    |
    +----------------+----------------+------------------------------------+
    |  RTM_nPE       |   4            |  Compile time: No.PE               |
    +----------------+----------------+------------------------------------+
    |  RTM_order     |   8            |  Compile time: Spatial Order       |
    +----------------+----------------+------------------------------------+
    |  RTM_height    |   10           |  Running time: Image total height  |
    +----------------+----------------+------------------------------------+
    |  RTM_width     |   10           |  Running time: Image total widht   |
    +----------------+----------------+------------------------------------+
    |  RTM_time      |   10           |  Running time: No.time             |
    +----------------+----------------+------------------------------------+

Backward kernel
--------------------------------

.. code-block:: bash

  make run TARGET=hw_emu

The above command tests and verifies backward kernel via Vitis hardware-emulation.
Once the emulation is passed, you can use the following command to build FPGA bitstream 
and launch the kernel on Alveo U280 FPGA. 

.. code-block:: bash

  make run TARGET=hw

The parameters listed in the following table can be configured with **make** command.
Notice that **RTM_time** must be multiple of **RTM_numBSMs**.

.. table:: Parameters with make command 
    :align: center

    +----------------+----------------+------------------------------------+
    |  Parameter     |  Default Value |  Notes                             |
    +================+================+====================================+
    |  RTM_maxDim    |   1282         |  Compile time: One dimmention limit|
    +----------------+----------------+------------------------------------+
    |  RTM_NXB       |   40           |  Compile time: Boundary width      |
    +----------------+----------------+------------------------------------+
    |  RTM_NZB       |   40           |  Compile time: Boundary height     |
    +----------------+----------------+------------------------------------+
    |  NUM_numFSMs   |   2            |  Compile time: No.stream module    |
    +----------------+----------------+------------------------------------+
    |  RTM_nPE       |   4            |  Compile time: No.PE               |
    +----------------+----------------+------------------------------------+
    |  RTM_order     |   8            |  Compile time: Spatial Order       |
    +----------------+----------------+------------------------------------+
    |  RTM_height    |   10           |  Running time: Image total height  |
    +----------------+----------------+------------------------------------+
    |  RTM_width     |   10           |  Running time: Image total widht   |
    +----------------+----------------+------------------------------------+
    |  RTM_time      |   10           |  Running time: No.time             |
    +----------------+----------------+------------------------------------+


Test 3D RTM
===============

Forward kernel with HBC/RBC boundary condition
----------------------------------------------

.. code-block:: bash

  make run TARGET=hw_emu

The above command tests and verifies forward kernel with HBC/RBC boundary condition via Vitis hardware-emulation.
Once the emulation is passed, you can use the following command to build FPGA bitstream 
and launch the kernel on Alveo U280 FPGA. 

.. code-block:: bash

  make build TARGET=hw
  make run TARGET=hw

The parameters listed in the following table can be configured with **make** command.
Notice that **RTM_time** must be multiple of **RTM_numFSMs**.
**RTM_z** must be less than **RTM_maxZZ** and be multiple of **RTM_nPEZ**.
**RTM_x** must be multiple of **RTM_nPEX**.


.. table:: Parameters with make command 
    :align: center

    +----------------+----------------+------------------------------------+
    |  Parameter     |  Default Value |  Notes                             |
    +================+================+====================================+
    |  RTM_maxY      |   280          |  Compile time: y-dimmention limit  |
    +----------------+----------------+------------------------------------+
    |  RTM_maxZ      |   180          |  Compile time: z-dimmention limit  |
    +----------------+----------------+------------------------------------+
    |  RTM_NXB       |   20           |  Compile time: Boundary width      |
    +----------------+----------------+------------------------------------+
    |  RTM_NYB       |   20           |  Compile time: Boundary width      |
    +----------------+----------------+------------------------------------+
    |  RTM_NZB       |   20           |  Compile time: Boundary height     |
    +----------------+----------------+------------------------------------+
    |  NUM_numFSMs   |   2            |  Compile time: No.stream module    |
    +----------------+----------------+------------------------------------+
    |  RTM_nPEX      |   4            |  Compile time: No.PE along X       |
    +----------------+----------------+------------------------------------+
    |  RTM_nPEZ      |   4            |  Compile time: No.PE along Z       |
    +----------------+----------------+------------------------------------+
    |  RTM_order     |   8            |  Compile time: Spatial Order       |
    +----------------+----------------+------------------------------------+
    |  RTM_x         |   10           |  Running time: Image x dim size    |
    +----------------+----------------+------------------------------------+
    |  RTM_y         |   10           |  Running time: Image y dim size    |
    +----------------+----------------+------------------------------------+
    |  RTM_z         |   10           |  Running time: Image z dim size    |
    +----------------+----------------+------------------------------------+
    |  RTM_time      |   10           |  Running time: No.time             |
    +----------------+----------------+------------------------------------+

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
