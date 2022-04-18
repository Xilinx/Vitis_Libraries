## Demo for Ethash with SystemCompiler

### Introduction
This demo is trying to give a show case for how to implement the Ethash algorithm with SystemCompiler. However, it is important to be noticed that this demo is not the final status of the Ethash design, it aims to display a feasible way for implementing a dataflow loop with the new feature called free running kernel in SystemCompiler. Some other latest added supports from SystemCompiler is also needed, like:

  1. Configurable stream depth in vpp::stream.
  2. Binding for different platform with SYS_PORT_PFM.
  3. SLR assignments with ASSIGN_SLR.
  4. Loop back dataflow with FREE_RUNNING.
 
### input file
A dag-file is provided at this folder for initializing the nodes allocated in the HBMs.

### How to run
Firstly, source the env.sh to set Vitis and XRT environment.
1. re-build the xclbin and host binary

```console
# build and run one of the following using U55N platform
#  * software emulation
#  * hardware emulation
#  * actual deployment on physical platform

$ make run TARGET=sw_emu DEVICE=xilinx_u55n_gen3x4_xdma_2_202110_1
$ make run TARGET=hw_emu DEVICE=xilinx_u55n_gen3x4_xdma_2_202110_1
$ make run TARGET=hw DEVICE=xilinx_u55n_gen3x4_xdma_2_202110_1
```
2. use the existing xclbin and binary for board verification

```console
# run binary on the server with U55N cards.
$ export VPP_SC_NPI=1
$ ./host.exe -dagfile ./dagfile1G.dat -n 1
```
