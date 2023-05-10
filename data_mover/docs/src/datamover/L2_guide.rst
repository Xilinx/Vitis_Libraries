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


All static Data-Mover designs are "codeless" that user need to create the OpenCL kernels by simply calling the **Kernel Generator** with a JSON description and ROM content (if needed) as text file.

**Kernel Generator** consists of:

- Kernel templates (in Jinja2), which can be instantiated through configurations from JSON
- Data converter to transform the user provided texture ROM content into usable initialization file
- Python script to automate the kernel generation from JSON to HLS C++ OpenCL kernel, which is ``L2/scripts/internal/generate_kernels.py``.

.. ATTENTION::
    Generated kernels are not self-contained source code, they would reference low-level block implementation headers in ``L1/include`` folder.
    Ensure that folder is passed to Vitis compiler as header search path when compiling project using generated PL kernels.


Programmable 4D Data-Mover
===========================

.. _programmable-features:

Feature
--------

AIE application often need to deal with multi-dimension data.
The mostly common cases are that AIE application need to select and read/write a regularly distributed sub-set from multi-dimensional array.
Because multi-dimension array has to be stored in linear addressing memory space, such sub-set is rarely a contiguous block in memory but always a lot of data segment.
It's not convenient for user to implement logic to calculate all segments' address and size and not efficient for AIE to do so.

Programmable 4D Data-Mover includes:

- Concise descriptor design that use 9x64bits to fully describe access on 4-dimension (and low-dimension) data.
- Template kernel design that can read multiple descriptor and accomplish the defined access pattern one by one.

**Descriptor Design**

Descriptor design is the most important part of Programmable 4D Data-Mover. It defines:

- How 4-dimension data mapped to linear address
- Where to find the sub-set to access
- What's the dimension of sub-set
- How to serialize the sub-set

To store 4D array ``A[W][Z][Y][X]`` in memory, it has to be mapped into linear address space which will certainly lead to addressing like ``&A[w][z][y][x] = bias + w * (Z*Y*X) + z * (Y*X) + y * (X) + x``. In such condition, adjacent elements in 4D array will have const strid. Then 9 parameters { bias (address of first element), X, X_stride, Y, Y_stride, Z, Z_stride, W, W_stride } will be enough to define the 4D array in memory.

- &A[w][z][y][x+1] – &A[w][z][y][x] = 1             (X_stride)
- &A[w][z][y+1][x] – &A[w][z][y][x] = X             (Y_stride)
- &A[w][z+1][y][x] – &A[w][z][y][x] = X * Y         (Z_stride)
- &A[w+1][z][y][x] – &A[w][z][y][x] = X * Y * Z     (W_stride)

Since Programmable 4D Data Mover need to write to / read from AXI stream, it also needs to define how to serialize 4D array.
We define ``ap_int<64> cfg[9]`` as descriptor to define one access:

