# Xilinx LZ4  

Xilinx LZ4 compression/decompression is FPGA based implementation of standard LZ4. 
Xilinx implementation of LZ4 application is aimed at achieving high throughput for both compression and decompression.
This Xilinx LZ4 application is developed and tested on Xilinx Alveo U200. To know more about standard LZ4 application please refer https://github.com/lz4/lz4 

This application is accelerated using generic hardware architecture for LZ based data compression algorithms.

![LZx compress select](../../../common/img/lzx_comp.png) <br />

![LZx decompress select](../../../common/img/lzx_decomp.png) <br />

For more details refer this [link](https://xilinx.github.io/Vitis_Libraries/data_compression/source/L2/design.html)

## Results

### Resource Utilization <br />

Table below presents resource utilization of Xilinx LZ4 compress/decompress
kernels with 8 engines for single compute unit. It is possible to extend number of engines to achieve higher throughput.


| Design | LUT | LUTMEM | REG | BRAM | URAM| DSP | Fmax (MHz) |
| --------------- | --- | ------ | --- | ---- | --- | -----| -----|
| Compression     | 51516(5.11%) | 14161(2.47%)|63838(3.01%)|58(3.20%) | 48(5.00%)|1(0.01%)|300|
| Decompression     | 30651(2.98%) | 13708(2.39%)|39520(1.85%)|146(7.93%)|0|1(0.01%)|300|


### Throughput & Compression Ratio

Table below presents the best end to end application throughput achieved with single compute unit during execution of this application.

| Topic| Results| 
|-------|--------|
|Best Compression Throughput|1.8 GB/s|
|Best Decompression Throughput| 1.8 GB/s |
|Average Compression Ratio| 2.13x (Silesia Benchmark)|

Note: Overall throughput can still be increased with multiple compute units.

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
        application.
```
#### Hardware

```
  make all TARGET=hw DEVICE=xilinx_u200_xdma_201830_2

  Note: This command compiles for hardware execution. It generates kernel binary ".xclbin" file. 
        This file is placed in ./build/xclbin*/ directory under LZ4 folder.

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
        4. To execute single file for compression and decompression : ./build/xil_lz4_8b -sx <compress_decompress xclbin> -v <file_name>  
        
   Note: Default arguments are set in Makefile

  Help:

        ===============================================================================================
        Usage: application.exe -[-h-cx-c-l-dx-d-sx-v-B-x]

                --help                  -h      Print Help Options   Default: [false]
                --compress_xclbin       -cx     Compress binary
                --compress              -c      Compress
                --file_list             -l      List of Input Files
                --decompress_xclbin     -dx     Decompress binary
                --decompress            -d      Decompress
                --compress_decompress   -sx     Compress_Decompress binary
                --validate              -v      Single file validate for Compress and Decompress  
                --block_size            -B      Compress Block Size [0-64: 1-256: 2-1024: 3-4096] Default: [0]
                --flow                  -x      Validation [0-All: 1-XcXd: 2-XcSd: 3-ScXd]   Default:[1]
        ===============================================================================================

```


### Limitations

#### Decompression

No support for block dependence case

