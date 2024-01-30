.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _user_guide_test_l1:

*******************************
L1 Test
*******************************

All L1 primitives' implementations have been tested against implementations in python. 
That is, a python based testing environment has been developed to generate random test inputs 
for each primitive, compute the golden reference, and finally compare the golden reference 
with the csim and cosim outputs of the primitive to verify the correctness of the implementation.
To run the testing process of L1 primitives, follow the steps below.

1. Set up Python environment
=============================
Follow the instructions described in :doc:`Python environment setup guide <../../pyenvguide>` 
to install anaconda3 and setup xf_hpc environment.
All testing should be run under xf_hpc environment.
Deactivate xf_hpc environment after testing.

2. Set up Vitis_hls environment
=================================
Navigate to directory L1/tests, and change the setting of environment variable 
**TA_PATH** to point to the installation path of your AMD Vitis |trade|, 
and run the following command to set up Vivado_hls environment.

.. code-block:: bash

   export XILINX_VITIS=${TA_PATH}/Vitis/2022.2
   source ${XILINX_VITIS}/settings64.sh

3. Test L1 primitives
==============================
To launch the testing process, navigate to the directory **L1/tests/hw/**.
There are three functions under testing in this directory. For each function,
there are several test cases with various configurations under **./tests/** directory. 
For each test case, use the following commands to check the Makefile usage.

.. code-block:: bash

    make help

Makefile usage example:

.. code-block:: bash

    make run CSIM=1 CSYNTH=1 COSIM=1 PLATFORM=<FPGA platform> PLATFORM_REPO_PATHS=<path to platform directories>

Command to run the selected tasks for specified device. Valid tasks are 'CSIM', 'CSYNTH', 'COSIM', 'VIVADO_SYN', 'VIVADO_IMPL'. 

'PLATFORM_REPO_PATHS' variable is used to specify the paths in which the platform files are searched for.

'PLATFORM' is case-insensitive and support awk regex. For example:

.. code-block:: bash

    make run PLATFORM='u280.*xdma' COSIM=1

It can also be an absolute path to a platform file. 

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
