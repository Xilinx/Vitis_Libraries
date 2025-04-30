.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _design_flows:

Design Flows
------------

The common tool and library prerequisites that apply across all design flows are documented in the requirements section above.

The recommended design flows are decribed as follows:

Set up the build environment using the AMD Vitis™ and XRT scripts, and set the ``PLATFORM_REPO_PATHS`` to the installation folder of the platform files.

.. code-block:: bash

    source <install path>/HEAD/Vitis/settings64.sh
    source /opt/xilinx/xrt/setup.sh
    export PLATFORM_REPO_PATHS=/opt/xilinx/platforms

The recommend flow to evaluate and test L1 components is described as follows using the AMD Vivado™ HLS tool. A top level C/C++ testbench (typically ``algorithm_name.cpp``) prepares the input data, passes them to the design under test, then performs any output data post processing and validation checks.

A ``Makefile`` is used to drive this flow with available steps including:

* ``CSIM`` (high level simulation),
* ``CSYNTH`` (high level synthesis to register transfer level (RTL)),
* ``COSIM`` (cosimulation between the software testbench and generated RTL),
* ``VIVADO_SYN`` (synthesis by Vivado), and
* ``VIVADO_IMPL`` (implementation by Vivado).

The flow is launched from the shell by calling ``make`` with variables set as in the following example:

.. code-block:: bash

   . /opt/xilinx/xrt/setup.sh
   export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
   cd L1/tests/specific_algorithm/
   # Only run C++ simulation on U250 card
   make run CSIM=1 CSYNTH=0 COSIM=0 VIVADO_SYN=0 VIVADO_IMPL=0 PLATFORM=xilinx_u250_gen3x16_xdma_4_1_202210_1


As well as verifying functional correctness, the reports generated from this flow give an indication of logic utilization, timing performance, latency, and throughput. The output files of interest can be located at the location of the test project where the path name is correlated with the algorithm. i.e., the callable function within the design under test.

To run the Vitis projects for benchmark evaluation and test, you might need the following example:

.. code-block:: bash

    ./opt/xilinx/xrt/setup.sh
    export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
    cd L1/benchmarks/specific_algorithm/
    # Run hardware emulation for cycle-accurate simulation with the RTL-model
    make run TARGET=hw_emu DEIVCE=xilinx_u250_gen3x16_xdma_4_1_202210_1
    # Run hardware to generate the desired xclbin binary
    make run TARGET=hw DEIVCE=xilinx_u250_gen3x16_xdma_4_1_202210_1
    # Delete xclbin and host excutable program
    make cleanall
