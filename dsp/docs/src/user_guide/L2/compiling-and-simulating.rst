..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _COMPILING_AND_SIMULATING:

************************
Compiling and Simulating
************************

**Prerequisites**:

.. code-block::

        source <your-Vitis-install-path>/lin64/HEAD/Vitis/settings64.csh
        setenv PLATFORM_REPO_PATHS <your-platform-repo-install-path>
        source <your-XRT-install-path>/xbb/xrt/packages/xrt-2.1.0-centos/opt/xilinx/xrt/setup.csh

Library Element Unit Test
--------------------------

Each library element category comes supplied with a test harness. 

For AI Engine library elements, it is located in the `L2/tests/aie/<library_element>` directory and consists of JSON, C++ files, as well as a Makefile.
For VSS library elements, it is located in the `L2/tests/vss/<library_element>` directory and contains a host file, some helper python scripts, along with the JSON, C++ files and Makefiles. 

JSON description of the test harness, defined in `L2/tests/<vss/aie>/<library_element>/description.json`, has been used to generate the Makefile. In addition, the `description.json` file defines the parameters of the test harness, e.g., a list of supported platforms.

Each Makefile uses a set of values for each of the library element parameters that are stored in in a JSON file in `L2/tests/aie/<library_element>/multi_params.json`. The set of parameters are combined in a form of a named test case, with default name being: `test_0_tool_canary_aie`. The set of parameters can be edited as required to configure the library element for your needs.

C++ files serve as an example of how to use the library element subgraph in the context of a super-graph. These test harnesses (graphs) can be found in the `L2/tests/aie/<library_element>/test.hpp` and `L2/tests/aie/<library_element>/test.cpp` file.

Although for AI Engine library elements, it is recommended that only L2 (graphs) library elements are instantiated directly in the user code, the kernels underlying the graphs can be found in the `L1/include/aie/<library_element>.hpp` and the `L1/src/aie/<library_element>.cpp` files.

For VSS library elements, it is recommended to use the top level VSS Makefile found in `L2/include/vss/<library_element>` as the entry point. 

The test harness run consists of several steps that result in a simulated and validated design. These include:

- Input files(s) generation.
- Validate configuration with metadata (in: `L2/meta`).
- Reference model compilation and simulation, to produce the `golden output`.
- Uut design compilation and simulation.
- Output post-processing (e.g., timestamps processing to produce throughput figures). The output of the reference model ( `logs/ref_output.txt` ) is verified against the output of the AI Engine graphs (`logs/uut_output.txt`).
- Status generation. On completion of the make, the `logs/status_<config_details>.txt` file will contain the result of compilation, simulation, and an indication of whether the reference model and AI Engine model outputs match. The report will also contain resource utilization and performance metrics.

Compiling Using the Makefile
----------------------------

Running Compilation
^^^^^^^^^^^^^^^^^^^

Use the following steps to compile and simulate the reference model with the x86sim target, then to compile and simulate the library element graph as described in the above section.

.. code-block::

        make cleanall run PLATFORM=vck190

.. note:: It is recommended to run a ``cleanall`` stage before the compiling design, to ensure no stale objects interfere with the compilation process.

.. note:: Platform information (e.g., PLATFORM=vck190) is a requirement of a make build process. A list of supported platforms can be found in `L2/tests/aie/<library_element>/description.json` in the "platform_allowlist" section.

Configuring the Test Case
^^^^^^^^^^^^^^^^^^^^^^^^^

To overwrite the default set of parameter, edit the `multi_params.json` file, and add a dedicated named test case or edit one of the existing ones, e.g.:

.. code-block::

    "test_my_design":{
        "DATA_TYPE": "cint32",
        "COEFF_TYPE": "int32",
        (...)
        }

To run a test case, specify the test case name passed to the PARAMS argument, e.g.:

.. code-block::

        make cleanall run PLATFORM=vck190 PARAMS=test_my_design

For list of all the configurable parameters, see the :ref:`CONFIGURATION_PARAMETERS`.

Selecting TARGET
^^^^^^^^^^^^^^^^

To perform a x86 compilation/simulation, run:

.. code-block::

    make run TARGET=x86sim.

List of all the Makefile targets:

.. code-block::

    make all TARGET=<aiesim/x86sim/hw_emu/hw> PLATFORM=<FPGA platform>
        Command to generate the design for specified Target and Shell.

    make run TARGET=<aiesim/x86sim/hw_emu/hw> PLATFORM=<FPGA platform>
        Command to run application in emulation.

    make clean
        Command to remove the generated non-hardware files.

    make cleanall
        Command to remove all the generated files.

.. note::
    For embedded platforms, the following setup steps are required:
        a. If the platform and common-image are downloaded from the Download Center (Suggested):
            | Run the `sdk.sh` script from the `common-image` directory to install sysroot using the command: ./sdk.sh -y -d ./ -p
            | Unzip the `rootfs` file : gunzip ./rootfs.ext4.gz
            | export SYSROOT=< path-to-platform-sysroot >
        b. You could also define SYSROOT, K_IMAGE, and ROOTFS by themselves:
            .. code-block::

                export SYSROOT=< path-to-platform-sysroot >
                export K_IMAGE=< path-to-Image-files >
                export ROOTFS=< path-to-rootfs >

Troubleshooting Compilation
---------------------------

Compilation Arguments
^^^^^^^^^^^^^^^^^^^^^

The test harness supplied with the library allows each library unit to be compiled and simulated in isolation. When the library unit is instanced within your design, the compilation result might differ from the result obtained with the test harness. This might be because compilation of your system might need arguments not present in your system.

Search the Makefile provided for UUT_TARGET_COMPILE_ARGS. For each library element, there can be compile arguments used to avoid errors or to improve performance, that is, specifying memories to be on separate banks to avoid wait states. These arguments will likely change with each release as the compile tool changes with each release.

Stack Size Allocation
^^^^^^^^^^^^^^^^^^^^^

Similarly, the test harness provided with each library unit estimates the stack size required for a variety of cases and creates a formula to assign sufficient amount of memory for stack purposes. When the library unit is instanced within your design, compilation can fail with insufficient stack allocated for a specific kernel. The error message should suggest a minimum figure that is required.

Use the compiler argument to allocate enough stack as advised by the compiler message. Alternatively, search the Makefile provided for STACK_SIZE, and use the formula for the library unit to calculate sufficient stack size and allocate accordingly.

