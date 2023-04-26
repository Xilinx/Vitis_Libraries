Webp Encoder
============

Webp encoder demo resides in ``L2/demo/webpEnc`` directory. The tutorial provides a step-by-step guide that covers commands for building and running kernel.

Executable Usage
----------------

* **Work Directory(Step 1)**

The steps for library download and environment setup can be found in [here](https://github.com/Xilinx/Vitis_Libraries/tree/master/codec/L2/demos#building). For getting the design,

```
   cd L2/demo/webpEnc
```   

* **Build kernel(Step 2)**

Run the following make command to build your XCLBIN and host binary targeting a specific device. Please be noticed that this process will take a long time, maybe couple of hours.

```
   make run TARGET=hw PLATFORM=xilinx_u200_xdma_201830_2
```   

* **Run kernel(Step 3)**

To get the benchmark results, please run the following command.

```
   ./build_dir.hw.xilinx_u200_xdma_201830_2/cwebp list.rst -use_ocl -q 80 -o output
```   

Webp Input Arguments:

```
   Usage: cwebp -[-use_ocl -q -o]
          list.rst:     the input list
          -use_ocl:     should be kept
          -q:           compression quality
          -o:           output directory
```          

Note: Default arguments are set in Makefile, you can use other [pictures](https://github.com/Xilinx/Vitis_Libraries/tree/master/codec/L2/demos#pictures) listed in the table.

* **Example output(Step 4)** 

```
   INFO: CreateKernel start.
   INFO: Number of Platforms: 1
   INFO: Selected Platform: Xilinx
   INFO: Number of devices for platform 0: 1
   INFO: target_device found:   xilinx_u200_xdma_201830_2
   INFO: target_device chosen:  xilinx_u200_xdma_201830_2
   NFO: OpenCL Version: 1.-48
   INFO: Loading kernel.xclbin
   INFO: Loading kernel.xclbin Finished

   ...

   *** Picture: 1 - 1,  Buffer: 0, Instance: 0, Event: 0 ***
   INFO: Host2Device finished. Computation time is 0.480000 (ms)
   INFO: PredKernel Finished. Computation time is 0.042000 (ms)
   INFO: ACKernel Finished. Computation time is 0.012000 (ms)
   INFO: Device2Host finished. Computation time is 0.005000 (ms)
   INFO: Loop of Pictures Finished. Computation time is 16.500000 (ms)
   INFO: VP8EncTokenLoopAsync Finished. Computation time is 22.676000 (ms)
   INFO: WebPEncodeAsync Finished. Computation time is 47.519000 (ms)
   INFO: Release Kernel.
```   

Profiling
---------

The hardware resource utilizations are listed in the following table.
Different tool versions may result slightly different resource.


Table 1 Hardware resources for webp kernels

|    Kernel    |   BRAM   |   URAM   |    DSP   |    FF    |   LUT   | Frequency(MHz) |
|--------------|----------|----------|----------|----------|---------|----------------|
|    Kernel1   |    72    |    10    |   410    |   56498  |  48301  |      250       |
|    Kernel2   |    11    |    0     |    5     |   23073  |  16375  |      250       |


* One instance achieves about 6~14 times acceleration. Here are some examples:


##### Table 2 Performance of Wepb Encoder for FPGA 

|   Kernel   | Width (pix) | Height (pix) | -q |  latency (ms) |  Throughput B(MB/s) | Throughput P(MB/s) | FPs (fps) |
|------------|-------------|--------------|----|---------------|---------------------|--------------------|-----------|
|   Kernel1  |    1920     |     1080     | 80 |     21.18     |       146.83        |       97.88        |   47.20   |
|   Kernel2  |    1920     |     1080     | 80 |     14.57     |       213.54        |       142.36       |   68.65   |
|   Kernel1  |    512      |     512      | 80 |     3.22      |       122.03        |       81.35        |   310.33  |
|   Kernel2  |    512      |     512      | 80 |     2.92      |       134.65        |       89.77        |   342.43  |
|   Kernel1  |    1920     |     1080     | 90 |     21.03     |       147.87        |       98.58        |   47.54   |
|   Kernel2  |    1920     |     1080     | 90 |     15.92     |       195.43        |       130.29       |   62.83   |
|   Kernel1  |    512      |     512      | 90 |     4.73      |       83.12         |       55.41        |   211.39  |
|   Kernel2  |    512      |     512      | 90 |     4.93      |       79.73         |       53.16        |   202.78  |


* Platform: FPGA U200, CPU details are listd belowd (single thread)

##### Note
```
    | 1. Kernels running on platform with Intel(R) Xeon(R) CPU E5-2603 v3 @ 1.60GHz, 48 Threads.
    | 2. time unit: ms.
    | 3. "-" Indicates that the result could not be obtained due to insufficient memory.
    | 4. FPGA time is the kernel runtime by adding data transfer and executed with webp encoder.
```    

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

