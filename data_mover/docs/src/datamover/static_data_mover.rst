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

Static Data Mover
=================

.. _static-features:

Feature
-------

The Static Data Mover has nine types of kernels in two categories. All kernels access a fixed amount of data in memory, URAM, or block RAM in a contiguous pattern.

The following kernels move data to AIE:

- ``LoadDdrToStream``: Loads data from the programmable logic (PL) double-data rate (DDR) memory to AIE through an AXI stream.
- ``LoadDdrToStreamWithCounter``: Loads data from PL DDR to AIE through an AXI stream and records the data count sent to AIE.
- ``SendRomToStream``: Sends data from on-chip block RAM to AIE through an AXI stream.
- ``SendRamToStream``: Sends data from on-chip URAM to AIE through an AXI stream (same as ``SendRomToStream``, but uses URAM as the data source instead of block RAM).

The following kernels move data from AIE:

- ``StoreStreamToMaster``: Receives data from AIE through an AXI stream and saves it to PL DDR.
- ``StoreStreamToMasterWithCounter``: Receives data from AIE through an AXI stream, saves it to PL DDR, and records the data count sent to DDR.
- ``ValidateStreamWithMaster``: Receives data from AIE through an AXI stream, compares it with reference data in PL DDR, and writes the overall pass/fail flag to PL DDR.
- ``ValidateStreamWithRom``: Receives data from AIE through an AXI stream, compares it with reference data in PL block RAM (BRAM), and writes the overall pass/fail flag to PL DDR.
- ``ValidateStreamWithRam``: Receives data from AIE through an AXI stream, compares it with reference data in PL URAM, and writes the overall pass/fail flag to PL DDR.

.. _static-build-config:

Build Time Configuration
-------------------------

Example Kernel Specification (JSON)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The following kernel specification in JSON describes a ``SendRomToStream`` kernel and a ``StoreStreamToMaster`` kernel.

The ``SendRomToStream`` kernel has two data paths, as shown by the two specifications in the ``map`` field. The first data path uses texture contents for ROM initialization from a file named ``din0``, and the corresponding datatype is specified as ``int64_t``. With ``num`` set to 512, the depth of the internal ROM is 512. To send data to the AXI stream at II = 1, the on-chip ROM width is automatically generated to match the output port width: 64 bits. The second data path is auto-generated using the same rules.

The ``StoreStreamToMaster`` kernel also has two data paths. To load data from AIE through the AXI stream at II = 1, the output port width for PL DDR is automatically generated to match the width of the corresponding AXI stream: 64 bits for the first data path and 32 bits for the second data path.

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

Refer to ``L2/tests/datamover`` for the JSON format of all nine types of kernels that can be generated.

Generating Kernels
^^^^^^^^^^^^^^^^^^

.. code-block:: bash

    cd L2/tests/datamover/load_master_to_stream
    make pre_build
    # The pre_build command is as follows:
    # pre_build:
    #     make -f $(CUR_DIR)/ksrc.mk GENKERNEL=$(XFLIB_DIR)/L2/scripts/generate_kernels SPEC=$(CUR_DIR)/kernel/spec.json TOOLDIR=$(CUR_DIR)/_krnlgen

Running Hardware Emulation
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   cd L2/tests/datamover/load_master_to_stream
   source /opt/xilinx/Vitis/2022.2/settings64.sh
   source /opt/xilinx/xrt/setup.sh
   export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
   make run TARGET=hw PLATFORM=${PLATFORM_REPO_PATHS}/xilinx_vck190_base_202210_1/xilinx_vck190_base_202210_1.xpfm

.. ATTENTION::
   * Only HW_EMU and HW run are available.
   * Kernel-to-kernel streaming can only be emulated in hardware emulation.

Data Converter
--------------

This C-based data converter transforms texture ROM contents to a hexadecimal string with a specific width that can be directly programmed into on-chip ROMs.

The following limitations apply when providing JSON specifications or ROM contents:

- Maximum output data width is 512 bits.
- Output data width must be wider than input.
- Input texture contents must be provided line by line, separated by ``\n``.
- Only the following input data types are supported.

Supported Input Datatypes
^^^^^^^^^^^^^^^^^^^^^^^^^

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