Invalid Throughput and/or Latency
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Throughput and latency are only reported when a stable operation has been detected. Complex designs might take several iterations to achieve stable state. When a test case is not run for enough iterations, the status report will flag such case with throughput and latency values set to -1.

Increase the number of iterations the simulation runs for to achieve a stable state and get accurate throughput and latency measurements.

Power Analysis
--------------

For DSPLIB elements, a naming convention 'VCD' can be used to harvest dynamic power consumption. Once 'VCD' string is added within the test name, VCD file of the simulation data is captured and PDM (Power Design Manager) calculates power metrics. User can find detailed power reports in `pwr_test` folder under their corresponding test result directory. Dynamic power result can also be found in the `logs/status_<config_details>.txt` file.

.. _CONFIGURATION_PARAMETERS:

Library Element Configuration Parameters
----------------------------------------

.. _COMMON_CONFIG_PARAMETERS:

Common Configuration Parameters
-------------------------------

Many library elements perform arithmetic and offer a scaling feature exposed as TP_SHIFT. During this operation, rounding and saturation can occur, configured according to parameters TP_RND and TP_SAT. The modes and values for TP_RND are  the same for AIE-ML and AIE-MLv2 devices, but differ from those for AIE devices, as captured in the following table.

.. table:: Common Configuration Parameters

    +------------------------+----------------+----------------+--------------------------------------+
    |     **Name**           |    **Type**    |  **Default**   |   Description                        |
    +========================+================+================+======================================+
    | SHIFT                  |    unsigned    |    8           | Acc results shift down value.        |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | ROUND_MODE             |    unsigned    |    0           | Rounding mode.                       |
    |                        |                |                |                                      |
    |                        |                |                +------------------+-------------------+
    |                        |                |                |     AIE          | AIE-ML or AIE-MLv2|
    |                        |                |                +------------------+-------------------+
    |                        |                |                |                  |                   |
    |                        |                |                | 0 - rnd_floor*   | 0 - rnd_floor*    |
    |                        |                |                |                  |                   |
    |                        |                |                | 1 - rnd_ceil*    | 1 - rnd_ceil*     |
    |                        |                |                |                  |                   |
    |                        |                |                | 2 - rnd_pos_inf  | 2 - rnd_sym_floor*|
    |                        |                |                |                  |                   |
    |                        |                |                | 3 - rnd_neg_inf  | 3 - rnd_sym_ceil* |
    |                        |                |                |                  |                   |
    |                        |                |                | 4 - rnd_sym_inf  | 8 - rnd_neg_inf   |
    |                        |                |                |                  |                   |
    |                        |                |                | 5 - rnd_sym_zero | 9 - rnd_pos_inf   |
    |                        |                |                |                  |                   |
    |                        |                |                | 6 - rnd_conv_even| 10 - rnd_sym_zero |
    |                        |                |                |                  |                   |
    |                        |                |                | 7 - rnd_conv_odd | 11 - rnd_sym_inf  |
    |                        |                |                |                  |                   |
    |                        |                |                |                  | 12 - rnd_conv_even|
    |                        |                |                |                  |                   |
    |                        |                |                |                  | 13 - rnd_conv_odd |
    |                        |                |                |                  |                   |
    +------------------------+----------------+----------------+------------------+-------------------+
    | SAT_MODE               |    unsigned    |    1           | Saturation mode.                     |
    |                        |                |                |                                      |
    |                        |                |                | 0 - none                             |
    |                        |                |                |                                      |
    |                        |                |                | 1 - saturate                         |
    |                        |                |                |                                      |
    |                        |                |                | 3 - symmetric saturate               |
    +------------------------+----------------+----------------+--------------------------------------+
    | NITER                  |    unsigned    |    8           | Number of iterations to execute.     |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIFF_TOLERANCE         |    unsigned    |    0           | Tolerance value when comparing       |
    |                        |                |                | output sample with reference model,  |
    |                        |                |                | e.g. 0.0025 for floats and cfloats.  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | STIM_TYPE              |    unsigned    |    0           | Supported types:                     |
    |                        |                |                |                                      |
    |                        |                |                | 0: random                            |
    |                        |                |                |                                      |
    |                        |                |                | 3: impulse                           |
    |                        |                |                |                                      |
    |                        |                |                | 4: all ones                          |
    |                        |                |                |                                      |
    |                        |                |                | 5: incrementing pattern              |
    |                        |                |                |                                      |
    |                        |                |                | 6: sym incrementing pattern          |
    |                        |                |                |                                      |
    |                        |                |                | 8: sine wave                         |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DATA_SEED              |    unsigned    |    1           | Seed used to generate random numbers |
    |                        |                |                | for the inputs.                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NUM_OUTPUTS            |    unsigned    |    1           | Number of output ports.              |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | AIE_VARIANT            |    unsigned    |    1           | AI Engine variant to use for metadata|
    |                        |                |                | validation.                          |
    |                        |                |                | Ignored for compilation and          |
    |                        |                |                | simulation purposes.                 |
    |                        |                |                |                                      |
    |                        |                |                | 1: AIE                               |
    |                        |                |                |                                      |
    |                        |                |                | 2: AIE-ML                            |
    |                        |                |                |                                      |
    |                        |                |                | 22: AIE-MLv2                         |
    +------------------------+----------------+----------------+--------------------------------------+

.. _CONFIGURATION_PARAMETERS_BITONIC_SORT:

Bitonic Sort configuration parameters
--------------------------------------

For the Bitonic Sort library element, use the following list of configurable parameters and default values.

.. table:: Bitonic Sort configuration parameters

    +------------------------+----------------+----------------+--------------------------------------+
    |     **Name**           |    **Type**    |  **Default**   |   Description                        |
    +========================+================+================+======================================+
    | T_DATA                 |    typename    |    int32       | Data Type.                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIM_SIZE               |    unsigned    |    32          | Size of list to sort.                |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NUM_FRAMES             |    unsigned    |    1           | Number of frames in a window.        |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | ASCENDING              |    unsigned    |    1           | Indicates whether sort is ascending. |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | CASC_LEN               |    unsigned    |    1           | Sets number of tiles used to chain   |
    |                        |                |                | bitonic stages.                      |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NITER                  |    unsigned    |    4           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | STIM_TYPE              |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+

.. note:: Given parameter values are subject to checks early in compilation to ensure support. See :ref:`LEGALITY_CHECKING`

