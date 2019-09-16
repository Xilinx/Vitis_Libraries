# Xilinx LZ4  

Xilinx LZ4 compression/decompression is FPGA based implementation of standard LZ4. 
Xilinx implementation of LZ4 application is aimed at achieving high throughput for both compression and decompression.
This Xilinx LZ4 application is developed and tested on Xilinx Alveo U200. To know
more about standard LZ4 application please refer https://github.com/lz4/lz4

This application is accelerated using generic hardware architecture for LZ based data compression algorithms.

![LZx decompress select](../../../common/img/lzx_decomp.png) <br />

For more details refer this [link](https://gitenterprise.xilinx.com/heeran/xil_lz4/blob/master/README.md)


## Results

### Resource Utilization <br />

Table below presents resource utilization of Xilinx LZ4 compress/decompress
kernels with 8 engines for single compute unit. It is possible to extend number of engines to achieve higher throughput.


| Design | LUT | LUTMEM | REG | BRAM | URAM| DSP | Fmax (MHz) |
| --------------- | --- | ------ | --- | ---- | --- | -----| -----|
| Unpacker          |  7295 (1.71%)|  105 (0.07%)|11867 (1.30%)| 9 (1.27%)|0|0|
| Decompression     | 39042 (9.17%) |  13756 (8.93%)| 48499 (5.32%)|146 (20.59%)|0| 3 (0.15%)|247|



### Throughput & Compression Ratio

Table below presents the best throughput achieved during execution of this application.

| Topic| Results| 
|-------|--------|
|Best Decompression End to End Throughput| 950 MB/s |
|Best Decompression Kernel Throughput| 1.5 GB/s |

Note: Overall throughput can still be increased with multiple compute units.

## Software & Hardware

```
  Software: Xilinx Vitis 2019.2
  Hardware: xilinx_samsung_v0_80
```
 
## Usage


### Build Steps

#### Emulation flows
```
  make run TARGET=<sw_emu/hw_emu> DEVICE=xilinx_samsung_v0_80
  
  Note: This command compiles for targeted emulation mode and executes the
        application. To execute it on AWS F1 instance, please follow instructions
        specific to AWS F1 emulation.
```
#### Hardware

```
  make all TARGET=hw DEVICE=xilinx_samsung_v0_80

  Note: This command compiles for hardware execution. It generates kernel binary ".xclbin" file. 
        This file is placed in ./xclbin directory under LZ4 folder. To execute it on AWS F1 instance, 
        please follow instructions specific to AWS F1 deployment process.

```

### Execution Steps

While using PARALLEL_BLOCK (8 default) the generated executable would be
"xil_lz4_8b"

```
  Input Arguments: 
    
        1. To execute single file for decompression: ./build/xil_lz4_8b -dx <decompress xclbin> -d <file_name.lz4>
        2. To validate various files together:       ./build/xil_lz4_8b -dx <decompress xclbin> -l <files.list>
            2.a. <files.list>: Contains various file names with current path    
        
   Note: Default arguments are set in Makefile

  Help:

        ===============================================================================================
        Usage: application.exe -[-h-l-dx-d-B-x]

                --help,             -h      Print Help Options   Default: [false]                
                --file_list,        -l      List of Input Files
                --decompress_xclbin -dx Decompress Binary
                --decompress,       -d      Decompress
                --block_size,       -B      Compress Block Size [0-64: 1-256: 2-1024: 3-4096] Default: [0]
                --flow,             -x      Validation [0-All: 1-XcXd: 2-XcSd: 3-ScXd]   Default:[1]
        ===============================================================================================

```


### Limitations

#### Decompression

No support for block dependence case

### P2P feature

In the P2P mode, the data never comes to host memory but directly transferred to/from FPGA DDR from/to SSD by the SSD DMA controller.

input_file: path to the input file. Should be present on NVMe disk in p2p mode.

Add multi board validation support by passing device ID through command line args (-id <device_id>).
device_id value should be in range of total number of identified devices.
By default, device_id set to 0.
If total identified devices are 2, then we can select device0 using -id 0 optionand device1 using -id 1 through command line args.
