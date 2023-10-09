..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
    
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
    
       http://www.apache.org/licenses/LICENSE-2.0
    
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

.. _COMPILING_AND_SIMULATING:

************************
Compiling and Simulating
************************

**Prerequisites**:

.. code-block::

        source <your-Vitis-install-path>/lin64/Vitis/HEAD/settings64.csh
        setenv PLATFORM_REPO_PATHS <your-platform-repo-install-path>
        source <your-XRT-install-path>/xbb/xrt/packages/xrt-2.1.0-centos/opt/xilinx/xrt/setup.csh


Library Element Unit Test
--------------------------

Each library element category comes supplied with a test harness. It is located in the `L2/tests/aie/<library_element>` directory.
Test harness consists of JSON, C++ files, as well as a Makefile.

JSON description of the test harness, defined in `L2/tests/aie/<library_element>/description.json` has been used to generate Makefile. In addition, `description.json` file defines parameters of of the test harness, e.g. list of supported platforms.

Each Makefile uses a set of values for each of the library element parameters that are stored in in a JSON file, in `L2/tests/aie/<library_element>/multi_params.json`. Set of parameters are combined in a form of a named testcase, with default name being: `test_0_tool_canary_aie`.
Set of parameters can be edited as required to configure the library element for your needs.

C++ files serve as an example of how to use the library element subgraph in the context of a super-graph. These test harnesses (graphs) can be found in the `L2/tests/aie/<library_element>/test.hpp` and `L2/tests/aie/<library_element>/test.cpp` file.

Although it is recommended that only L2 (graphs) library elements are instantiated directly in user code, the kernels underlying the graphs can be found in the `L1/include/aie/<library_element>.hpp` and the `L1/src/aie/<library_element>.cpp` files.

Test harness run consists of several steps that result in a simulated and validated design. These include:
- input generation
- validate configuration with metadata
- ref model compilation & simulation, in order to produce `golden output`.
- uut design compilation & simulation
- output post-processing (e.g. timestamps processing to produce throughput figures).
  The output of the reference model ( `logs/ref_output.txt` ) is verified against the output of the AIE graphs ( `logs/uut_output.txt` ).
- status generation
  On completion of the make, the file `L2/tests/aie/<library_element>/logs/status_<config_details>.txt` will contain the result of compilation, simulation and an indication of whether the reference model and AIE model outputs match.

Compiing using Makefile
-----------------------

Use the following steps to compile and simulate the reference model with the x86sim target, then to compile and simulate the library element graph as described in the above section.

.. code-block::

        make cleanall run PLATFORM=vck190

.. note:: It is recommended to run a ``cleanall`` stage before compiling design, to ensue no stale objects interfere with the compilation process.

.. note:: PLATFORM information (e.g.: PLATFORM=vck190) is a requirement of a make build process. List of supported platforms can be found in `L2/tests/aie/<library_element>/description.json`.


To overwrite the default set of parameter, please edit multi_params.json file and add a dedicated named testcase or edit one of the existing ones, e.g.:

.. code-block::
    "test_my_design":{
        "DATA_TYPE": "cint32",
        "COEFF_TYPE": "int32",
        (...)
        }


To run a testcase, please specify the testcase name passed to the PARAMS argument, e.g.:

.. code-block::

        make cleanall run PLATFORM=vck190 PARAMS=test_my_design

For list of all the configurable parameters, see the :ref:`CONFIGURATION_PARAMETERS`.

To perform a x86 compilation/simulation, run:

.. code-block::

    make run TARGET=x86sim.

List of all Makefile targets:

.. code-block::

    make all TARGET=<aiesim/x86sim/hw_emu/hw> PLATFORM=<FPGA platform>
        Command to generate the design for specified Target and Shell.

    make run TARGET=<aiesim/x86sim/hw_emu/hw> PLATFORM=<FPGA platform>
        Command to run application in emulation.

    make clean
        Command to remove the generated non-hardware files.

    make cleanall
        Command to remove all the generated files.