.. _CONFIGURATION_PARAMETERS_CONV_CORR:

Convolution / Correlation configuration parameters
--------------------------------------------------

For the Convolution / Correlation library element the list of configurable parameters and default values is presented below.

.. table:: Convolution / Correlation configuration parameters

    +------------------------+----------------+----------------+--------------------------------------+
    |     **Name**           |    **Type**    |  **Default**   |   Description                        |
    +========================+================+================+======================================+
    | T_DATA_F               |    typename    |    int16       | Data Type of input F.                |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | T_DATA_G               |    typename    |    int16       | Data Type of input G.                |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | T_DATA_OUT             |    typename    |    int32       | Data Type of output.                 |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | FUNCT_TYPE             |    unsigned    |    1           | Function Type.                       |
    |                        |                |                |                                      |
    |                        |                |                | 0 - Correlation                      |
    |                        |                |                |                                      |
    |                        |                |                | 1 - Convolution                      |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | COMPUTE_MODE           |    unsigned    |    0           | Mode which determines output type.   |
    |                        |                |                |                                      |
    |                        |                |                | 0 - Full                             |
    |                        |                |                |                                      |
    |                        |                |                | 1 - Valid                            |
    |                        |                |                |                                      |
    |                        |                |                | 2 - Same                             |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | F_LEN                  |    unsigned    |    1024        | Dimension size of vector F.          |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | G_LEN                  |    unsigned    |    32          | Dimension size of vector G.          |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SHIFT                  |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | API_IO                 |    unsigned    |    0           | Graph's port API.                    |
    |                        |                |                |                                      |
    |                        |                |                | 0 - window                           |
    |                        |                |                |                                      |
    |                        |                |                | 1 - stream                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NITER                  |    unsigned    |    8           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | ROUND_MODE             |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SAT_MODE               |    unsigned    |    1           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NUM_FRAMES             |    unsigned    |    1           | Number of frames in a window.        |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | CASC_LEN               |    unsigned    |    1           | cascaded length to set the number    |
    |                        |                |                | of tiles used in chain of conv/corr. |
    +------------------------+----------------+----------------+--------------------------------------+
    | PHASES                 |    unsigned    |    1           | Phases to set number of parallel     |
    |                        |                |                | cascaded chains of conv/corr.        |
    +------------------------+----------------+----------------+--------------------------------------+
    | USE_RTP_VECTOR_LENGTHS |    unsigned    |    0           | RTP parameter of F and G Lengths     |
    |                        |                |                |                                      |
    |                        |                |                | 0 - Static **G_LEN**                 |
    |                        |                |                |                                      |
    |                        |                |                | 1 - Dynamic **G_LEN**                |
    |                        |                |                |                                      |
    |                        |                |                |  **F_LEN** is static in both cases   |
    +------------------------+----------------+----------------+--------------------------------------+
    | STIM_TYPE_F            |    unsigned    |    0           | See ``STIM_TYPE`` in                 |
    |                        |                |                | :ref:`COMMON_CONFIG_PARAMETERS`      |
    +------------------------+----------------+----------------+--------------------------------------+
    | STIM_TYPE_G            |    unsigned    |    0           | See ``STIM_TYPE`` in                 |
    |                        |                |                | :ref:`COMMON_CONFIG_PARAMETERS`      |
    +------------------------+----------------+----------------+--------------------------------------+

.. _CONFIGURATION_PARAMETERS_DDS_MIXER:

DDS/Mixer Configuration Parameters
----------------------------------

For the DDS/Mixer library element, use the following list of configurable parameters and default values:

.. table:: DDS/Mixer Configuration Parameters

    +------------------------+----------------+----------------+--------------------------------------+
    |     **Name**           |    **Type**    |  **Default**   |   Description                        |
    +========================+================+================+======================================+
    | DATA_TYPE              |    typename    |    cint16      | Data Type.                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | WINDOW_VSIZE           |    unsigned    |    256         | Input/Output window size.            |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | MIXER_MODE             |    unsigned    |    2           | The mode of operation of the         |
    |                        |                |                | dds_mixer.                           |
    |                        |                |                |                                      |
    |                        |                |                | 0: dds only                          |
    |                        |                |                |                                      |
    |                        |                |                | 1: dds plus single data channel      |
    |                        |                |                | mixer                                |
    |                        |                |                |                                      |
    |                        |                |                | 2: dds plus two data channel         |
    |                        |                |                | mixer, for symmetrical carrier       |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | P_API                  |    unsigned    |    0           | 0: window,                           |
    |                        |                |                |                                      |
    |                        |                |                | 1: stream                            |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | UUT_SSR                |    unsigned    |    1           | Super Sample Rate  SSR parameter.    |
    |                        |                |                | Defaults to 1.                       |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NITER                  |    unsigned    |    16          | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIFF_TOLERANCE         |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | INITIAL_DDS_OFFSET     |    unsigned    |    0           | Initial DDS offset.                  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DDS_PHASE_INC          |    unsigned    | 0xD6555555     | DDS Phase Increment.                 |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DATA_STIM_TYPE         |    unsigned    |    0           | See ``STIM_TYPE`` in                 |
    |                        |                |                | :ref:`COMMON_CONFIG_PARAMETERS`      |
    +------------------------+----------------+----------------+--------------------------------------+
    | ROUND_MODE             |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SAT_MODE               |    unsigned    |    1           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | USE_PHASE_RELOAD       |    unsigned    | static         | 0: 'static phase'                    |
    |                        |                | phase          |                                      |
    |                        |                |                | 1: 'reload phase offset'             |
    +------------------------+----------------+----------------+--------------------------------------+

Additionally, for the DDS/Mixer library element that uses LUTs, an additional template parameter is available:

+------+-----------+---------+---------------------------------------------------------------------------------------------+
| Name |    Type   | Default |                                         Description                                         |
+======+===========+=========+=============================================================================================+
| SFDR | unsigned  | 90      | specifies the expected Spurious Free Dynamic Range that the useR expects from the generated |
|      |           |         | design.                                                                                     |
+------+-----------+---------+---------------------------------------------------------------------------------------------+

.. _CONFIGURATION_PARAMETERS_DFT:

DFT Configuration Parameters
-------------------------------

For the DFT library element, use the following list of configurable parameters and default values.

