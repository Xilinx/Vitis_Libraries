# Codec Library

Codec Library is an open-sourced library written in C/C++ for accelerating image coding, decoding and related processing algorithms. Currently, 1 algorithm, JPEG decoding, is accelerated.

## Overview

The brief information about algorithms accelerated by Codec Library:

1. JPEG decoding: one L2 API is provided for accelerating entire JPEG decoding process, which supports the ‘Sequential DCT-based mode’ of ISO/IEC 10918-1 standard. It can process 1 Huffman token and create up to 8 DCT coefficients within one cycle. It is also an easy-to-use decoder as it can directly parse the JPEG file header without help of software functions. In addition, L1 API is provided for Huffman decoding.

## Benchmark Result

In `L2/demos`, thest Kernels are built into xclbin targeting U200. We achieved a good performance against several dataset, e.g. lena.png with latency of "value". For more details about the benchmarks, please kindly find them in [benchmark results](https://xilinx.github.io/Vitis_Libraries/codec/2022.1/benchmark.html).

## Documentations

For more details of the Codec library, please refer to [xf_codec Library Documentation](https://xilinx.github.io/Vitis_Libraries/codec/2022.1/index.html).

## License

Licensed using the [Apache 2.0 license](https://www.apache.org/licenses/LICENSE-2.0).

    Copyright 2022 Xilinx, Inc.
    
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
    
        http://www.apache.org/licenses/LICENSE-2.0
    
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
    Copyright 2022 Xilinx, Inc.

## Contribution/Feedback

Welcome! Guidelines to be published soon.