.. note:: For embedded devices like vck190, env variable SYSROOT, EDGE_COMMON_SW and PERL need to be set first. For example,
        a.If the platform and common-image are downloaded from Xilinx Download Center(Suggested):
            Run the sdk.sh script from the common-image directory to install sysroot using the command : ./sdk.sh -y -d ./ -p
            Unzip the rootfs file : gunzip ./rootfs.ext4.gz
            export SYSROOT=< path-to-platform-sysroot >
        b.User could also define SYSROOT, K_IMAGE and ROOTFS by themselves:
            .. code-block::

                export SYSROOT=< path-to-platform-sysroot >
                export K_IMAGE=< path-to-Image-files >
                export ROOTFS=< path-to-rootfs >






Troubleshooting Compilation
---------------------------
The Makefiles supplied with the library allow each library unit to be compiled and simulated in isolation. When the library unit is instanced within your design, compilation may fail. This may be because compilation of your system may need arguments not present in your system. The following are possible compile-time errors and suggested remedies for each.

Stack size error. Search the Makefile provided for STACK_SIZE. This has a suggested formula for the library unit.
Other errors. Search the Makefile provided for UUT_TARGET_COMPILE_ARGS. For each library element there may be compile arguments used to avoid errors or to improve performance, e.g. specifying memories to be on separate banks to avoid wait states. These arguments will likely change with each release as the compile tool changes with each release.

.. _CONFIGURATION_PARAMETERS:

Library Element Configuration Parameters
----------------------------------------

.. _CONFIGURATION_PARAMETERS_DDS_MIXER:

DDS/Mixer Configuration Parameters
----------------------------------

For the DDS/Mixer library element, the list of configurable parameters and default values is presented below.

.. table:: L2 DDS/Mixer Configuration Parameters

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
    |                        |                |                | 0 = dds only                         |
    |                        |                |                |                                      |
    |                        |                |                | 1 = dds plus single data channel     |
    |                        |                |                | mixer                                |
    |                        |                |                |                                      |
    |                        |                |                | 2 = dds plus two data channel        |
    |                        |                |                | mixer, for symmetrical carrier       |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | P_API                  |    unsigned    |    0           | 0 = window,                          |
    |                        |                |                |                                      |
    |                        |                |                | 1 = stream                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | UUT_SSR                |    unsigned    |    1           | Super Sample Rate  SSR parameter.    |
    |                        |                |                | Defaults to 1.                       |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NITER                  |    unsigned    |    16          | Number of iterations to execute.     |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIFF_TOLERANCE         |    unsigned    |    0           | Tolerance value when comparing       |
    |                        |                |                | output sample with reference model,  |
    |                        |                |                | e.g. 0.0025 for floats and cfloats.  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | INITIAL_DDS_OFFSET     |    unsigned    |    0           | Initial DDS offset.                  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DDS_PHASE_INC          |    unsigned    | 0xD6555555     | DDS Phase Increment.                 |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DATA_STIM_TYPE         |    unsigned    |    0           | Supported types:                     |
    |                        |                |                |                                      |
    |                        |                |                | 0 - random                           |
    |                        |                |                |                                      |
    |                        |                |                | 3 - impulse                          |
    |                        |                |                |                                      |
    |                        |                |                | 4 - all ones                         |
    |                        |                |                |                                      |
    |                        |                |                | 5 - incrementing pattern             |
    |                        |                |                |                                      |
    |                        |                |                | 6 - sym incrementing pattern         |
    |                        |                |                |                                      |
    |                        |                |                | 8 - sine wave                        |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SAT_MODE               | Unsigned int   | Saturation     | 0: 'none'                            |
    |                        |                | mode           |                                      |
    |                        |                |                | 1: 'saturate'                        |
    |                        |                |                |                                      |
    |                        |                |                | 3: 'symmetric saturate'              |
    +------------------------+----------------+----------------+--------------------------------------+


.. _CONFIGURATION_PARAMETERS_FFT:

FFT configuration parameters
-------------------------------

For the FFT/iFFT library element the list of configurable parameters and default values is presented below.

.. table:: L2 FFT configuration parameters

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
    | SHIFT                  |    unsigned    |    17          | Acc results shift down value.        |
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
    |                        |                |                | 0 - window                           |
    |                        |                |                |                                      |
    |                        |                |                | 1 - stream                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | PARALLEL_POWER         |    unsigned    |   0            | Parallelism, controlling             |
    |                        |                |                | Super Sample Rate operation.         |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NITER                  |    unsigned    |    4           | Number of iterations to execute.     |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIFF_TOLERANCE         |    unsigned    |    0           | Tolerance value when comparing       |
    |                        |                |                | output sample with reference model,  |
    |                        |                |                | e.g. 0.0025 for floats and cfloats.  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | STIM_TYPE              |    unsigned    |    0           | Supported types:                     |
    |                        |                |                |                                      |
    |                        |                |                | 0 - random                           |
    |                        |                |                |                                      |
    |                        |                |                | 3 - impulse                          |
    |                        |                |                |                                      |
    |                        |                |                | 4 - all ones                         |
    |                        |                |                |                                      |
    |                        |                |                | 5 - incrementing pattern             |
    |                        |                |                |                                      |
    |                        |                |                | 6 - sym incrementing pattern         |
    |                        |                |                |                                      |
    |                        |                |                | 8 - sine wave                        |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SAT_MODE               | Unsigned int   | Saturation     | 0: 'none'                            |
    |                        |                | mode           |                                      |
    |                        |                |                | 1: 'saturate'                        |
    |                        |                |                |                                      |
    |                        |                |                | 3: 'symmetric saturate'              |
    +------------------------+----------------+----------------+--------------------------------------+

.. note:: The above configurable parameters range may exceed a library element's maximum supported range, in which case the compilation will end with a static_assert error informing about the exceeded range.

.. _CONFIGURATION_PARAMETERS_FFT_WINDOW:

FFT Window configuration parameters
--------------------------------------

For the FFT Window library element the list of configurable parameters and default values is presented below.

.. table:: L2 FFT Window configuration parameters

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
    | SHIFT                  |    unsigned    |    17          | Acc results shift down value.        |
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
    |                        |                |                | 0 - window                           |
    |                        |                |                |                                      |
    |                        |                |                | 1 - stream                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | WINDOW_CHOICE          |    unsigned    |    0           | Supported types:                     |
    |                        |                |                |                                      |
    |                        |                |                | 0 - Hamming                          |
    |                        |                |                |                                      |
    |                        |                |                | 1 - Hann                             |
    |                        |                |                |                                      |
    |                        |                |                | 2 - Blackman                         |
    |                        |                |                |                                      |
    |                        |                |                | 3 - Kaiser                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NITER                  |    unsigned    |    4           | Number of iterations to execute.     |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIFF_TOLERANCE         |    unsigned    |    0           | Tolerance value when comparing       |
    |                        |                |                | output sample with reference model,  |
    |                        |                |                | e.g. 0.0025 for floats and cfloats.  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SAT_MODE               | Unsigned int   | Saturation     | 0: 'none'                            |
    |                        |                | mode           |                                      |
    |                        |                |                | 1: 'saturate'                        |
    |                        |                |                |                                      |
    |                        |                |                | 3: 'symmetric saturate'              |
    +------------------------+----------------+----------------+--------------------------------------+

.. note:: The above configurable parameters range may exceed a library element's maximum supported range, in which case the compilation will end with a static_assert error informing about the exceeded range.

.. _CONFIGURATION_PARAMETERS_FILTERS:

FIR configuration parameters
-------------------------------

The list below consists of configurable parameters for FIR library elements with their default values.