.. table:: DFT Configuration Parameters

    +------------------------+----------------+----------------+--------------------------------------+
    |     **Name**           |    **Type**    |  **Default**   |   Description                        |
    +========================+================+================+======================================+
    | DATA_TYPE              |    typename    |    cint16      | Data Type.                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | TWIDDLE_TYPE           |    typename    |    cint16      | Twiddle Type.                        |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | POINT_SIZE             |    unsigned    |    16          | DFT point size.                      |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SHIFT                  |    unsigned    |    8           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | FFT_NIFFT              |    unsigned    |    0           | Forward (1) or reverse (0) transform.|
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NUM_FRAMES             |    unsigned    |    8           | The number of batches of input data  |
    |                        |                |                | that will be processed per iteration.|
    +------------------------+----------------+----------------+--------------------------------------+
    | CASC_LEN               |    unsigned    |    1           | Cascade length.                      |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | API_IO                 |    unsigned    |    0           | Graph's port API.                    |
    |                        |                |                |                                      |
    |                        |                |                | 0: window                            |
    |                        |                |                |                                      |
    |                        |                |                | 1: stream                            |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | UUT_SSR                |    unsigned    |    1           | Super Sample Rate  SSR parameter.    |
    |                        |                |                | Defaults to 1.                       |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NITER                  |    unsigned    |    8           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIFF_TOLERANCE         |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | ROUND_MODE             |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SAT_MODE               |    unsigned    |    1           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | STIM_TYPE              |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+

.. note:: Given parameter values are subject to checks early in compilation to ensure support. See :ref:`LEGALITY_CHECKING`

.. _CONFIGURATION_PARAMETERS_EUCLIDEAN_DISTANCE:

Euclidean Distance Configuration Parameters
-------------------------------------------

For the Euclidean Distance library element, use the following list of configurable parameters and default values.

.. table:: Euclidean Distance Configuration Parameters

    +------------------------+----------------+----------------+--------------------------------------+
    |     **Name**           |    **Type**    |  **Default**   |   Description                        |
    +========================+================+================+======================================+
    | DATA                   |    typename    |    float       | Data Type.                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DATA_OUT               |    typename    |    float       | Output Data Type.                    |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | LEN                    |    unsigned    |    32          | Number of samples in input buffers.  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIM                    |    unsigned    |   3            | Number of dimensions in input        |
    |                        |                |                | samples.                             |
    +------------------------+----------------+----------------+--------------------------------------+
    | API_IO                 |    unsigned    |    0           | Graph's port API.                    |
    |                        |                |                |                                      |
    |                        |                |                | 0: window                            |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | RND                    |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SAT                    |    unsigned    |    1           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | IS_OUTPUT_SQUARED      |    unsigned    |    0           | Compute output as:                   |
    |                        |                |                |                                      |
    |                        |                |                | 0: SQUARE_ROOT                       |
    |                        |                |                |                                      |
    |                        |                |                | 1: SQUARED                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+

.. note:: Given parameter values are subject to checks early in compilation to ensure support. See :ref:`LEGALITY_CHECKING`

.. _CONFIGURATION_PARAMETERS_FFT:

FFT Configuration Parameters
-------------------------------

For the FFT/iFFT library element, use the following list of configurable parameters and default values.

.. table:: FFT Configuration Parameters

    +------------------------+----------------+----------------+--------------------------------------+
    |     **Name**           |    **Type**    |  **Default**   |   Description                        |
    +========================+================+================+======================================+
    | DATA_TYPE              |    typename    |    cint16      | Data Type.                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | TWIDDLE_TYPE           |    typename    |    cint16      | Twiddle Type.                        |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | POINT_SIZE             |    unsigned    |    1024        | FFT point size.                      |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SHIFT                  |    unsigned    |    17          | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | FFT_NIFFT              |    unsigned    |    0           | Forward (1) or reverse (0) transform.|
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | WINDOW_VSIZE           |    unsigned    |    1024        | Input/Output window size.            |
    |                        |                |                |                                      |
    |                        |                |                | By default, set to: $(POINT_SIZE).   |
    +------------------------+----------------+----------------+--------------------------------------+
    | CASC_LEN               |    unsigned    |    1           | Cascade length.                      |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DYN_PT_SIZE            |    unsigned    |    0           | Enable (1) Dynamic Point size        |
    |                        |                |                | feature.                             |
    +------------------------+----------------+----------------+--------------------------------------+
    | API_IO                 |    unsigned    |    0           | Graph's port API.                    |
    |                        |                |                |                                      |
    |                        |                |                | 0: window                            |
    |                        |                |                |                                      |
    |                        |                |                | 1: stream                            |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | PARALLEL_POWER         |    unsigned    |   0            | Parallelism, controlling             |
    |                        |                |                | Super Sample Rate operation.         |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NITER                  |    unsigned    |    4           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIFF_TOLERANCE         |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | STIM_TYPE              |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | ROUND_MODE             |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SAT_MODE               |    unsigned    |    1           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+

.. note:: Given parameter values are subject to checks early in compilation to ensure support. See :ref:`LEGALITY_CHECKING`

.. _CONFIGURATION_PARAMETERS_FFT_WINDOW:

FFT Window Configuration Parameters
--------------------------------------

For the FFT Window library element, use the following list of configurable parameters and default values.

.. table:: FFT Window Configuration Parameters

    +------------------------+----------------+----------------+--------------------------------------+
    |     **Name**           |    **Type**    |  **Default**   |   Description                        |
    +========================+================+================+======================================+
    | DATA_TYPE              |    typename    |    cint16      | Data Type.                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | COEFF_TYPE             |    typename    |    cint16      | Coeff Type.                          |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | POINT_SIZE             |    unsigned    |    1024        | FFT point size.                      |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SHIFT                  |    unsigned    |    17          | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | WINDOW_VSIZE           |    unsigned    |    1024        | Input/Output window size.            |
    |                        |                |                |                                      |
    |                        |                |                | By default, set to: $(POINT_SIZE).   |
    +------------------------+----------------+----------------+--------------------------------------+
    | DYN_PT_SIZE            |    unsigned    |    0           | Enable (1) Dynamic Point size        |
    |                        |                |                | feature.                             |
    +------------------------+----------------+----------------+--------------------------------------+
    | API_IO                 |    unsigned    |    0           | Graph's port API.                    |
    |                        |                |                |                                      |
    |                        |                |                | 0: window                            |
    |                        |                |                |                                      |
    |                        |                |                | 1: stream                            |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | WINDOW_CHOICE          |    unsigned    |    0           | Supported types:                     |
    |                        |                |                |                                      |
    |                        |                |                | 0: Hamming                           |
    |                        |                |                |                                      |
    |                        |                |                | 1: Hann                              |
    |                        |                |                |                                      |
    |                        |                |                | 2: Blackman                          |
    |                        |                |                |                                      |
    |                        |                |                | 3: Kaiser                            |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NITER                  |    unsigned    |    4           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIFF_TOLERANCE         |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | ROUND_MODE             |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SAT_MODE               |    unsigned    |    1           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | STIM_TYPE              |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+

