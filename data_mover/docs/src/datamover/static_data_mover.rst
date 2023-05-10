.. 
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

.. _L2_DATAMOVER_LABEL:

.. toctree::
   :caption: Table of Contents
   :maxdepth: 3


All Data-Mover designs are "codeless" that user need to create the OpenCL kernels by simply calling the **Kernel Generator** with a JSON description and ROM content (if needed) as text file.

**Kernel Generator** consists of:

- Kernel templates (in Jinja2), which can be instantiated through configurations from JSON
- Data converter to transform the user provided texture ROM content into usable initialization file
- Python script to automate the kernel generation from JSON to HLS C++ OpenCL kernel, which is ``L2/scripts/internal/generate_kernels.py``.

.. ATTENTION::
    Generated kernels are not self-contained source code, they would reference low-level block implementation headers in ``L1/include`` folder.
    Ensure that folder is passed to Vitis compiler as header search path when compiling project using generated PL kernels.


Static Data-Mover
==================

.. _static-features:

Feature
-------

``Static Data Mover`` has 9 types of kernels in 2 different categories.
They all access certain amount of data in Memory/URAM/BRAM in a continuous style.
This is the only access pattern.

Data to AIE:

- LoadDdrToStream: For loading data from PL's DDR to AIE through AXI stream
- LoadDdrToStreamWithCounter: For loading data from PL's DDR to AIE through AXI stream and recording the data count sending to AIE
- SendRomToStream: For sending data from on-chip BRAM to AIE through AXI stream
- SendRamToStream: The same as ``SendRomToStream``, but the difference is that the source data is coming from URAM instead of BRAM

Data from AIE:

- StoreStreamToMaster: For receiving data from AIE through AXI stream and save them to PL's DDR
- StoreStreamToMasterWithCounter: For receiving data from AIE through AXI stream and saving them to PL's DDR, as well as recording the data count sending to DDR
- ValidateStreamWithMaster: For receiving data from AIE through AXI stream and comparing with the goldens in PL's DDR, as well as putting the overall pass/fail flag into PL's DDR
- ValidateStreamWithRom: For receiving data from AIE through AXI stream and comparing with the goldens in PL's BRAM, as well as putting the overall pass/fail flag into PL's DDR
- ValidateStreamWithRam: For receiving data from AIE through AXI stream and comparing with the goldens in PL's URAM, as well as putting the overall pass/fail flag into PL's DDR

.. _static-build-config:

Build Time Configuration
-------------------------

**Example Kernel Specification (JSON)**

The following kernel specification in JSON describes a ``SendRomToStream`` kernel and a ``StoreStreamToMaster`` kernel.

The ``SendRomToStream`` kernel should have 2 data paths as we can see that there are 2 specifications in ``map`` field. Let's take the first data path for example, it will be using texture contents for ROM initialization from file named ``din0``, and the corresponding datatype is specified as ``int64_t``. As the ``num`` is set to 512, so the depth of the internal ROM should be 512. Since we want to send the data to AXI stream in II = 1, the on-chip ROM's width will be automatically generated regarding to the output port's width, that said 64-bit. The second data path will be auto-generated with the same rules.

The ``StoreStreamToMater`` kernel should also have 2 data paths. As we want to load the data from AIE through AXI stream in II = 1, the output port width for PL's DDR will be auto-generated regarding to the width of the corresponding AXI stream, that said 64-bit for 1st data path, 32-bit for 2nd data path.

.. code-block:: JSON

    {
        "rom2s_x2": {
            "impl": "SendRomToStream",
            "map": [
                {
                    "in_file": {
                        "name": "din0",
                        "type": "int64_t",
                        "num": 512
                    },
                    "out": {
                        "stream": "s0",
                        "width": 64
                    }
                },
                {
                    "in_file": {
                        "name": "din1",
                        "type": "int32_t",
                        "num": 1024
                    },
                    "out": {
                        "stream": "s1",
                        "width": 32
                    }
                }
            ]
        },
        "s2m_x2": {
            "impl": "StoreStreamToMaster",
            "map": [
                {
                    "in_port": {
                        "stream": "s0",
                        "width": 64
                    },
                    "out": {
                        "buffer": "p0"
                    }
                },
                {
                    "in_port": {
                        "stream": "s1",
                        "width": 32
                    },
                    "out": {
                        "buffer": "p1"
                    }
                }
            ]
        }
    }

Kindly refer to ``L2/tests/datamover`` for JSON format of all 9 types of kernels that can be generated.

**Example of How to generate kernels**

.. code-block:: bash

    cd L2/tests/datamover/load_master_to_stream
    make pre_build
    # The pre_build command is as follows:
    # pre_build:
    #     make -f $(CUR_DIR)/ksrc.mk GENKERNEL=$(XFLIB_DIR)/L2/scripts/generate_kernels SPEC=$(CUR_DIR)/kernel/spec.json TOOLDIR=$(CUR_DIR)/_krnlgen

**Example of How to run hardware emulation of hardware**

.. code-block:: bash

   cd L2/tests/datamover/load_master_to_stream
   source /opt/xilinx/Vitis/2022.2/settings64.sh
   source /opt/xilinx/xrt/setup.sh
   export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
   make run TARGET=hw PLATFORM=${PLATFORM_REPO_PATHS}/xilinx_vck190_base_202210_1/xilinx_vck190_base_202210_1.xpfm

.. ATTENTION::
   * Only HW_EMU and HW run available
   * Kernel-to-kernel streaming is not available in software emulation, design can only be enulated in hardware emulation.

Data Converter
--------------

This C-based data converter is used to transform the texture ROM contents to hexadecimal string with specific width that can be directly programmed into on-chip ROMs.

Please be noticed that there are several limitations for this data converter, so when you provide the JSON specifications or the ROM contents, these rules have to be followed:

- Maximum width of the output data width is 512-bit
- Output data width have to be wider than input
- Input texture contents should be provided line-by-line, that said ``\n`` separated
- Only the following input data types are supoorted

**Supported input datatypes**

+--------------+
| Element Type |
+==============+
| half         |
+--------------+
| float        |
+--------------+
| double       |
+--------------+
| int8_t       |
+--------------+
| int16_t      |
+--------------+
| int32_t      |
+--------------+
| int64_t      |
+--------------+

