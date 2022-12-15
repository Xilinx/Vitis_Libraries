# Vitis Ultrasound Library

## Overview
Vitis Ultrasound library provides implementation of different L1/L2/L3 APIs as a toolbox for ultrasound image processing. Current version provides:
- L1, the lowest level of abstraction and is composed of simple BLAS operation
- L2, the functional units of the Beamformer, which can be obtained by composing L1 libraries
- L3, complete Beamformer which uses all of the three points above and contain run tests for beamforming design of PW/SA/Scanline

## Libraries contents
- [Overview of L1 apis](L1)
- [Overview of L2 apis](L2)
- [Overview of L3 apis](L3)

## Getting Start

### Download the Vitis Ultrasound Library
```bash
git clone https://github.com/Xilinx/Vitis_Libraries.git
cd Vitis_Libraries/ultrasound
```

### Run examples
- [Run L2 testcase](L2/tests) 
- [Run L3 testcase](L3/tests)

## License
Copyright 2022 AMD, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.