.. table:: L2 FIR configuration parameters

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
    | SHIFT                  |    unsigned    |    16          | Acc results shift down value.        |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | ROUND_MODE             |    unsigned    |    0           | Rounding mode.                       |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | INPUT_WINDOW_VSIZE     |    unsigned    |    512         | Input window size.                   |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | CASC_LEN               |    unsigned    |    1           | Cascade length.                      |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | INTERPOLATE_FACTOR     |    unsigned    |    1           | Interpolation factor,                |
    |                        |                |                | see note below                       |
    +------------------------+----------------+----------------+--------------------------------------+
    | DECIMATE_FACTOR        |    unsigned    |    1           | Decimation factor,                   |
    |                        |                |                | see note below                       |
    +------------------------+----------------+----------------+--------------------------------------+
    | DUAL_IP                |    unsigned    |    0           | Dual inputs used in FIRs,            |
    |                        |                |                | see note below                       |
    +------------------------+----------------+----------------+--------------------------------------+
    | NUM_OUTPUTS            |    unsigned    |    1           | Number of output ports.              |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | USE_COEFF_RELOAD       |    unsigned    |    0           | Use 2 sets of reloadable             |
    |                        |                |                | coefficients, where the second set   |
    |                        |                |                | deliberately corrupts a single,      |
    |                        |                |                | randomly selected coefficient.       |
    +------------------------+----------------+----------------+--------------------------------------+
    | PORT_API               |    unsigned    |    0           | Graph's port API.                    |
    |                        |                |                |                                      |
    |                        |                |                | 0 - window                           |
    |                        |                |                |                                      |
    |                        |                |                | 1 - stream                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | UUT_SSR                |    unsigned    |    1           | Super Sample Rate  SSR parameter.    |
    |                        |                |                | Defaults to 1.                       |
    |                        |                |                | see note below                       |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NITER                  |    unsigned    |    16          | Number of iterations to execute.     |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIFF_TOLERANCE         |    unsigned    |    0           | Tolerance value when comparing       |
    |                        |                |                | output sample with reference model,  |
    |                        |                |                | e.g. 0.0025 for floats and cfloats.  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DATA_STIM_TYPE         |    unsigned    |    0           | Supported types:                     |
    |                        |                |                |                                      |
    |                        |                |                | 0 - random                           |
    |                        |                |                |                                      |
    |                        |                |                | 3 - impulse                          |
    |                        |                |                |                                      |
    |                        |                |                | 4 - all ones                         |
    |                        |                |                |                                      |
    |                        |                |                | 5 - incrementing pattern             |
    |                        |                |                |                                      |
    |                        |                |                | 6 - sym incrementing pattern         |
    |                        |                |                |                                      |
    |                        |                |                | 8 - sine wave                        |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | COEFF_STIM_TYPE        |    unsigned    |    0           | Supported types:                     |
    |                        |                |                |                                      |
    |                        |                |                | 0 - random                           |
    |                        |                |                |                                      |
    |                        |                |                | 3 - impulse                          |
    |                        |                |                |                                      |
    |                        |                |                | 4 - all ones                         |
    |                        |                |                |                                      |
    |                        |                |                | 5 - incrementing pattern             |
    |                        |                |                |                                      |
    |                        |                |                | 6 - sym incrementing pattern         |
    |                        |                |                |                                      |
    |                        |                |                | 8 - sine wave                        |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | USE_CUSTOM_CONSTRAINT  |    unsigned    |    0           | Overwrite default or non-existent.   |
    |                        |                |                |                                      |
    |                        |                |                | 0 - no action                        |
    |                        |                |                |                                      |
    |                        |                |                | 1 - use Graph's access functions     |
    |                        |                |                | to set a location and                |
    |                        |                |                | overwrite a fifo_depth constraint.   |
    |                        |                |                | see also :ref:`FIR_CONSTRAINTS`      |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SAT_MODE               | Unsigned int   | Saturation     | 0: 'none'                            |
    |                        |                | mode           |                                      |
    |                        |                |                | 1: 'saturate'                        |
    |                        |                |                |                                      |
    |                        |                |                | 3: 'symmetric saturate'              |
    +------------------------+----------------+----------------+--------------------------------------+