- cfg[0]:           bias (address of 4D array's first element in memory)
- cfg[1], cfg[2]:   stride of first accessed dimension, size of first accessed dimension
- cfg[3], cfg[4]:   stride of second accessed dimension, size of second accessed dimension
- cfg[5], cfg[6]:   stride of third accessed dimension, size of third accessed dimension
- cfg[7], cfg[8]:   stride of fourth accessed dimension, size of fourth accessed dimension

With the descriptor above, Programmable 4D Data Mover will serialize the read/write as pseudo-code below (take read as example):
Programmable 4D Data Mover will load one or multiple descriptors from a descriptor buffer.
The descriptor buffer begins with a 64bits ``num`` which indicate how many descriptors are there in the buffer.
Then ``num`` will be followed by one or multiple 9x64bits descriptors, all compact stored.
It will start parsing the first descriptor, finish the access, then parse and finish the next descriptor.
It will keep the processing until it finishes all descriptors.

.. code-block:: cpp

    for(ap_int<64> d4 = 0; d4 < cfg[8]; d4++) {
        for(ap_int<64> d3 = 0; d3 < cfg[6]; d3++) {
            for(ap_int<64> d2 = 0; d2 < cfg[4]; d2++) {
                for(ap_int<64> d1 = 0; d1 < cfg[2]; d1++) {
                    elem_address = cfg[0] + d4 * cfg[7] + d3 * cfg[5] + d2 * cfg[3] + d1 * cfg[1];
                    data_to_be_read = data[elem_address];
                }
            }
        }
    }

**Kernel Design**

Programmable 4D Data Movers are templated design to access elements of 32 / 64 / 128 / 256 / 512 bits width.
They have standalone AXI master port to access descriptor buffer.
AXI master to access descriptors are configured to be 64 bits wide. Other AXI master and AXI stream port are configured to be same width of data elements.
AXI master ports share the same "latency" "outstanding" "burst length" setup, and they should be the same with the pragma setup in kernels that wrap up the data mover.
They share the same kernel generator and JSON spec, please take reference from example below.

.. image:: /images/4d_kernl_interface.png 
   :alt: various pattern
   :width: 100%
   :align: center

Programmable 4D Data Mover's performance depends on:

- Compile time: data width, larger data width, larger bandwidth.
- Run time: cfg[1] of each descriptor. When cfg[1] = 1, it will lead to burst access and bandwidth will be nice, otherwise it will lead to non-burst access and bandwidth won't be as good as burst access.

.. image:: /images/dm_perf.png 
   :alt: various pattern
   :width: 50%
   :align: center

.. _programmable-build-config:

Build Time Configuration
-------------------------

**Example Kernel Specification (JSON)**

The following kernel specification in JSON describes a ``4DCuboidRead`` kernel and a ``4DCuboidWrite`` kernel.

The ``4DCuboidRead`` kernel should have 2 data paths as we can see that there are 2 specifications in ``map`` field. 
Let's take the first data path for example, it will use AXI-M port ``din_0`` to read 4D array and AXI-M ``desp_0`` to access descriptor buffer.
Both AXI-M ports have "latency", "outstanding" and "burst_len" setup in their HLS kernel pragma. Its output port is AXI-stream ``dout_0`` and both ``din_0`` and ``dout_0``'s port width are 64.

The ``4DCuboidWrite`` kernel should have 2 data paths as we can see that there are 2 specifications in ``map`` field. 
Let's take the first data path for example, it will use AXI-stream port ``din_0`` to read 4D array and AXI-M ``desp_0`` to access descriptor buffer.
It will use AXI-M port ``dout_0`` for output. Both AXI-M ports have "latency", "outstanding" and "burst_len" setup in their HLS kernel pragma. Both ``din_0`` and ``dout_0``'s port width are 64.


.. code-block:: JSON

    {
        "cuboid_read": {
            "impl": "4DCuboidRead",
            "map": [
                {
                    "in_port": {
                        "buffer": "din_0",
                        "descriptors": "desp_0",
                        "latency": 32,
                        "outstanding": 32,
                        "burst_len": 32
                    },
                    "out_port": {
                        "stream": "dout_0",
                        "width": 64
                    }
                },
                {
                    "in_port": {
                        "buffer": "din_1",
                        "descriptors": "desp_1",
                        "latency": 32,
                        "outstanding": 32,
                        "burst_len": 32
                    },
                    "out_port": {
                        "stream": "dout_1",
                        "width": 64
                    }
                }
            ]
        },
        "cuboid_write": {
            "impl": "4DCuboidWrite",
            "map": [
                {
                    "in_port": {
                        "stream": "din_0",
                        "descriptors": "desp_0",
                        "width": 64
                    },
                    "out_port": {
                        "buffer": "dout_0",
                        "latency": 32,
                        "outstanding": 32,
                        "burst_len": 32
                    }
                },
                {
                    "in_port": {
                        "stream": "din_1",
                        "descriptors": "desp_1",
                        "width": 64
                    },
                    "out_port": {
                        "buffer": "dout_1",
                        "latency": 32,
                        "outstanding": 32,
                        "burst_len": 32
                    }
                }
            ]
        }
    }

**Example of How to generate kernels**

.. code-block:: bash

    cd L2/tests/datamover/4D_datamover
    make pre_build
    # The pre_build command is as follows:
    # pre_build:
    #     make -f $(CUR_DIR)/ksrc.mk GENKERNEL=$(XFLIB_DIR)/L2/scripts/generate_kernels SPEC=$(CUR_DIR)/kernel/spec.json TOOLDIR=$(CUR_DIR)/_krnlgen

**Example of How to run hardware emulation of hardware**

.. code-block:: bash

   cd L2/tests/datamover/4D_datamover
   source /opt/xilinx/Vitis/2022.2/settings64.sh
   source /opt/xilinx/xrt/setup.sh
   export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
   make run TARGET=hw PLATFORM=${PLATFORM_REPO_PATHS}/xilinx_vck190_base_202210_1/xilinx_vck190_base_202210_1.xpfm

.. _programmable-runtime-config:

Run Time Configuration
-----------------------

Programmable 4D Data-Mover will read multiple descriptors from descriptor buffer which can be configed from host side in run-time.
Descriptor buffer is compacted store in memory and start with 1x64bit ``num`` that indicate how many descriptors are there in the buffer.
Then ``num`` is followed by one or multiple descriptors.

Here's examples of descriptor buffer and corresponding patterns.
The underlying array is a 3D array (due to difficulties to draw a actual 4D array): ``A[10][7][8]`` which could be treated as ``A[1][10][7][8]``.
Its first element's address is 0, which means 'bias' = 0. Its size means that W = 1, Z = 10, Y = 7, X = 8.
We can assume that its mapping lead to W_stride = 0 (), Z_stride = 56, Y_stride = 8, X_stride = 1.

.. code-block:: cpp

   {4,
    0, 1, 8, 8, 7, 56, 10, 0, 1,
    0, 8, 7, 1, 8, 56, 10, 0, 1,
    0, 56, 10, 8, 7, 1, 8, 0, 1,
    4, 1, 4, 8, 3, 56, 2, 0, 1}


The first number in buffer is ``4`` which means there are 4 descriptors followed.
Their implied pattern are as below:

Descriptor[0]: {0, 1, 8, 8, 7, 56, 10, 0, 1}. The implied pattern is:

.. code-block:: cpp

    for(ap_int<64> d4 = 0; d4 < 1; d4++) { // cfg[8] = 1
        for(ap_int<64> d3 = 0; d3 < 10; d3++) { // cfg[6] = 10
            for(ap_int<64> d2 = 0; d2 < 7; d2++) { // cfg[4] = 7
                for(ap_int<64> d1 = 0; d1 < 8; d1++) { // cfg[2] = 8
                    // cfg[7] = 0, cfg[5] = 56, cfg[3] = 8, cfg[1] = 1, cfg[0] = 0
                    elem_address = d4 * 0 + d3 * 56 + d2 * 8 + d1 * 1 + 0;
                    data_to_be_read = data[elem_address];
                }
            }
        }
    }

.. image:: /images/X_Y_Z_W_pattern.png
   :alt: various pattern
   :width: 30%
   :align: center

Descriptor[1]: {0, 8, 7, 1, 8, 56, 10, 0, 1}. The implied pattern is:

.. code-block:: cpp

    for(ap_int<64> d4 = 0; d4 < 1; d4++) { // cfg[8] = 1
        for(ap_int<64> d3 = 0; d3 < 10; d3++) { // cfg[6] = 10
            for(ap_int<64> d2 = 0; d2 < 8; d2++) { // cfg[4] = 8
                for(ap_int<64> d1 = 0; d1 < 7; d1++) { // cfg[2] = 7
                    // cfg[7] = 0, cfg[5] = 56, cfg[3] = 1, cfg[1] = 8, cfg[0] = 0
                    elem_address = d4 * 0 + d3 * 56 + d2 * 1 + d1 * 8 + 0;
                    data_to_be_read = data[elem_address];
                }
            }
        }
    }

.. image:: /images/Y_X_Z_W_pattern.png
   :alt: various pattern
   :width: 30%
   :align: center

Descriptor[2]: {0, 56, 10, 8, 7, 1, 8, 0, 1}. The implied pattern is:

.. code-block:: cpp

    for(ap_int<64> d4 = 0; d4 < 1; d4++) { // cfg[8] = 1
        for(ap_int<64> d3 = 0; d3 < 8; d3++) { // cfg[6] = 8
            for(ap_int<64> d2 = 0; d2 < 7; d2++) { // cfg[4] = 7
                for(ap_int<64> d1 = 0; d1 < 10; d1++) { // cfg[2] = 10
                    // cfg[7] = 0, cfg[5] = 1, cfg[3] = 8, cfg[1] = 56, cfg[0] = 0
                    elem_address = d4 * 0 + d3 * 1 + d2 * 8 + d1 * 56 + 0;
                    data_to_be_read = data[elem_address];
                }
            }
        }
    }

.. image:: /images/Z_Y_X_W_pattern.png
   :alt: various pattern
   :width: 30%
   :align: center

Descriptor[3]: {4, 1, 4, 8, 3, 56, 2, 0, 1}. The implied pattern is:

.. code-block:: cpp

    for(ap_int<64> d4 = 0; d4 < 1; d4++) { // cfg[8] = 1
        for(ap_int<64> d3 = 0; d3 < 2; d3++) { // cfg[6] = 2
            for(ap_int<64> d2 = 0; d2 < 3; d2++) { // cfg[4] = 3
                for(ap_int<64> d1 = 0; d1 < 4; d1++) { // cfg[2] = 4
                    // cfg[7] = 0, cfg[5] = 56, cfg[3] = 8, cfg[1] = 1, cfg[0] = 4
                    elem_address = d4 * 0 + d3 * 56 + d2 * 8 + d1 * 1 + 4;
                    data_to_be_read = data[elem_address];
                }
            }
        }
    }

.. image:: /images/X_Y_Z_W_sub_pattern.png
   :alt: various pattern
   :width: 30%
   :align: center


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

