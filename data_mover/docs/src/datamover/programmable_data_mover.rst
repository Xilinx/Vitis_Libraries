.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _L2_DATAMOVER_LABEL:

.. toctree::
   :caption: Table of Contents
   :maxdepth: 3

All Data-Mover designs are "codeless" that you need to create the OpenCL™ kernels by simply calling the **Kernel Generator** with a JSON description and ROM content (if needed) as a text file.

The **Kernel Generator** consists of:

- Kernel templates (in Jinja2), which can be instantiated through configurations from JSON.
- Data converter to transform the user-provided texture ROM content into a usable initialization file.
- Python script to automate the kernel generation from JSON to HLS C++ OpenCL kernel, which is ``L2/scripts/internal/generate_kernels.py``.

.. ATTENTION::
    Generated kernels are not self-contained source code, they would reference low-level block implementation headers in the ``L1/include`` folder. Ensure that folder is passed to the AMD Vitis™ compiler as a header search path when compiling project using generated programmable logic (PL) kernels.

Programmable 4D Data-Mover
===========================

.. _programmable-features:

Feature
--------

The AI Engine (AIE) application often needs to deal with multi-dimension data. The most common cases are that the AIE application needs to select and read/write a regularly distributed sub-set from a multi-dimensional array. Because a multi-dimension array has to be stored in a linear addressing memory space, such sub-set is rarely a contiguous block in memory but always a lot of data segment. It is not convenient for you to implement logic to calculate all segments' address and size and not efficient for AIE to do so.

The Programmable 4D Data-Mover includes:

- Concise descriptor design that uses 9x64 bits to fully describe access on 4-dimension (and low-dimension) data.
- Template kernel design that can read multiple descriptors and accomplish the defined access pattern one by one.

Descriptor Design
^^^^^^^^^^^^^^^^^

The Descriptor design is the most important part of the Programmable 4D Data-Mover. It defines:

- How 4-dimension data is mapped to linear addresses
- Where to find the sub-set to access
- What is the dimension of sub-set
- How to serialize the sub-set

To store a 4D array, ``A[W][Z][Y][X]``, in memory, it has to be mapped into a linear address space which will certainly lead to addressing like ``&A[w][z][y][x] = bias + w * (Z*Y*X) + z * (Y*X) + y * (X) + x``. In such a condition, adjacent elements in the 4D array will have const strid. Then nine parameters { bias (address of first element), X, X_stride, Y, Y_stride, Z, Z_stride, W, W_stride } will be enough to define the 4D array in memory.

- &A[w][z][y][x+1] – &A[w][z][y][x] = 1             (X_stride)
- &A[w][z][y+1][x] – &A[w][z][y][x] = X             (Y_stride)
- &A[w][z+1][y][x] – &A[w][z][y][x] = X * Y         (Z_stride)
- &A[w+1][z][y][x] – &A[w][z][y][x] = X * Y * Z     (W_stride)

Because the Programmable 4D Data Mover needs to write to/read from the AXI4-Stream, it also needs to define how to serialize the 4D array. Define ``ap_int<64> cfg[9]`` as descriptor to define one access:

- cfg[0]: Bias (address of the 4D array's first element in memory)
- cfg[1], cfg[2]: Stride of the first accessed dimension, size of the first accessed dimension
- cfg[3], cfg[4]: Stride of the second accessed dimension, size of the second accessed dimension
- cfg[5], cfg[6]: Stride of the third accessed dimension, size of the third accessed dimension
- cfg[7], cfg[8]: Stride of the fourth accessed dimension, size of the fourth accessed dimension

With the descriptor above, the Programmable 4D Data Mover will serialize the read/write as the following pseudo code (take read as example). The Programmable 4D Data Mover will load one or multiple descriptors from a descriptor buffer. The descriptor buffer begins with a 64-bit ``num`` which indicate how many descriptors are there in the buffer. Then ``num`` will be followed by one or multiple 9x64 bit descriptors, all compact stored. It will start parsing the first descriptor, finish the access, then parse and finish the next descriptor. It will keep the processing until it finishes all descriptors.

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

Kernel Design
^^^^^^^^^^^^^

Programmable 4D Data Movers are templated designs to access elements of 32/64/128/256/512 bits width. They have a standalone AXI master port to access the descriptor buffer. The AXI master to access descriptors are configured to be 64 bits wide. Other AXI master and AXI4-Stream ports are configured to be same width of data elements. AXI master ports share the same "latency" "outstanding" "burst length" setup, and they should be the same with the pragma setup in the kernels that wrap up the data mover. They share the same kernel generator and JSON spec; take reference from the following example.

.. image:: /images/4d_kernl_interface.png 
   :alt: various pattern
   :width: 100%
   :align: center

The Programmable 4D Data Mover's performance depends on:

- Compile time: Data width, larger data width, and larger bandwidth.
- Runtime: cfg[1] of each descriptor. When cfg[1] = 1, it will lead to burst access and bandwidth will be nice; otherwise it will lead to non-burst access and bandwidth will not be as good as burst access.

.. image:: /images/dm_perf.png 
   :alt: various pattern
   :width: 50%
   :align: center

.. _programmable-build-config:

Build Time Configuration
-------------------------

Example Kernel Specification (JSON)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The following kernel specification in JSON describes a ``4DCuboidRead`` kernel and a ``4DCuboidWrite`` kernel.

The ``4DCuboidRead`` kernel should have two data paths as you can see that there are two specifications in the ``map`` field. For example, take the first data path, it will use an AXI-M port ``din_0`` to read 4D array and AXI-M ``desp_0`` to access the descriptor buffer. Both AXI-M ports have "latency", "outstanding", and "burst_len" set up in their HLS kernel pragma. Its output port is AXI4-Stream ``dout_0``, and both the ``din_0`` and ``dout_0``'s port width are 64.

The ``4DCuboidWrite`` kernel should have two data paths, as you can see that there are two specifications in the ``map`` field. For example, take the first data path, it will use an AXI4-Stream port, ``din_0``, to read the 4D array and AXI-M ``desp_0`` to access the descriptor buffer. It will use the AXI-M port, ``dout_0``, for output. Both AXI-M ports have "latency", "outstanding", and "burst_len" set up in their HLS kernel pragma. Both the ``din_0`` and ``dout_0``'s port width are 64.

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

Example of How to Generate Kernels
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

    cd L2/tests/datamover/4D_datamover
    make pre_build
    # The pre_build command is as follows:
    # pre_build:
    #     make -f $(CUR_DIR)/ksrc.mk GENKERNEL=$(XFLIB_DIR)/L2/scripts/generate_kernels SPEC=$(CUR_DIR)/kernel/spec.json TOOLDIR=$(CUR_DIR)/_krnlgen

Example of How to Run Hardware Emulation of Hardware
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   cd L2/tests/datamover/4D_datamover
   source /opt/xilinx/Vitis/2022.2/settings64.sh
   source /opt/xilinx/xrt/setup.sh
   export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
   make run TARGET=hw PLATFORM=${PLATFORM_REPO_PATHS}/xilinx_vck190_base_202210_1/xilinx_vck190_base_202210_1.xpfm

.. _programmable-runtime-config:

Runtime Configuration
-----------------------

The Programmable 4D Data-Mover will read multiple descriptors from a descriptor buffer which can be configured from the host side in runtime. The descriptor buffer is compacted store in memory and start with 1x64 bit ``num`` that indicate how many descriptors are there in the buffer. Then ``num`` is followed by one or multiple descriptors.

Here are examples of the descriptor buffer and corresponding patterns. The underlying array is a 3D array (due to difficulties to draw a actual 4D array): ``A[10][7][8]`` which could be treated as ``A[1][10][7][8]``.
Its first element's address is 0, which means 'bias' = 0. Its size means that W = 1, Z = 10, Y = 7, and X = 8. Assume that its mapping lead to W_stride = 0 (), Z_stride = 56, Y_stride = 8, X_stride = 1.

.. code-block:: cpp

   {4,
    0, 1, 8, 8, 7, 56, 10, 0, 1,
    0, 8, 7, 1, 8, 56, 10, 0, 1,
    0, 56, 10, 8, 7, 1, 8, 0, 1,
    4, 1, 4, 8, 3, 56, 2, 0, 1}

The first number in buffer is ``4`` which means there are four descriptors followed. Their implied pattern are:

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