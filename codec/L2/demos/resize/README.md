.. 
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

# Resize 

Renumber example resides in ``L2/demos/reszie`` directory. The tutorial provides a step-by-step guide that covers commands for building and running kernel.

## Executable Usage

* **Work Directory(Step 1)**

The steps for library download and environment setup can be found in [here](https://github.com/Xilinx/Vitis_Libraries/tree/master/codec/L2/demos#building). For getting the design,

```
   cd L2/demos/resize
```   

* **Build kernel(Step 2)**

Run the following make command to build your XCLBIN and host binary targeting a specific device. Please be noticed that this process will take a long time, maybe couple of hours.

```
   make run TARGET=hw PLATFORM=xilinx_u50_gen3x16_xdma_201920_3
```   

* **Run kernel(Step 3)**

To get the benchmark results, please run the following command.

```
   ./build_dir.hw.xilinx_u50_gen3x16_xdma_201920_3/host.exe -xclbin build_dir.hw.xilinx_u50_gen3x16_xdma_201920_3/kernel_resize.xclbin -i images/t0.raw -srcw 512 -srch 512 -dstw 256 -dsth 256 
```   

Renumber Input Arguments:

```
   Usage: host.exe -[-xclbin -i -srcw -srch -dstw -dsth]
         -xclbin:           the kernel name
         -i:                the input bin file
         -srcw:             the source image width
         -srch:             the source image height
         -dstw:             the destination width 
         -dsth:             the destination height
```         

Note: Default arguments are set in Makefile, the data have only one column that the node's community id is divided by other clustering algorithm, for example louvain.

* **Example output(Step 4)** 

```
    Info: Program created
    Info: Kernel created
    kernel has been created
    INFO: kernel start------
    INFO: kernel end------
    INFO: Execution time 15644.1ms
    Info: Time in host-to-device: 751.56ms
    Info: Time in kernel: 14000.9ms
    Info: Time in device-to-host: 891.515ms
    The src image size is 512*512.
    The dst image size is 64*64.
    Image resized successfully.
    PASS: no error found.
    Info: Test passed
```    
    
## Profiling

The hardware resource utilizations are listed in the following table.
Different tool versions may result slightly different resource.

Table 1 : Hardware resources for Resize 

    +---------------------+----------+----------+----------+----------+---------+-----------------+
    |    Kernel           |   BRAM   |   URAM   |    DSP   |    FF    |   LUT   | Frequency(MHz)  |
    +---------------------+----------+----------+----------+----------+---------+-----------------+
    |  kernel_resize(1x)  |    14    |    0     |    53    |   8635   |  6566   |      397.1      |
    +---------------------+----------+----------+----------+----------+---------+-----------------+
    |  kernel_resize(8x)  |    29    |    0     |    168   |   20824  |  15087  |      340.9      |
    +---------------------+----------+----------+----------+----------+---------+-----------------+

Table 2 : Resize FPGA acceleration benchmark 

    +---------------+-----------+--------------------+-----------------+
    |    Inputs     |   Size    |  FPGA 1x/8x (ms)   |   Fps 1x / 8x   |
    +---------------+-----------+--------------------+-----------------+
    |   7680*4320   |  512*512  |    84.30 / 12.55   |  11.86 / 79.67  |
    +---------------+-----------+--------------------+-----------------+
    |   7680*4320   | 1920*1080 |    84.35 / 12.43   |  11.86 / 80.46  | 
    +---------------+-----------+--------------------+-----------------+
    |   7680*4320   | 3840*2160 |    84.34 / 12.43   |  11.86 / 80.46  | 
    +---------------+-----------+--------------------+-----------------+

Note: This table is the result of each image resize down 8 times.

.. Note::

   1. Resize running on Intel(R) Xeon(R) Silver 4116 CPU @ 2.10GHz, cache(16896 KB), cores(12).
   2. time unit: ms.

.. toctree::
    :maxdepth: 1
