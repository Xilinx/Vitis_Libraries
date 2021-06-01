# Codec Library

Codec Library is an open-sourced library written in C/C++ accelerating image processing including JPEG Decoder, WebP, Lepton, PIK and JPEG-XL. It now covers a level of acceleration: the module level(L1), the pre-defined kernel level(L2).

## Overview

The algorithms implemented by Codec Library include:

*  JPEG Decoder: "JPEG" stands for Joint Photographic Experts Group, the name of the committee that created the JPEG standard and also other still picture coding standards. The "Joint" stood for ISO TC97 WG8 and CCITT SGVIII.
*  PIK Encoder: PIK is a raster-graphics file format that supports both lossy and lossless compression. It is designed to outperform existing raster formats and thus to become their universal replacement. PIK is the prototype of JPEG-XL and its compression quality is equal to JPEG-XL speed 6/7.

## Benchmark Result

In `L2/demos`, thest Kernels are built into xclbin targeting U200. We achieved a good performance against several dataset, e.g. lena.png with latency of "value". For more details about the benchmarks, please kindly find them in [benchmark results](https://xilinx.github.io/Vitis_Libraries/graph/2021.1/benchmarks/results.html).


## Documentations

For more details of the Codec library, please refer to [xf_codec Library Documentation](https://xilinx.github.io/Vitis_Libraries/graph/2021.1/index.html).

## License

Licensed using the [Apache 2.0 license](https://www.apache.org/licenses/LICENSE-2.0).

    Copyright 2019 Xilinx, Inc.
    
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
    
        http://www.apache.org/licenses/LICENSE-2.0
    
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
    Copyright 2019 Xilinx, Inc.

## Contribution/Feedback

Welcome! Guidelines to be published soon.


