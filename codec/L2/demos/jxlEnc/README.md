JXL Encoder
===============

JXL Encoder example resides in ``L2/demos/jxlEnc`` directory. The tutorial provides a step-by-step guide that covers commands for building and running kernel.

Executable Usage
----------------

* **Work Directory(Step 1)**

The steps for library download and environment setup can be found in [here](https://github.com/Xilinx/Vitis_Libraries/tree/master/codec/L2/demos#building). For getting the design,

```
   cd L2/demos/jxlEnc
```

* **Build kernel(Step 2)**

Run the following make command to build your XCLBIN and host binary targeting a specific device. Please be noticed that this process will take a long time, maybe couple of hours.

```
   make run TARGET=hw PLATFORM=xilinx_u50_gen3x16_xdma_201920_3
```   

* **Run kernel(Step 3)**

To get the benchmark results, please run the following command.

```
  ./build_dir.hw.xilinx_u50_gen3x16_xdma_201920_3/host.exe --xclbin ./build_dir.hw.xilinx_u50_gen3x16_xdma_201920_3/jxlEnc.xclbin PNGFilePath JXLFilePath 
```   

JXL Encoder Input Arguments:

```
   Usage: host.exe -[-xclbin]
          --xclbin:     the kernel name
          PNGFilePath:  the path to the input *.PNG
          JXLFilePath:  the path to the output *.jxl
```          

Note: Default arguments are set in Makefile, you can use other [pictures](https://github.com/Xilinx/Vitis_Libraries/tree/master/codec/L2/demos#pictures) listed in the table.

* **Example output(Step 4)** 

```
   Found Platform
   Platform Name: Xilinx
   Info: Context created
   Info: Command queue created
   INFO: Found Device=xilinx_u50_gen3x16_xdma_201920_3
   INFO: Importing build_dir.hw_emu.xilinx_u50_gen3x16_xdma_201920_3/jxlEnc.xclbin
   Loading: 'build_dir.hw_emu.xilinx_u50_gen3x16_xdma_201920_3/jxlEnc.xclbin'
   Info: Program created
   Info: Kernel created
   INFO: kernel has been created
   INFO: Kernel Start
   INFO: Finish kernel execution
   INFO: Finish E2E execution
   ...

   INFO: Finish kernel execution
   INFO: Finish E2E execution
   INFO: Data transfer from host to device: 100 us
   INFO: Data transfer from device to host: 20 us
   INFO: kernel execution time: 600 ms
```

Profiling
---------

The hardware resource utilizations are listed in the following table.
Different tool versions may result slightly different resource.


##### Table 1 IP resources for JXL encoder 

|      IP                |   BRAM   |   URAM   |    DSP   |    FF    |   LUT   |
|------------------------|----------|----------|----------|----------|---------|
|    lossy_enc_compute   |    364   |    53    |    498   |   145111 |  121741 |
|    cluster_histogram   |    70    |    28    |    51    |   60744  |  38507  |
|    tokInit_histogram   |    150   |    41    |    95    |   64710  |  39289  |


##### Table 2 JXL Encoder Performance
  
###### lossy_enc_compute 
|       Image       |      Size     |  Time(ms)  |  Throughput(MP/s)  |
|-------------------|---------------|------------|--------------------|
|  lena_c_512.png   |    512x512    |    3.63    |        72.21       |     
|  hq_1024x1024.png |   1024x1024   |    13.06   |        80.29       |    
|  hq_2Kx2K.png     |   2048x2048   |    50.33   |        83.34       |  
  
###### cluster_histogram 
|       Image       |      Size     |  Time(ms)  |  Throughput(MP/s)  |
|-------------------|---------------|------------|--------------------|
|  lena_c_512.png   |    512x512    |    4.6     |        56.98       |     
|  hq_1024x1024.png |   1024x1024   |    14.6    |        71.82       |    
|  hq_2Kx2K.png     |   2048x2048   |    41.13   |        101.97      |   
  
###### tokInit_histogram 
|       Image       |      Size     |   Time(ms)  |  Throughput(MP/s)  |
|-------------------|---------------|-------------|--------------------|
|  lena_c_512.png   |    512x512    |    6.07     |        43.19       |     
|  hq_1024x1024.png |   1024x1024   |    18.03    |        58.16       |    
|  hq_2Kx2K.png     |   2048x2048   |    79.30    |        52.89       |   

## License

Licensed using the [Apache 2.0 license](https://www.apache.org/licenses/LICENSE-2.0).

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
