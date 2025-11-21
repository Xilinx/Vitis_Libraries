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

Dependencies
------------

The solver library utilizes several compilation, metadata checks, and post-compilation scripts from the Vitis AIE DSP library. Ensure that you have set up the DSP library environment in the same directory as the solver library. For more information on setting up the AIE DSP library environment, refer to the `Vitis DSP Library User Guide <https://docs.amd.com/r/en-US/Vitis_Libraries/L2-AIE-DSP-Library-User-Guide>`.


Library Element Unit Test
--------------------------

Each library element category comes supplied with a test harness. 

For AI Engine library elements, it is located in the `L2/tests/aie/<library_element>` directory and consists of JSON, C++ files, as well as a Makefile.

The JSON description of the test harness, defined in ``L2/tests/aie/<library_element>/description.json``, is used to generate the Makefile. In addition, the ``description.json`` file defines the parameters of the test harness, e.g., a list of supported platforms.

Each Makefile uses a set of values for each library element parameter that are stored in a JSON file at ``L2/tests/aie/<library_element>/multi_params.json``. The parameters are combined into named test cases, with the default name being ``test_0_tool_canary_aie``. You can edit these parameters as required to configure the library element for your needs.

C++ files serve as an example of how to use the library element subgraph in the context of a super-graph. These test harnesses (graphs) can be found in the `L2/tests/aie/<library_element>/test.hpp` and `L2/tests/aie/<library_element>/test.cpp` files.

Although for AI Engine library elements, it is recommended that only L2 (graphs) library elements are instantiated directly in the user code, the kernels underlying the graphs can be found in the `L1/include/aie/<library_element>.hpp` and the `L1/src/aie/<library_element>.cpp` files. 

The test harness run consists of several steps that result in a simulated and validated design. These include:

- Input file(s) generation.
- Validate configuration against metadata (in: `L2/meta`).
- Reference model compilation and simulation, to produce the `golden output`.
- UUT design compilation and simulation.
- Output post-processing (e.g., timestamp processing to produce throughput figures). The output of the reference model (``logs/ref_output.txt``) is verified against the output of the AI Engine graphs (``logs/uut_output.txt``).
- Output validation using linear algebra or other methods, depending on the library element. The validation report is stored in ``logs/validation.txt``. Note: A validation failure reported as ERROR indicates functional failure. If reported as WARNING, the functional test is considered passed; however, you must ensure the warning is acceptable for your application.
- Status generation. On completion of the make, the `logs/status_<config_details>.txt` file will contain the result of compilation, simulation, and an indication of whether the reference model and AI Engine model outputs match. The report will also contain resource utilization and performance metrics.


Compiling Using the Makefile
----------------------------

Running Compilation
^^^^^^^^^^^^^^^^^^^

Use the following steps to compile and simulate the reference model with the x86sim target, then to compile and simulate the library element graph as described in the above section.

.. code-block::

        make cleanall run PLATFORM=vck190

.. note:: It is recommended to run a ``cleanall`` stage before compiling the design to ensure no stale objects interfere with the compilation process.

.. note:: Platform information (e.g., PLATFORM=vck190) is a requirement of a make build process. A list of supported platforms can be found in `L2/tests/aie/<library_element>/description.json` in the "platform_allowlist" section.

Configuring the Test Case
^^^^^^^^^^^^^^^^^^^^^^^^^

To overwrite the default set of parameters, edit the `multi_params.json` file, and add a dedicated named test case or edit one of the existing ones, e.g.:

.. code-block::

    "test_my_design":{
        "DATA_TYPE": "cint32",
        "DIM_ROWS": "16",
        (...)
        }

To run a test case, specify the test case name passed to the PARAMS argument, e.g.:

.. code-block::

        make cleanall run PLATFORM=vck190 PARAMS=test_my_design

For a list of all configurable parameters, see :ref:`CONFIGURATION_PARAMETERS`.

Selecting TARGET
^^^^^^^^^^^^^^^^

To perform an x86 compilation/simulation, run:

.. code-block::

    make run TARGET=x86sim

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

The test harness supplied with the library allows each library unit to be compiled and simulated in isolation. When the library unit is instantiated within your design, the compilation result might differ from the result obtained with the test harness. This can occur because your system may require compiler arguments not present in the test harness.
This can occur because your system may require compiler arguments not present in the test harness.

Search the Makefile provided for UUT_TARGET_COMPILE_ARGS. For each library element, there can be compile arguments used to avoid errors or to improve performance, that is, specifying memories to be on separate banks to avoid wait states. These arguments will likely change with each release as the compile tool changes with each release.

Stack Size Allocation
^^^^^^^^^^^^^^^^^^^^^

