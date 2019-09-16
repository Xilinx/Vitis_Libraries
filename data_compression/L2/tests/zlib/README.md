# Xilinx ZLIB

Xilinx ZLIB compression/decompression is FPGA based implementation of standard ZLIB. 
Xilinx implementation of ZLIB application is aimed at achieving high throughput for both compression and decompression.
This Xilinx ZLIB application is developed and tested on Xilinx Alveo U200 instance.

This application is accelerated using generic hardware architecture for LZ based data compression algorithms.

For more details refer this [link](https://gitenterprise.xilinx.com/heeran/xil_lz4/blob/master/README.md)


## Results

### Resource Utilization <br />

Table below presents resource utilization of Xilinx ZLIB compress/decompress
kernels with 8 engines for single compute unit. It is possible to extend number of engines to achieve higher throughput.


| Design | LUT | LUTMEM | REG | BRAM | URAM| DSP | Fmax (MHz) |
| --------------- | --- | ------ | --- | ---- | --- | -----| -----|
| Compression     | 99996(10.08%) | 44701(7.74%)|61033(2.90%)|146(7.79%) | 48(5%)|1(0.01%)|240|
| Decompression     | 44447(4.30%) | 22413(3.88%)|40626(1.87%)|146(7.79%)|0|1(0.01%)|250|



### Throughput & Compression Ratio

Table below presents the best throughput achieved during execution of this application. 
The throughput reported for compression is with 8 parallel engines and two compute units.
The throughput reported for decompression is with 1 parallel engine and single compute unit.

| Topic| Results| 
|-------|--------|
|Best Compression Throughput|2.1 GB/s|
|Best Decompression Throughput| 220 MB/s |
|Average Compression Ratio| 2.78x (Silesia Benchmark)|

Note: Overall throughput can still be increased with multiple compute units and multiple parallel engines.

## Software & Hardware

```
  Software: Xilinx Vitis 2019.2
  Hardware: xilinx_u200_xdma_201830_2 (Xilinx Alveo U200)
```
 
## Usage


### Build Steps

#### Emulation flows
```
  make run TARGET=<sw_emu/hw_emu> DEVICE=xilinx_u200_xdma_201830_2
  
  Note: This command compiles for targeted emulation mode and executes the
        application. To execute it on AWS F1 instance, please follow instructions
        specific to AWS F1 emulation.
```
#### Hardware

```
  make all TARGET=hw DEVICE=$AWS_PLATFORM

  Note: This command compiles for hardware execution. It generates kernel binary ".xclbin" file. 
        This file is placed in ./build/xclbin*/ directory under zlib folder. To execute it on AWS F1 instance, 
        please follow instructions specific to AWS F1 deployment process.

```

### Execution Steps

While using PARALLEL_BLOCK (8 default) the generated executable would be
"xil_lz4_8b"

```
  Input Arguments: 
    
        1. To execute single file for compression :  ./build/xil_lz4_8b -cx <compress xclbin> -c <file_name>
        2. To execute single file for decompression: ./build/xil_lz4_8b -dx <decompress xclbin> -d <file_name.lz4>
        3. To validate various files together:       ./build/xil_lz4_8b -cx <compress xclbin> -dx <decompress xclbin> -l <files.list>
            3.a. <files.list>: Contains various file names with current path    
        
   Note: Default arguments are set in Makefile

  Help:

        ===============================================================================================
        Usage: application.exe -[-h-cx-c-l-dx-d-B-x]

                --help,             -h      Print Help Options   Default: [false]
                --compress_xclbin   -cx     Compress Binary
                --compress,         -c      Compress
                --file_list,        -l      List of Input Files
                --decompress_xclbin -dx     Decompress Binary
                --decompress,       -d      Decompress
                --block_size,       -B      Compress Block Size [0-64: 1-256: 2-1024: 3-4096] Default: [0]
                --flow,             -x      Validation [0-All: 1-XcXd: 2-XcSd: 3-ScXd]   Default:[1]
        ===============================================================================================

```


### Limitations

#### Decompression

No support for block dependence case