.. note:: Given parameter values are subject to checks early in compilation to ensure support. See :ref:`LEGALITY_CHECKING`

.. _CONFIGURATION_PARAMETERS_FILTERS:

FIR Configuration Parameters
-------------------------------

The following list consists of configurable parameters for FIR library elements with their default values.

.. table:: FIR Configuration Parameters

    +------------------------+----------------+----------------+--------------------------------------+
    |     **Name**           |    **Type**    |  **Default**   |   Description                        |
    +========================+================+================+======================================+
    | DATA_TYPE              |    typename    |    cint16      | Data Type.                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | COEFF_TYPE             |    typename    |    int16       | Coefficient Type.                    |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | FIR_LEN                |    unsigned    |    81          | FIR length.                          |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SHIFT                  |    unsigned    |    16          | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | INPUT_WINDOW_VSIZE     |    unsigned    |    512         | Input window size.                   |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | CASC_LEN               |    unsigned    |    1           | Cascade length.                      |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | INTERPOLATE_FACTOR     |    unsigned    |    1           | Interpolation factor,                |
    |                        |                |                | see note below.                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DECIMATE_FACTOR        |    unsigned    |    1           | Decimation factor,                   |
    |                        |                |                | see note below.                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | TDM_CHANNELS           |    unsigned    |    1           | Number of TDM Channels.              |
    |                        |                |                | Only used by TDM FIR,                |
    |                        |                |                | see note below.                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DUAL_IP                |    unsigned    |    0           | Dual inputs used in FIRs,            |
    |                        |                |                | see note below.                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NUM_OUTPUTS            |    unsigned    |    1           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | USE_COEFF_RELOAD       |    unsigned    |    0           | Use two sets of reloadable           |
    |                        |                |                | coefficients, where the second set   |
    |                        |                |                | deliberately corrupts a single,      |
    |                        |                |                | randomly selected coefficient.       |
    +------------------------+----------------+----------------+--------------------------------------+
    | PORT_API               |    unsigned    |    0           | Graph's port API.                    |
    |                        |                |                |                                      |
    |                        |                |                | 0: window                            |
    |                        |                |                |                                      |
    |                        |                |                | 1: stream                            |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | UUT_SSR                |    unsigned    |    1           | Super Sample Rate  SSR parameter.    |
    |                        |                |                | Defaults to 1.                       |
    |                        |                |                | see note below                       |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NITER                  |    unsigned    |    16          | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIFF_TOLERANCE         |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DATA_STIM_TYPE         |    unsigned    |    0           | See ``STIM_TYPE`` in                 |
    |                        |                |                | :ref:`COMMON_CONFIG_PARAMETERS`      |
    +------------------------+----------------+----------------+--------------------------------------+
    | COEFF_STIM_TYPE        |    unsigned    |    0           | See ``STIM_TYPE`` in                 |
    |                        |                |                | :ref:`COMMON_CONFIG_PARAMETERS`      |
    +------------------------+----------------+----------------+--------------------------------------+
    | USE_CUSTOM_CONSTRAINT  |    unsigned    |    0           | Overwrite default or non-existent.   |
    |                        |                |                |                                      |
    |                        |                |                | 0: no action                         |
    |                        |                |                |                                      |
    |                        |                |                | 1: use the Graph's access functions  |
    |                        |                |                | to set a location and                |
    |                        |                |                | overwrite a fifo_depth constraint.   |
    |                        |                |                | see also :ref:`FIR_CONSTRAINTS`      |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | ROUND_MODE             |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SAT_MODE               |    unsigned    |    1           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+

.. note:: Given parameter values are subject to checks early in compilation to ensure support. See :ref:`LEGALITY_CHECKING`

.. note:: Not all dsplib elements support all of the above configurable parameters. Unsupported parameters which are not used have no impact on execution, e.g., the `INTERPOLATE_FACTOR` parameter is only supported by interpolation filters and will be ignored by other library elements.

.. _CONFIGURATION_PARAMETERS_FUNC_APPROX:

Function Approximation configuration parameters
-----------------------------------------------

For the Function Approximation library element, use the list of configurable parameters and default values is presented below.

.. table:: Function Approximation configuration parameters

    +------------------------+----------------+----------------+--------------------------------------+
    |     **Name**           |    **Type**    |  **Default**   |   **Description**                    |
    +========================+================+================+======================================+
    | DATA_TYPE              | typename       | cint16         | Data type.                           |
    +------------------------+----------------+----------------+--------------------------------------+
    | COARSE_BITS            | unsigned       | 8              | Number of bits used for coarse       |
    |                        |                |                | lookup. Determines the number of     |
    |                        |                |                | linear sections in the lookup table. |
    +------------------------+----------------+----------------+--------------------------------------+
    | FINE_BITS              | unsigned       | 7              | Number of bits used for              |
    |                        |                |                | interpolation between lookup         |
    |                        |                |                | sections.                            |
    +------------------------+----------------+----------------+--------------------------------------+
    | DOMAIN_MODE            | unsigned       | 0              | Describes normalized input, x,       |
    |                        |                |                | domain.                              |
    +------------------------+----------------+----------------+--------------------------------------+
    | FUNC_CHOICE            | unsigned       | 0              | Sets which utility function is used  |
    |                        |                |                | to create lookup table values. Each  |
    |                        |                |                | utility function provides            |
    |                        |                |                | approximations for a specific        |
    |                        |                |                | function. The following values and   |
    |                        |                |                | functions are enumerated for         |
    |                        |                |                | FUNC_CHOICE:                         |
    |                        |                |                | 0 - SQRT_FUNC                        |
    |                        |                |                | 1 - INVSQRT_FUNC                     |
    |                        |                |                | 2 - LOG_FUNC                         |
    |                        |                |                | 3 - EXP_FUNC                         |
    |                        |                |                | 4 - INV_FUNC                         |
    +------------------------+----------------+----------------+--------------------------------------+
    | SHIFT                  | unsigned       | 0              | See :ref:`COMMON_CONFIG_PARAMETERS`. |
    +------------------------+----------------+----------------+--------------------------------------+
    | WINDOW_VSIZE           | unsigned       | 6              | See :ref:`COMMON_CONFIG_PARAMETERS`. |
    +------------------------+----------------+----------------+--------------------------------------+
    | ROUND_MODE             | unsigned       | 0              | See :ref:`COMMON_CONFIG_PARAMETERS`. |
    +------------------------+----------------+----------------+--------------------------------------+
    | SAT_MODE               | unsigned       | 1              | See :ref:`COMMON_CONFIG_PARAMETERS`. |
    +------------------------+----------------+----------------+--------------------------------------+
    | NITER                  | unsigned       | 4              | See :ref:`COMMON_CONFIG_PARAMETERS`. |
    +------------------------+----------------+----------------+--------------------------------------+
    | STIM_TYPE              | unsigned       | 0              | This parameter drives the input data |
    |                        |                |                | stimulus differently from other      |
    |                        |                |                | library elements, as a different     |
    |                        |                |                | input generation script is used.     |
    |                        |                |                | STIM_TYPE = 0 provides random data   |
    |                        |                |                | in the selected DOMAIN_MODE.         |
    |                        |                |                | STIM_TYPE = 1 provides incrementing  |
    |                        |                |                | data across the entire domain for    |
    |                        |                |                | the selected DOMAIN_MODE.            |
    +------------------------+----------------+----------------+--------------------------------------+