Similarly, the test harness provided with each library unit estimates the stack size required for a variety of cases and creates a formula to assign sufficient amount of memory for stack purposes. When the library unit is instantiated within your design, compilation can fail with insufficient stack allocated for a specific kernel. The error message should suggest a minimum figure that is required.

Use the compiler argument to allocate enough stack as advised by the compiler message. Alternatively, search the Makefile provided for STACK_SIZE, and use the formula for the library unit to calculate sufficient stack size and allocate accordingly.

Invalid Throughput and/or Latency
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Throughput and latency are only reported when a stable operation has been detected. Complex designs might take several iterations to achieve a stable state. When a test case is not run for enough iterations, the status report will flag such a case with throughput and latency values set to -1.

Increase the number of iterations the simulation runs for to achieve a stable state and get accurate throughput and latency measurements.

Power Analysis
--------------

For SOLVERLIB elements, the naming convention 'VCD' can be used to harvest dynamic power consumption. When the string 'VCD' is included in the test name, a VCD file of the simulation data is captured and PDM (Power Design Manager) calculates power metrics. You can find detailed power reports in the ``pwr_test`` folder under the corresponding test result directory. Dynamic power results can also be found in the ``logs/status_<config_details>.txt`` file.

.. _CONFIGURATION_PARAMETERS:

Library Element Configuration Parameters
----------------------------------------

.. _COMMON_CONFIG_PARAMETERS:

Common Configuration Parameters
-------------------------------

Many library elements perform arithmetic and offer a scaling feature exposed as TP_SHIFT. During this operation, rounding and saturation can occur, configured according to parameters TP_RND and TP_SAT. The modes and values for TP_RND are the same for AIE-ML and AIE-MLv2 devices, but differ from those for AIE devices, as captured in the following table.

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

.. _CONFIGURATION_PARAMETERS_CHOLESKY:

Cholesky configuration parameters
--------------------------------------

For the Cholesky library element, use the following list of configurable parameters and default values.

.. table:: Cholesky configuration parameters

    +------------------------+----------------+----------------+--------------------------------------+
    |     **Name**           |    **Type**    |  **Default**   |   Description                        |
    +========================+================+================+======================================+
    | DATA_TYPE              |    typename    |    float       | Data Type.                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIM_SIZE               |    unsigned    |    32          | The dimension size of the matrix.    |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NUM_FRAMES             |    unsigned    |    1           | Number of frames in a window.        |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | GRID_DIM               |    unsigned    |    1           | The dimension size of the grid       |
    |                        |                |                | of kernels used to break up a matrix.|
    +------------------------+----------------+----------------+--------------------------------------+
    | NITER                  |    unsigned    |    4           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | STIM_TYPE              |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+

.. note:: Given parameter values are subject to checks early in compilation to ensure support.

.. _CONFIGURATION_PARAMETERS_QRD:

.. _CONFIGURATION_PARAMETERS_QRD:

QRD configuration parameters
--------------------------------------

For the QRD library element, use the following list of configurable parameters and default values.

.. table:: QRD configuration parameters

    +------------------------+----------------+----------------+--------------------------------------+
    |     **Name**           |    **Type**    |  **Default**   |   Description                        |
    +========================+================+================+======================================+
    | DATA_TYPE              |    typename    |    float       | Data Type.                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIM_ROWS               |    unsigned    |    32          | Row dimension of the input matrix.   |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIM_COLS               |    unsigned    |    32          | Column dimension of the input matrix.|
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NUM_FRAMES             |    unsigned    |    1           | Number of frames in a window.        |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | CASC_LEN               |    unsigned    |    1           | The number of cascaded kernels to    |
    |                        |                |                | be used to perform the function.     |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIM_A_LEADING          |    unsigned    |    1           | Matrix A data memory order.          |
    |                        |                |                | 1 is row major, 0 is column major.   |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIM_Q_LEADING          |    unsigned    |    1           | Matrix Q data memory order.          |
    |                        |                |                | 1 is row major, 0 is column major.   |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIM_R_LEADING          |    unsigned    |    1           | Matrix R data memory order.          |
    |                        |                |                | 1 is row major, 0 is column major.   |
    +------------------------+----------------+----------------+--------------------------------------+
    | NITER                  |    unsigned    |    4           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | STIM_TYPE              |    unsigned    |    0           | See :ref:`COMMON_CONFIG_PARAMETERS`  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+

.. note:: Given parameter values are subject to checks early in compilation to ensure support.

.. _LEGALITY_CHECKING:

Legality Checking
-----------------

During early compilation, parameter values are validated to ensure support by the metadata and build system. If a configuration is not supported, the build will report an error with details of the violation. Refer to the notes within each configuration table and to element-specific documentation for constraints.

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
