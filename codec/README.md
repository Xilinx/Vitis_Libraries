# Codec Library

Codec Library is an open-sourced library written in C/C++ for accelerating image coding, decoding and related processing algorithms. Currently, 6 kinds of algorithms are accelerated, including JPEG decoding, pik encoding, WebP encoding, lepton encoding JPEG-XL encoding and bicubic resizing.

## Overview

The brief information about algorithms accelerated by Codec Library:

1. JPEG decoding: one L2 API is provided for accelerating entire JPEG decoding process, which supports the ‘Sequential DCT-based mode’ of ISO/IEC 10918-1 standard. It can process 1 Huffman token and create up to 8 DCT coefficients within one cycle. It is also an easy-to-use decoder as it can directly parse the JPEG file header without help of software functions. In addition, L1 API is provided for Huffman decoding.
2. Pik encoding: 3 L2 APIs are provided for accelerating about 90% workload of lossy compression in Google’s pik. The pikEnc used the ‘fast mode’ of pik encoder which can provide better encoding efficiency than most of other still image encoding methods.
3. WebP encoding: 2 L2 APIs are provided for accelerating about 90% workload of lossy compression in WebP which is a popular image format developed by Google and supported in Chrome, Opera and Android, that is optimized to enable faster and smaller images on the Web.
4. Lepton encoding: the API ‘jpegDecLeptonEnc’ can be used for accelerating the encoding process for a new image format 'Lepton' developed by Dropbox. The format can save about 22% size of JPEG images losslessly.
5. JPEG-XL encoding: 3 L2 APIs are provided for accelerating the lossy encoding process of the JPEG XL Image Coding System (ISO/IEC 18181). Currently, not all computing intensive modules are offloaded, and more accelerating APIs will be available in feature.
6. Bicubic resizing: the L2 APIs 'resizeTop' is based on bicubic algorithm, which can take 1 or 8 input samples per cycle. When taking 8 samples, it can process 80 8K images per second. Although resizing is not a coding or encoding algorithm, it is widely used with image codecs in image transcoding applications.

## Source Files

Vitis development environment supports a variety of build flows based on the target engine and source type.

Examples and tests in L1, L2, and L3 use these unit level kernels in various ways for building a project.

## Application Development
Vitis library is organized into L1, L2, and L3 folders, each relating to a different stage of application development.

**L1** :
      Makefiles and sources in L1 facilitate HLS based flow for a quick check without considering complexities of Platform and OpenCL/XRT framework. Tasks at this level include:

* Check the functionality of an individual kernel (C-simulation)
* Estimate resource usage, latency, etc. (Synthesis)
* Run cycle accurate simulations (Co-simulation)
* Package as IP and get final resource utilization/timing details (Export RTL)
       
	**Note**:  Once RTL (or XO file after packaging IP) is generated, the Vivado flow is invoked for XCLBIN file generation if required.

**L2** :
       Makefiles and sources in L2 facilitate building XCLBIN file from various sources (HDL, HLS or XO files) of kernels with host code written in OpenCL/XRT framework targeting a device. This flow supports:

* Software emulation to check the functionality
* Hardware emulation to check RTL level simulation
* Build and test on hardware

**L3** :
       Makefiles and sources in L3 demonstrate applications developed involving multiple kernels in pipeline. These Makefiles can be used for executing tasks, as with the L2 Makefiles.

## Benchmark Result

In `L2/demos`, thest Kernels are built into xclbin targeting U200. We achieved a good performance against several dataset, e.g. lena.png with latency of "value". For more details about the benchmarks, please kindly find them in [benchmark results](https://docs.xilinx.com/r/en-US/Vitis_Libraries/codec/benchmark.html).

## Documentations

For more details of the Codec library, please refer to [xf_codec Library Documentation](https://docs.xilinx.com/r/en-US/Vitis_Libraries/codec/index.html).

## License

Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
SPDX-License-Identifier: X11


## Contribution/Feedback

Welcome! Guidelines to be published soon.