.. note:: The above configurable parameters range may exceed a library element's maximum supported range, in which case the compilation will end with a static_assert error informing about the exceeded range.

.. note:: Not all dsplib elements support all of the above configurable parameters. Unsupported parameters which are not used have no impact on execution, e.g., parameter `INTERPOLATE_FACTOR` is only supported by interpolation filters and will be ignored by other library elements.

.. _CONFIGURATION_PARAMETERS_GEMM:

Matrix Multiply Configuration Parameters
-------------------------------------------

For the Matrix Multiply (GeMM) library element the list of configurable parameters and default values is presented below.

.. table:: L2 Matrix Multiply configuration parameters

    +------------------------+----------------+----------------+--------------------------------------+
    |     **Name**           |    **Type**    |  **Default**   |   Description                        |
    +========================+================+================+======================================+
    | T_DATA_A               |    typename    |    cint16      | Input A Data Type.                   |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | T_DATA_B               |    typename    |    cint16      | Input B Data Type.                   |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | P_DIM_A                |    unsigned    |    16          | Input A Dimension                    |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | P_DIM_AB               |    unsigned    |    16          | Input AB Common Dimension.           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | P_DIM_B                |    unsigned    |    16          | Input B Dimension.                   |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | SHIFT                  |    unsigned    |    20          | Acc results shift down value.        |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | ROUND_MODE             |    unsigned    |    0           | Rounding mode.                       |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | P_CASC_LEN             |    unsigned    |    1           | Cascade length.                      |
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
    | NITER                  |    unsigned    |    16          | Number of iterations to execute.     |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIFF_TOLERANCE         |    unsigned    |    0           | Tolerance value when comparing       |
    |                        |                |                | output sample with reference model,  |
    |                        |                |                | e.g. 0.0025 for floats and cfloats.  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | STIM_TYPE_A            |    unsigned    |    0           | Supported types:                     |
    |                        |                |                |                                      |
    |                        |                |                | 0 - random                           |
    |                        |                |                |                                      |
    |                        |                |                | 3 - impulse                          |
    |                        |                |                |                                      |
    |                        |                |                | 4 - all ones                         |
    |                        |                |                |                                      |
    |                        |                |                | 5 - incrementing pattern             |
    |                        |                |                |                                      |
    |                        |                |                | 6 - sym incrementing pattern         |
    |                        |                |                |                                      |
    |                        |                |                | 8 - sine wave                        |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | STIM_TYPE_B            |    unsigned    |    0           | Supported types:                     |
    |                        |                |                |                                      |
    |                        |                |                | 0 - random                           |
    |                        |                |                |                                      |
    |                        |                |                | 3 - impulse                          |
    |                        |                |                |                                      |
    |                        |                |                | 4 - all ones                         |
    |                        |                |                |                                      |
    |                        |                |                | 5 - incrementing pattern             |
    |                        |                |                |                                      |
    |                        |                |                | 6 - sym incrementing pattern         |
    |                        |                |                |                                      |
    |                        |                |                | 8 - sine wave                        |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | P_SAT_MODE             | Unsigned int   | Saturation     | 0: 'none'                            |
    |                        |                | mode           |                                      |
    |                        |                |                | 1: 'saturate'                        |
    |                        |                |                |                                      |
    |                        |                |                | 3: 'symmetric saturate'              |
    +------------------------+----------------+----------------+--------------------------------------+

.. note:: The above configurable parameters range may exceed a library element's maximum supported range, in which case the compilation will end with a static_assert error informing about the exceeded range.


.. _CONFIGURATION_PARAMETERS_WIDGETS:

Widgets Configuration Parameters
-----------------------------------

For the Widgets library elements the list of configurable parameters and default values is presented below.

