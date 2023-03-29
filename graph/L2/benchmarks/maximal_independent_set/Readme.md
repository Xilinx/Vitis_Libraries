# Maximal Independent Set 

Maximal Independent Set example resides in ``L2/benchmarks/maximal_independent_set`` directory. The tutorial provides a step-by-step guide that covers commands for building and running kernel.

## Executable Usage

* **Work Directory(Step 1)**

The steps for library download and environment setup can be found in [here](https://github.com/Xilinx/Vitis_Libraries/tree/master/graph/L2/benchmarks#building). For getting the design,

```
   cd L2/benchmarks/maximal_independent_set
```

* **Build kernel(Step 2)**

Run the following make command to build your XCLBIN and host binary targeting a specific device. Please be noticed that this process will take a long time, maybe couple of hours.

```
   make run TARGET=hw PLATFORM=xilinx_u50_gen3x16_xdma_5_202210_1
```

* **Run kernel(Step 3)**

To get the benchmark results, please run the following command.

```
   ./build_dir.hw.xilinx_u50_gen3x16_xdma_5_202210_1/host.exe -xclbin build_dir.hw.xilinx_u50_gen3x16_xdma_5_202210_1/mis_kernel.xclbin -o data/data-csr-offset.mtx -i data/data-csr-indicesweights.mtx  -mis data/mis.txt
```

Renumber Input Arguments:

```
   Usage: host.exe -[-xclbin -o -i -mis]
         -xclbin:        the kernel name
         -o              offset file of input graph in CSR format
         -i              edge file of input graph in CSR format
         -mis            golden reference file for validatation
```

Note: Default arguments are set in Makefile, you can use other [datasets](https://github.com/Xilinx/Vitis_Libraries/tree/master/graph/L2/benchmarks#datasets) listed in the table.

## Profiling

The hardware resource utilizations are listed in the following table.
Different tool versions may result slightly different resource.

Table 1 Hardware resources

   |    Kernel         |   BRAM   |   URAM   |    DSP    |   LUT   | Frequency(MHz)  |
   |-------------------|----------|----------|-----------|---------|-----------------|
   |   mis_kernel      |    786   |   0      |     12    |  13595  |      211.9      |

## Benchmark

The performance is shown in the table below.

Table 2 Comparison between CPU and FPGA  

   | Datasets         | Vertex   | Edges    | CPU time  | FPGA time  | Speedup  |
   |------------------|----------|----------|-----------|------------|----------|
   | coPapersCiteseer | 434102   | 16036720 |  0.26     |   0.309    |  84.14%  |
   | coPapersDBLP     | 540486   | 15245729 |  0.31     |   0.317    |  97.79%  |
   | hollywood        | 1139905  | 57515616 |  1.78     |   1.21     |  147.11% |
   | as-Skitter       | 1694616  | 11094209 |  0.74     |   0.344    |  215.12% |
   | cit-Patents      | 3774768  | 16518948 |  2.07     |   0.584    |  354.45% |


##### Note
```
   1. Maximal independent set CPU time benchmarking is running on Intel(R) Xeon(R) CPU E5-2667 v3 @ 3.20GHz, cache(2048 KB), cores(31)
   2. time unit: ms.
   3. This mis implementation focus on single-kernel-level design and focusing on mid-scale dataset processing. As showed in table, with the increasing of the graph vertex number, the FPGA show increasingly advantage over CPU offloading.
   4. The performance is tested under config of "set_property -dict [list CONFIG.ECC_EN {false} CONFIG.ECC_SCRUB_EN {false}] [get_bd_cells hmss_0]"
```