.. _CONFIGURATION_PARAMETERS_HADAMARD:

Hadamard Product configuration parameters
-----------------------------------------

For the Hadamard Product library element, use the list of configurable parameters and default values is presented below.

.. table:: Hadamard Product configuration parameters

    +------------------------+----------------+----------------+--------------------------------------+
    |     **Name**           |    **Type**    |  **Default**   |   Description                        |
    +========================+================+================+======================================+
    | DATA_A                 |    typename    |    cint16      | Data Type.                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DATA_B                 |    typename    |    cint16      | Data Type.                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIM                    |    unsigned    |    256         | Number of samples in the             |
    |                        |                |                | vectors A and B.                     |
    +------------------------+----------------+----------------+--------------------------------------+
    | NUM_FRAMES             |    unsigned    |    1           | Number of vectors to be processed.   |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SHIFT                  |    unsigned    |    6           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | API_IO                 |    unsigned    |    0           | Graph's port API.                    |
    |                        |                |                |                                      |
    |                        |                |                | 0 - window                           |
    |                        |                |                |                                      |
    |                        |                |                | 1 - stream                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | ROUND_MODE             |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SAT_MODE               |    unsigned    |    1           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NITER                  |    unsigned    |    4           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIFF_TOLERANCE         |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | STIM_TYPE              |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+

.. note:: Given parameter values are subject to checks early in compilation to ensure support. See :ref:`LEGALITY_CHECKING`

.. _CONFIGURATION_PARAMETERS_KRONECKER:

Kronecker configuration parameters
--------------------------------------

For the Kronecker library element the list of configurable parameters and default values is presented below.

.. table:: Kronecker configuration parameters

    +------------------------+----------------+----------------+--------------------------------------+
    |     **Name**           |    **Type**    |  **Default**   |   Description                        |
    +========================+================+================+======================================+
    | T_DATA_A               |    typename    |    int32       | Data type of input matrix A.         |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | T_DATA_B               |    typename    |    int32       | Data type of input matrix B.         |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIM_A_ROWS             |    unsigned    |    16          | Number of rows of input Matrix A.    |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIM_A_COLS             |    unsigned    |    8           | Number of columns of input Matrix A. |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIM_B_ROWS             |    unsigned    |    16          | Number of rows of input Matrix B.    |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIM_B_COLS             |    unsigned    |    8           | Number of columns of input Matrix B. |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NUM_FRAMES             |    unsigned    |    1           | Number of frames in a window.        |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | API_IO                 |    unsigned    |    1           | Graph's port API.                    |
    |                        |                |                |                                      |
    |                        |                |                | 0 - in window / out window           |
    |                        |                |                |                                      |
    |                        |                |                | 1 - in window / out stream           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SHIFT                  |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | UUT_SSR                |    unsigned    |    1           | Super Sample Rate.                   |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | ROUND_MODE             |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SAT_MODE               |    unsigned    |    1           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NITER                  |    unsigned    |    4           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | STIM_TYPE              |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+

.. note:: Given parameter values are subject to checks early in compilation to ensure support. See :ref:`LEGALITY_CHECKING`

.. _CONFIGURATION_PARAMETERS_GEMM:

Matrix Multiply Configuration Parameters
-------------------------------------------

For the Matrix Multiply (GeMM) library element, use the following list of configurable parameters and default values.