.. table:: L2 Widget API Casts Configuration Parameters

    +------------------------+----------------+----------------+--------------------------------------+
    |     **Name**           |    **Type**    |  **Default**   |   Description                        |
    +========================+================+================+======================================+
    | DATA_TYPE              |    typename    |    cint16      | Data Type.                           |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | IN_API                 |    unsigned    |    0           | 0 = window,                          |
    |                        |                |                |                                      |
    |                        |                |                | 1 = stream                           |
    +------------------------+----------------+----------------+--------------------------------------+
    | OUT_API                |    unsigned    |    0           | 0 = window,                          |
    |                        |                |                |                                      |
    |                        |                |                | 1 = stream                           |
    +------------------------+----------------+----------------+--------------------------------------+
    | NUM_INPUTS             |    unsigned    |    1           | The number of input stream           |
    |                        |                |                | interfaces                           |
    +------------------------+----------------+----------------+--------------------------------------+
    | WINDOW_VSIZE           |    unsigned    |    256         | Input/Output window size.            |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NUM_OUTPUT_CLONES      |    unsigned    |    1           | The number of output window          |
    |                        |                |                | port copies                          |
    +------------------------+----------------+----------------+--------------------------------------+
    | PATTERN                |    unsigned    |    0           | The pattern of interleave            |
    |                        |                |                | by which samples from each           |
    |                        |                |                | of 2 streams are arranged            |
    |                        |                |                |                                      |
    |                        |                |                | into the destination window,         |
    |                        |                |                | or from the input window             |
    |                        |                |                | to dual output streams.              |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | NITER                  |    unsigned    |    16          | Number of iterations to execute.     |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIFF_TOLERANCE         |    unsigned    |    0           | Tolerance value when comparing       |
    |                        |                |                | output sample with reference model,  |
    |                        |                |                | e.g. 0.0025 for floats and cfloats.  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DATA_STIM_TYPE         |    unsigned    |    0           | Supported types:                     |
    |                        |                |                |                                      |
    |                        |                |                | 0 - random                           |
    |                        |                |                |                                      |
    |                        |                |                | 3 - impulse                          |
    |                        |                |                |                                      |
    |                        |                |                | 4 - all ones                         |
    |                        |                |                |                                      |
    |                        |                |                | 5 - incrementing pattern             |
    |                        |                |                |                                      |
    |                        |                |                | 6 - sym incrementing pattern         |
    |                        |                |                |                                      |
    |                        |                |                | 8 - sine wave                        |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+


.. table:: L2 Widget Real to Complex Configuration Parameters

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
    | NITER                  |    unsigned    |    16          | Number of iterations to execute.     |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DIFF_TOLERANCE         |    unsigned    |    0           | Tolerance value when comparing       |
    |                        |                |                | output sample with reference model,  |
    |                        |                |                | e.g. 0.0025 for floats and cfloats.  |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | DATA_STIM_TYPE         |    unsigned    |    0           | Supported types:                     |
    |                        |                |                |                                      |
    |                        |                |                | 0 - random                           |
    |                        |                |                |                                      |
    |                        |                |                | 3 - impulse                          |
    |                        |                |                |                                      |
    |                        |                |                | 4 - all ones                         |
    |                        |                |                |                                      |
    |                        |                |                | 5 - incrementing pattern             |
    |                        |                |                |                                      |
    |                        |                |                | 6 - sym incrementing pattern         |
    |                        |                |                |                                      |
    |                        |                |                | 8 - sine wave                        |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+


.. note:: The above configurable parameters range may exceed a library element's maximum supported range, in which case the compilation will end with a static_assert error informing about the exceeded range.


.. |image1| image:: ./media/image1.png
.. |image2| image:: ./media/image2.png
.. |image3| image:: ./media/image4.png
.. |image4| image:: ./media/image2.png
.. |image6| image:: ./media/image2.png
.. |image7| image:: ./media/image5.png
.. |image8| image:: ./media/image6.png
.. |image9| image:: ./media/image7.png
.. |image10| image:: ./media/image2.png
.. |image11| image:: ./media/image2.png
.. |image12| image:: ./media/image2.png
.. |image13| image:: ./media/image2.png
.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:


