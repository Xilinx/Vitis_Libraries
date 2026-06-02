.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. toctree::
   :hidden:

.. _design_flows:

============
Design Flows
============

The common tool and library prerequisites for all design flows are listed in the requirements section.

Shell Environment
=================

Set up the build environment using the AMD Vitis™ and XRT scripts.

.. code-block:: shell

    source /opt/xilinx/Vitis/2022.2/settings64.sh
    source /opt/xilinx/xrt/setup.sh
    export PLATFORM_REPO_PATHS=/opt/xilinx/platforms

For ``csh`` users, use the corresponding scripts with the ``.csh`` suffix and adjust the variable assignment command.

Set ``PLATFORM_REPO_PATHS`` to the installation folder of the platform files so that the makefiles in this library can use the ``PLATFORM`` variable as a pattern. Otherwise, provide the full path to the ``.xpfm`` file using the ``PLATFORM`` variable.

HLS Cases Command Line Flow
===========================

.. code-block:: shell

    cd L1/tests/hls_case_folder

    make run TARGET=csim \
        PLATFORM=/path/to/xilinx_u280_xdma_201920_3.xpfm

The following test control variables are available:

* ``CSIM`` for high-level simulation.
* ``CSYNTH`` for high-level synthesis to RTL.
* ``COSIM`` for co-simulation between the software test bench and generated RTL.
* ``VIVADO_SYN`` for synthesis by AMD Vivado™.
* ``VIVADO_IMPL`` for implementation by Vivado.

For all these variables, set to ``1`` to enable execution or ``0`` to skip. The default value of all control variables is ``0``; omit any variable from the command line to skip the corresponding step.