.. table:: Matrix Multiply Configuration Parameters

    +------------------------+----------------+----------------+--------------------------------------+
    |     **Name**           |    **Type**    |  **Default**   |   Description                        |
    +========================+================+================+======================================+
    | T_DATA_A               |    typename    |    cint16      | Input A Data Type.                   |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | T_DATA_B               |    typename    |    cint16      | Input B Data Type.                   |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DATA_OUT_TYPE          |    typename    |    cint16      | Output Data Type.                    |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | P_DIM_A                |    unsigned    |    16          | Input A Dimension.                   |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | P_DIM_AB               |    unsigned    |    16          | Input AB Common Dimension.           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | P_DIM_B                |    unsigned    |    16          | Input B Dimension.                   |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SHIFT                  |    unsigned    |    20          | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | ROUND_MODE             |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | P_CASC_LEN             |    unsigned    |    1           | Cascade length.                      |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | UUT_SSR                |    unsigned    |    1           |  Super Sample Rate.                  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | P_DIM_A_LEADING        |    unsigned    |    0           | ROW_MAJOR = 0                        |
    |                        |                |                |                                      |
    |                        |                |                | COL_MAJOR = 1                        |
    +------------------------+----------------+----------------+--------------------------------------+
    | P_DIM_B_LEADING        |    unsigned    |    1           | ROW_MAJOR = 0                        |
    |                        |                |                |                                      |
    |                        |                |                | COL_MAJOR = 1                        |
    +------------------------+----------------+----------------+--------------------------------------+
    | P_DIM_OUT_LEADING      |    unsigned    |    0           | ROW_MAJOR = 0                        |
    |                        |                |                |                                      |
    |                        |                |                | COL_MAJOR = 1                        |
    +------------------------+----------------+----------------+--------------------------------------+
    | P_ADD_TILING_A         |    unsigned    |    1           | no additional tiling kernel = 0      |
    |                        |                |                |                                      |
    |                        |                |                | add additional tiling kernel = 1     |
    +------------------------+----------------+----------------+--------------------------------------+
    | P_ADD_TILING_B         |    unsigned    |    1           | no additional tiling kernel = 0      |
    |                        |                |                |                                      |
    |                        |                |                | add additional tiling kernel = 1     |
    +------------------------+----------------+----------------+--------------------------------------+
    | P_ADD_DETILING_OUT     |    unsigned    |    1           | no additional detiling kernel = 0    |
    |                        |                |                |                                      |
    |                        |                |                | add additional detiling kernel = 1   |
    +------------------------+----------------+----------------+--------------------------------------+
    | NITER                  |    unsigned    |    16          | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIFF_TOLERANCE         |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | STIM_TYPE_A            |    unsigned    |    0           | See ``STIM_TYPE`` in                 |
    |                        |                |                | :ref:`COMMON_CONFIG_PARAMETERS`      |
    +------------------------+----------------+----------------+--------------------------------------+
    | STIM_TYPE_B            |    unsigned    |    0           | See ``STIM_TYPE`` in                 |
    |                        |                |                | :ref:`COMMON_CONFIG_PARAMETERS`      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SAT_MODE               |    unsigned    |    1           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+

.. note:: Given parameter values are subject to checks early in compilation to ensure support. See :ref:`LEGALITY_CHECKING`

.. _CONFIGURATION_PARAMETERS_GEMV:

Matrix Vector Multiply Configuration Parameters
-----------------------------------------------

For the Matrix Vector Multiply (GeMV) library element, use the following list of configurable parameters and default values.

.. table:: Matrix Vector Multiply Configuration Parameters

    +------------------------+----------------+----------------+--------------------------------------+
    |     **Name**           |    **Type**    |  **Default**   |   Description                        |
    +========================+================+================+======================================+
    | DATA_A                 |    typename    |    cint16      | Input Matrix A Data Type.            |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DATA_B                 |    typename    |    cint16      | Input Vector B Data Type.            |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIM_A                  |    unsigned    |    16          | Input Matrix A Dimension             |
    |                        |                |                | (number of matrix rows).             |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIM_B                  |    unsigned    |    16          | Input Vector B Dimension             |
    |                        |                |                | (number of matrix columns).          |
    +------------------------+----------------+----------------+--------------------------------------+
    | SHIFT                  |    unsigned    |    16          | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | ROUND_MODE             |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | UUT_SSR                |    unsigned    |    1           |  Super Sample Rate.                  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | CASC_LEN               |    unsigned    |    1           | Cascade length.                      |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NUM_FRAMES             |    unsigned    |    1           | The number of batches of input data  |
    |                        |                |                | that will be processed per iteration.|
    +------------------------+----------------+----------------+--------------------------------------+
    | DIM_A_LEADING          |    unsigned    |    1           | COL_MAJOR = 1                        |
    |                        |                |                | ROW MAJOR = 0                        |
    +------------------------+----------------+----------------+--------------------------------------+
    | USE_MATRIX_RELOAD      |    unsigned    |    0           | 0 - Matrix A via iobuffer            |
    |                        |                |                | 1 - reloadable Matrix A via RTP ports|
    +------------------------+----------------+----------------+--------------------------------------+
    | API_IO                 |    unsigned    |    0           | 0 - Vector B via iobuffer            |
    |                        |                |                | 1 - Vector B via stream, only when   |
    |                        |                |                | reloadable Matrix A via RTP port     |
    +------------------------+----------------+----------------+--------------------------------------+
    | DUAL_IP                |    unsigned    |    0           | 0 - single vector input per kernel   |
    |                        |                |                | 1 - dual vector inputs per kernel    |
    +------------------------+----------------+----------------+--------------------------------------+
    | NUM_OUTPUTS            |    unsigned    |    1           | 1 - single output per SSR rank       |
    |                        |                |                | 2 - dual outputs per SSR rank,       |
    |                        |                |                | stream only                          |
    +------------------------+----------------+----------------+--------------------------------------+
    | NITER                  |    unsigned    |    16          | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIFF_TOLERANCE         |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | STIM_TYPE_A            |    unsigned    |    0           | See ``STIM_TYPE`` in                 |
    |                        |                |                | :ref:`COMMON_CONFIG_PARAMETERS`      |
    +------------------------+----------------+----------------+--------------------------------------+
    | STIM_TYPE_B            |    unsigned    |    0           | See ``STIM_TYPE`` in                 |
    |                        |                |                | :ref:`COMMON_CONFIG_PARAMETERS`      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SAT_MODE               |    unsigned    |    1           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+

.. note:: Given parameter values are subject to checks early in compilation to ensure support. See :ref:`LEGALITY_CHECKING`

.. _CONFIGURATION_PARAMETERS_MRFFT:

Mixed Radix FFT Configuration Parameters
----------------------------------------

For the Mixed Radix library element, use the following list of configurable parameters and default values.

.. table:: FFT Configuration Parameters

    +------------------------+----------------+----------------+--------------------------------------+
    |     **Name**           |    **Type**    |  **Default**   |   Description                        |
    +========================+================+================+======================================+
    | DATA_TYPE              |    typename    |    cint16      | Data Type.                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | TWIDDLE_TYPE           |    typename    |    cint16      | Twiddle Type.                        |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | POINT_SIZE             |    unsigned    |    48          | FFT point size.                      |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SHIFT                  |    unsigned    |    6           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | FFT_NIFFT              |    unsigned    |    0           | Forward (1) or reverse (0) transform.|
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | WINDOW_VSIZE           |    unsigned    |    48          | Input/Output window size.            |
    |                        |                |                |                                      |
    |                        |                |                | By default, set to: $(POINT_SIZE).   |
    +------------------------+----------------+----------------+--------------------------------------+
    | CASC_LEN               |    unsigned    |    1           | Cascade length.                      |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | API_IO                 |    unsigned    |    0           | Graph's port API.                    |
    |                        |                |                |                                      |
    |                        |                |                | 0: window                            |
    |                        |                |                |                                      |
    |                        |                |                | 1: stream                            |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NITER                  |    unsigned    |    4           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIFF_TOLERANCE         |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | STIM_TYPE              |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | ROUND_MODE             |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SAT_MODE               |    unsigned    |    1           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+

.. note:: Given parameter values are subject to checks early in compilation to ensure support. See :ref:`LEGALITY_CHECKING`


.. _CONFIGURATION_PARAMETERS_OUTER_TENSOR:

Outer Tensor configuration parameters
--------------------------------------

For the Outer Tensor library element, use the following list of configurable parameters and default values.

.. table:: Outer Tensor configuration parameters

    +------------------------+----------------+----------------+--------------------------------------+
    |     **Name**           |    **Type**    |  **Default**   |   Description                        |
    +========================+================+================+======================================+
    | T_DATA_A               |    typename    |    int32       | Data Type.                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | T_DATA_B               |    typename    |    int32       | Data Type.                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIM_SIZE_A             |    unsigned    |    16          | Dimension size of vector A           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIM_SIZE_B             |    unsigned    |    32          | Dimension size of vector B           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NUM_FRAMES             |    unsigned    |    1           | Number of frames in a window.        |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SHIFT                  |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | API_IO                 |    unsigned    |    0           | Graph's port API.                    |
    |                        |                |                |                                      |
    |                        |                |                | 0 - window                           |
    |                        |                |                |                                      |
    |                        |                |                | 1 - stream                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | UUT_SSR                |    unsigned    |    1           | Super Sample Rate  SSR parameter.    |
    |                        |                |                | Defaults to 1.                       |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NITER                  |    unsigned    |    4           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | ROUND_MODE             |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SAT_MODE               |    unsigned    |    1           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | STIM_TYPE              |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+

.. note:: Given parameter values are subject to checks early in compilation to ensure support. See :ref:`LEGALITY_CHECKING`

.. _CONFIGURATION_PARAMETERS_SAMPLE_DELAY:

Sample Delay Configuration Parameters
-------------------------------------

For the Sample Delay library elements, use the following list of configurable parameters and default values.

.. table:: Sample Delay Configuration Parameters

    +------------------------+----------------+----------------+--------------------------------------+
    |     **Name**           |    **Type**    |  **Default**   |   Description                        |
    +========================+================+================+======================================+
    | DATA_TYPE              |    typename    |    cint16      | Data Type.                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | WINDOW_VSIZE           |    unsigned    |    256         | Input/Output window size.            |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | PORT_API               |    unsigned    |    0           | 0 = window,                          |
    |                        |                |                |                                      |
    |                        |                |                | 1 = stream                           |
    +------------------------+----------------+----------------+--------------------------------------+
    | NITER                  |    unsigned    |    16          | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DELAY_INI_VALUE        |    unsigned    |    10          | The delay to the input data.         |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | MAX_DELAY              |    unsigned    |   256          | The maximum threshold on the delay.  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DATA_STIM_TYPE         |    unsigned    |    0           | See ``STIM_TYPE`` in                 |
    |                        |                |                | :ref:`COMMON_CONFIG_PARAMETERS`      |
    +------------------------+----------------+----------------+--------------------------------------+


.. _CONFIGURATION_PARAMETERS_WIDGETS:

Widgets Configuration Parameters
-----------------------------------

For the Widgets library elements, use the following list of configurable parameters and default values.

.. table:: Widget API Casts Configuration Parameters

    +------------------------+----------------+----------------+--------------------------------------+
    |     **Name**           |    **Type**    |  **Default**   |   Description                        |
    +========================+================+================+======================================+
    | DATA_TYPE              |    typename    |    cint16      | Data Type.                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | IN_API                 |    unsigned    |    0           | 0: window                            |
    |                        |                |                |                                      |
    |                        |                |                | 1: stream                            |
    +------------------------+----------------+----------------+--------------------------------------+
    | OUT_API                |    unsigned    |    0           | 0: window,                           |
    |                        |                |                |                                      |
    |                        |                |                | 1: stream                            |
    +------------------------+----------------+----------------+--------------------------------------+
    | NUM_INPUTS             |    unsigned    |    1           | The number of input stream           |
    |                        |                |                | interfaces.                          |
    +------------------------+----------------+----------------+--------------------------------------+
    | WINDOW_VSIZE           |    unsigned    |    256         | Input/Output window size.            |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NUM_OUTPUT_CLONES      |    unsigned    |    1           | The number of output window          |
    |                        |                |                | port copies                          |
    +------------------------+----------------+----------------+--------------------------------------+
    | PATTERN                |    unsigned    |    0           | The pattern of interleave            |
    |                        |                |                | by which samples from each           |
    |                        |                |                | of two streams are arranged          |
    |                        |                |                | into the destination window,         |
    |                        |                |                | or from the input window             |
    |                        |                |                | to dual output streams.              |
    +------------------------+----------------+----------------+--------------------------------------+
    | NITER                  |    unsigned    |    16          | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIFF_TOLERANCE         |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DATA_STIM_TYPE         |    unsigned    |    0           | See ``STIM_TYPE`` in                 |
    |                        |                |                | :ref:`COMMON_CONFIG_PARAMETERS`      |
    +------------------------+----------------+----------------+--------------------------------------+


.. table:: Widget Real to Complex Configuration Parameters

    +------------------------+----------------+----------------+--------------------------------------+
    |     **Name**           |    **Type**    |  **Default**   |   Description                        |
    +========================+================+================+======================================+
    | DATA_TYPE              |    typename    |    cint16      | Data Type.                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DATA_OUT_TYPE          |    typename    |    cint16      | Data Type.                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | WINDOW_VSIZE           |    unsigned    |    256         | Input/Output window size.            |
    +------------------------+----------------+----------------+--------------------------------------+
    | NITER                  |    unsigned    |    16          | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIFF_TOLERANCE         |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DATA_STIM_TYPE         |    unsigned    |    0           | See ``STIM_TYPE`` in                 |
    |                        |                |                | :ref:`COMMON_CONFIG_PARAMETERS`      |
    +------------------------+----------------+----------------+--------------------------------------+

.. note:: Given parameter values are subject to checks early in compilation to ensure support. See :ref:`LEGALITY_CHECKING`


.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
