.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _L2_DATAMOVER_LABEL:

.. toctree::
   :caption: Table of Contents
   :maxdepth: 3

All Data Mover designs are codeless — you create the OpenCL™ kernels by calling the **Kernel Generator** with a JSON description and ROM content (if needed) as a text file.

The **Kernel Generator** consists of:

- Kernel templates (in Jinja2), which can be instantiated through configurations from JSON.
- Data converter to transform the user-provided texture ROM content into a usable initialization file.
- Python script to automate kernel generation from JSON to HLS C++ OpenCL kernel: ``L2/scripts/internal/generate_kernels.py``.

.. ATTENTION::
    Generated kernels are not self-contained source code; they reference low-level block implementation headers in the ``L1/include`` folder. Ensure that folder is passed to the AMD Vitis™ compiler as a header search path when compiling a project using generated programmable logic (PL) kernels.

Programmable 4D Data Mover
===========================

.. _programmable-features:

Feature
--------

The AI Engine (AIE) application often needs to handle multi-dimension data. The most common case is that the AIE application needs to select and read or write a regularly distributed subset from a multi-dimensional array. Because a multi-dimension array must be stored in a linear addressing memory space, such a subset is rarely a contiguous block in memory but consists of many data segments. Implementing logic to calculate all segment addresses and sizes is impractical, and doing so on the AIE is inefficient.

The Programmable 4D Data Mover includes:

- Concise descriptor design that uses 9×64 bits to fully describe access on four-dimension (and lower-dimension) data.
- Template kernel design that reads multiple descriptors and completes the defined access pattern one by one.

Descriptor Design
^^^^^^^^^^^^^^^^^

The descriptor design is the most important part of the Programmable 4D Data Mover. It defines:

- How four-dimension data is mapped to linear addresses
- Where to find the subset to access
- What the dimension of the subset is
- How to serialize the subset

To store a 4D array, ``A[W][Z][Y][X]``, in memory, it must be mapped into a linear address space, which leads to addressing such as ``&A[w][z][y][x] = bias + w * (Z*Y*X) + z * (Y*X) + y * (X) + x``. In this arrangement, adjacent elements in the 4D array have a constant stride. Nine parameters — bias (address of first element), X, X_stride, Y, Y_stride, Z, Z_stride, W, W_stride — are sufficient to define the 4D array in memory.

- &A[w][z][y][x+1] – &A[w][z][y][x] = 1             (X_stride)
- &A[w][z][y+1][x] – &A[w][z][y][x] = X             (Y_stride)
- &A[w][z+1][y][x] – &A[w][z][y][x] = X * Y         (Z_stride)
- &A[w+1][z][y][x] – &A[w][z][y][x] = X * Y * Z     (W_stride)

Because the Programmable 4D Data Mover reads from and writes to the AXI4-Stream, it also needs to define how to serialize the 4D array. Define ``ap_int<64> cfg[9]`` as the descriptor to define one access:

- cfg[0]: Bias (address of the 4D array's first element in memory)
- cfg[1], cfg[2]: Stride of the first accessed dimension, size of the first accessed dimension
- cfg[3], cfg[4]: Stride of the second accessed dimension, size of the second accessed dimension
- cfg[5], cfg[6]: Stride of the third accessed dimension, size of the third accessed dimension
- cfg[7], cfg[8]: Stride of the fourth accessed dimension, size of the fourth accessed dimension

With the descriptor above, the Programmable 4D Data Mover serializes the read/write as shown in the following pseudo-code (using read as an example). The Programmable 4D Data Mover loads one or more descriptors from a descriptor buffer. The descriptor buffer begins with a 64-bit ``num`` that indicates how many descriptors are in the buffer. Then ``num`` descriptors follow, each nine 64-bit values stored compactly. The kernel parses the first descriptor, completes the access, then parses and completes the next descriptor, continuing until all descriptors are processed.

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

Programmable 4D Data Movers are templated designs that access elements of 32, 64, 128, 256, or 512 bits width. They have a standalone AXI master port to access the descriptor buffer, configured to be 64 bits wide. Other AXI master and AXI4-Stream ports are configured to the same width as the data elements. AXI master ports share the same "latency", "outstanding", and "burst length" settings; these must match the pragma settings in the kernels that wrap the data mover. They share the same kernel generator and JSON specification. The following figure shows the kernel interface.

.. image:: /images/4d_kernl_interface.png
   :alt: various pattern
   :width: 100%
   :align: center

The Programmable 4D Data Mover performance depends on:

- Compile time: data width. Larger data width provides larger bandwidth.
- Runtime: ``cfg[1]`` of each descriptor. When ``cfg[1]`` = 1, the kernel uses burst access and achieves full bandwidth. Otherwise, it uses non-burst access and bandwidth is lower than burst access.

The following figure shows the performance characteristics of the data mover.

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

The ``4DCuboidRead`` kernel has two data paths, as shown by the two specifications in the ``map`` field. The first data path uses an AXI-M port ``din_0`` to read the 4D array and AXI-M ``desp_0`` to access the descriptor buffer. Both AXI-M ports have "latency", "outstanding", and "burst_len" set in their HLS kernel pragma. The output port is AXI4-Stream ``dout_0``, and both ``din_0`` and ``dout_0`` have a port width of 64.

The ``4DCuboidWrite`` kernel has two data paths, as shown by the two specifications in the ``map`` field. The first data path uses an AXI4-Stream port ``din_0`` to read the 4D array and AXI-M ``desp_0`` to access the descriptor buffer. It uses the AXI-M port ``dout_0`` for output. Both AXI-M ports have "latency", "outstanding", and "burst_len" set in their HLS kernel pragma. Both ``din_0`` and ``dout_0`` have a port width of 64.

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

Generating Kernels
^^^^^^^^^^^^^^^^^^

.. code-block:: bash

    cd L2/tests/datamover/4D_datamover
    make pre_build
    # The pre_build command is as follows:
    # pre_build:
    #     make -f $(CUR_DIR)/ksrc.mk GENKERNEL=$(XFLIB_DIR)/L2/scripts/generate_kernels SPEC=$(CUR_DIR)/kernel/spec.json TOOLDIR=$(CUR_DIR)/_krnlgen

Running Hardware Emulation
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   cd L2/tests/datamover/4D_datamover
   source /opt/xilinx/Vitis/2022.2/settings64.sh
   source /opt/xilinx/xrt/setup.sh
   export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
   make run TARGET=hw PLATFORM=${PLATFORM_REPO_PATHS}/xilinx_vck190_base_202210_1/xilinx_vck190_base_202210_1.xpfm

.. _programmable-runtime-config:

Runtime Configuration
-----------------------

The Programmable 4D Data Mover reads multiple descriptors from a descriptor buffer that can be configured from the host side at runtime. The descriptor buffer is stored compactly in memory and starts with a 1×64-bit ``num`` that indicates how many descriptors are in the buffer. Then ``num`` descriptors follow.

The following examples show the descriptor buffer and corresponding patterns. The underlying array is a 3D array (shown as a 3D array because 4D arrays cannot be drawn directly): ``A[10][7][8]``, which can be treated as ``A[1][10][7][8]``. Its first element's address is 0, so bias = 0. Its size means W = 1, Z = 10, Y = 7, and X = 8. Its mapping leads to W_stride = 0, Z_stride = 56, Y_stride = 8, X_stride = 1.

.. code-block:: cpp

   {4,
    0, 1, 8, 8, 7, 56, 10, 0, 1,
    0, 8, 7, 1, 8, 56, 10, 0, 1,
    0, 56, 10, 8, 7, 1, 8, 0, 1,
    4, 1, 4, 8, 3, 56, 2, 0, 1}

The first number in the buffer is ``4``, which means four descriptors follow. Their implied patterns are:

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

The following figure shows the access pattern for Descriptor[0].

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

The following figure shows the access pattern for Descriptor[1].

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

The following figure shows the access pattern for Descriptor[2].

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

The following figure shows the access pattern for Descriptor[3].

.. image:: /images/X_Y_Z_W_sub_pattern.png
   :alt: various pattern
   :width: 30%
   :align: center
