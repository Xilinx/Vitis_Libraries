# L2: Kernels

## Data-Mover Generator

This layer provides a tool (`L2/scripts/generate_kernels`) to create data-mover kernel source files
form description written in JSON format.
This tool aims to help developers to quickly create helper kernels to verify, benchmark and deploy their design
in hardware. No HLS kernel development experience is required to use this tool.

The `src/xf_data_mover` subfolder contains the template of kernels used by the tool,
in [Jinja2](https://jinja.palletsprojects.com/en/2.11.x/) format.

## License


Licensed using the [Apache 2.0 license](https://www.apache.org/licenses/LICENSE-2.0).

    Copyright (C) 2019-2022, Xilinx, Inc.
    Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
    
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
    
        http://www.apache.org/licenses/LICENSE-2.0
    
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
