.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: BLAS, Library, Vitis BLAS Library, primitives, L1 test
   :description: Vitis BLAS library L1 primitive implementations have been tested in vitis tools.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _user_guide_test_l1:

*******************************
L1 Test
*******************************

.. code-block:: bash

    source <install path>/Vitis/2023.2/settings64.sh
    source /opt/xilinx/xrt/setup.sh
    cd L1/tests/case_folder/
    make run TARGET=<cosim/csim/csynth/vivado_syn/vivado_impl> PLATFORM=/path/to/xilinx_u250_gen3x16_xdma_3_1_202020_1.xpfm

- `csim` (high level simulation),
- `csynth` (high level synthesis to register transfer level (RTL)),
- `cosim` (cosimulation between software testbench and generated RTL),
- `vivado_syn` (synthesis by AMD Vivado™), and
- `vivado_impl` (implementation by Vivado).

