This folder contains Vivado-HLS-based unit tests for kernel primitives.

To test l1 primitives, please follow the steps below:
  1. navigate to xf_blas/L1/tests
  2. source vitis environment
  3. run case in vitis. For example: 
     
```console
    source <install path>/2025.1/Vitis/settings64.sh
    source /opt/xilinx/xrt/setup.sh
    cd L1/tests/case_folder
    make run TARGET=<cosim/csim/csynth/vivado_syn/vivado_impl> PLATFORM=/path/to/xilinx_u250_gen3x16_xdma_4_1_202210_1.xpfm
```
- `csim` (high level simulation),
- `csynth` (high level synthesis to RTL),
- `cosim` (cosimulation between software testbench and generated RTL),
- `vivado_syn` (synthesis by Vivado) and
- `vivado_impl` (implementation by Vivado).

