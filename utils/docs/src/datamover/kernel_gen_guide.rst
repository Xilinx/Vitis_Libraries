.. _L2_DATAMOVER_LABEL:

.. toctree::
   :caption: Table of Contents
   :maxdepth: 3

   L2.rst


================================
Generating PL Data-Mover Kernels
================================

.. _KERNEL_GEN_LABEL:

Overview
========

To provide "codeless" support for testing and validating the AIE or other AXI stream-based designs.
Users can create the OpenCL kernels by simply calling the kernel source code generator with a JSON description
and ROM content (if needed) as text file.

**Kernel Generator**

The kernel generator consists of the following parts:

- Kernel templates (in Jinja2): Can be instantiated through configurations from JSON
- Data converter: For transforming the user provided texture ROM content into usable initialization file
- Python script: For automating the kernel generation from JSON to HLS C++ OpenCL kernel

.. ATTENTION::
    Generated kernels are not self-contained source code, they would reference low-level block implementation headers in ``L1/include`` folder.
    Ensure that folder is passed to Vitis compiler as header search path when compiling project using generated PL kernels.


Kernel Templates
----------------

These kernel templates cannot be used as kernel top directly, they have to be instantiated by the Python script with the configurations coming from JSON.

Currently, there are 9 types of kernels in 2 different categories:

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

**Exampple of How to generate kernels**

.. code-block:: bash

    cd L2/tests/datamover/load_master_to_stream
    make pre_build
    # The pre_build command is as follows:
    # pre_build:
    #     make -f $(CUR_DIR)/ksrc.mk GENKERNEL=$(XFLIB_DIR)/L2/scripts/generate_kernels SPEC=$(CUR_DIR)/kernel/spec.json TOOLDIR=$(CUR_DIR)/_krnlgen



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

Full Example Projects
---------------------

* **Work Directory**
Choose `load_master_to_stream` as example, show the steps of build and run kernel

.. code-block:: bash

    cd L2/tests/datamover/load_master_to_stream

* **Setup environment**
Specifying the corresponding Vitis, XRT, and path to the platform repository by running following commands.

.. code-block:: bash

   source /opt/xilinx/Vitis/2022.1/settings64.sh
   source /opt/xilinx/xrt/setup.sh
   export PLATFORM_REPO_PATHS=/opt/xilinx/platforms

* **Build and Run Kernel**

.. code-block:: bash

    make run TARGET=hw DEVICE=${PLATFORM_REPO_PATHS}/xilinx_vck190_base_202210_1/xilinx_vck190_base_202210_1.xpfm HOST_ARCH=aarch64

.. ATTENTION::
    * Only HW_EMU and HW run available
    * Kernel-to-kernel streaming is not available in software emulation, design can only be enulated in hardware emulation.


* **Performance**
    A serial of tests for 1 channel datamover from DDR to Stream is as bellow. The frequency in this test is 300MHz, and the platform is vck190.

    +------------------------+--------------------+-------------------+
    | Input_Size (Byte)      |  Kernel_Time (ms)  |  BandWidth(GB/s)  |
    +------------------------+--------------------+-------------------+
    | 16777216 (2^24 Byte)   |       8.51274      |   1.879535849     | 
    +------------------------+--------------------+-------------------+
    | 33554432 (2^25 Byte)   |       17.1036      |   1.870951145     | 
    +------------------------+--------------------+-------------------+
    | 67108864 (2^26 Byte)   |       34.4284      |   1.858930418     | 
    +------------------------+--------------------+-------------------+
    | 134217728 (2^27 Byte)  |       70.3489      |   1.819502508     | 
    +------------------------+--------------------+-------------------+
    | 268435456 (2^28 Byte)  |       137.864      |   1.856902455     | 
    +------------------------+--------------------+-------------------+
    | 536870912 (2^29 Byte)  |       274.997      |   1.861838493     | 
    +------------------------+--------------------+-------------------+
    | 1073741824 (2^30 Byte) |       551.768      |   1.85585246      | 
    +------------------------+--------------------+-------------------+





