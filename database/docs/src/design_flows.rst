.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis, Database, Vitis Database Library, Alveo
   :description: Vitis Database library licensing, software and hardware requirements.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _flows:

.. toctree::
   :hidden:

.. _design_flows:

Design Flows
------------

The common tool and library prerequisites that apply across all design flows are documented in the requirements section above.

The recommended design flows are described as follows:

Shell Environment
~~~~~~~~~~~~~~~~~

Set up the build environment using AMD Vitis™ and XRT scripts.

.. code-block:: shell

    source /opt/xilinx/Vitis/2022.2/settings64.sh
    source /opt/xilinx/xrt/setup.sh
    export PLATFORM_REPO_PATHS=/opt/xilinx/platforms


For ``csh`` users, look for corresponding scripts with the ``.csh`` suffix, and adjust the variable setting command accordingly.

Setting `PLATFORM_REPO_PATHS` to the installation folder of platform files can enable makefiles in this library to use the `PLATFORM` variable as a pattern. Otherwise, the full path to .xpfm file needs to be provided via `PLATFORM` variable.

HLS Cases Command Line Flow
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: shell

    cd L1/tests/hls_case_folder
    
    make run CSIM=1 CSYNTH=0 COSIM=0 VIVADO_SYN=0 VIVADO_IMPL=0 \
        PLATFORM=/path/to/xilinx_u280_xdma_201920_3.xpfm

Test control variables are:

* ``CSIM`` for high level simulation.
* ``CSYNTH`` for high level synthesis to register transfer level (RTL).
* ``COSIM`` for co-simulation between software test bench and generated RTL.
* ``VIVADO_SYN`` for synthesis by AMD Vivado™.
* ``VIVADO_IMPL`` for implementation by Vivado.

For all these variables, setting to ``1`` indicates execution while ``0`` for skipping. The default value of all these control variables are ``0``, so they can be omitted from command line if the corresponding step is not wanted.

Vitis Cases Command Line Flow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: shell

    cd L2/tests/vitis_case_folder
    
    # build and run one of the following using U280 platform
    make run TARGET=hw_emu PLATFORM=/path/to/xilinx_u280_xdma_201920_3.xpfm
    
    # delete generated files
    make cleanall


Here, ``TARGET`` decides the FPGA binary type.

* ``hw_emu`` is for hardware emulation.
* ``hw`` is for deployment on physical card. (Compilation to hardware binary often takes hours.)

Besides ``run``, the Vitis case makefile also allows ``host`` and ``xclbin`` as the build target.
