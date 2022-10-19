..
   Copyright 2022 Xilinx, Inc.

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

*******************************************
Compiling and Simulating Using the Makefile
*******************************************

A Makefile is included with each library element. It is located in the `L2/tests/aie/<library_element>` directory. Each Makefile holds default values for each of the library element parameters. These values can be edited as required to configure the library element for your needs. Alternatively, these defaults may be overridden by arguments to the make command as described below.

In addition, example design(s) located in: `L2/examples/<example_design>` contain a Makefile that offers similar functionality.
Example design(s) are not parametrizable.

Prerequisites:

.. code-block::

        source <your-Vitis-install-path>/lin64/Vitis/HEAD/settings64.csh
        setenv PLATFORM_REPO_PATHS <your-platform-repo-install-path>
        source <your-XRT-install-path>/xbb/xrt/packages/xrt-2.1.0-centos/opt/xilinx/xrt/setup.csh
        setenv DSPLIB_ROOT <your-Vitis-libraries-install-path/dsp>


Use the following steps to compile and simulate the reference model with the x86sim target, then to compile and simulate the library element graph using the AIE emulation platform. The output of the reference model ( `logs/ref_output.txt` ) is verified against the output of the AIE graphs ( `logs/uut_output.txt` ).

.. code-block::

        make run

To overwrite the default parameters, add desired parameters as arguments to the make command, for example:

.. code-block::

        make run DATA_TYPE=cint16 SHIFT=16

For list of all the configurable parameters, see the :ref:`CONFIGURATION_PARAMETERS`.

List of all Makefile targets:

.. code-block::

    make all TARGET=<x86sim/aiesim/> PLATFORM=<FPGA platform>
        Command to generate the design for specified Target and Shell.


    make sd_card TARGET=<x86sim/aiesim/> PLATFORM=<FPGA platform>
        Command to prepare sd_card files.


    make run TARGET=<x86sim/aiesim/> PLATFORM=<FPGA platform>
        Command to run application in emulation.


    make xclbin TARGET=<x86sim/aiesim/> PLATFORM=<FPGA platform>
        Command to build xclbin application.


    make host
        Command to build host application.


    make clean
        Command to remove the generated non-hardware files.

    make cleanall
        Command to remove all the generated files.

.. note:: For embedded devices like vck190, env variable SYSROOT, EDGE_COMMON_SW and PERL need to be set first. For example,

            .. code-block::

                export SYSROOT=< path-to-platform-sysroot >
                export EDGE_COMMON_SW=< path-to-rootfs-and-Image-files >
                export PERL=<path-to-perl-installation-location >

On completion of the make, the file `L2/tests/aie/<library_element>/logs/status.txt` will contain the result of compilation, simulation and an indication of whether the reference model and AIE model outputs match.

To perform a x86 compilation/simulation, run:

.. code-block::

    make run TARGET=x86sim.

It is also possible to randomly generate coefficient and input data, or to generate specific stimulus patterns like ALL_ONES, IMPULSE, etc. by running:

.. code-block::

      make run STIM_TYPE=4.

L2 Library Element Unit Test
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Each library element category comes supplied with a test harness which is an example of how to use the library element subgraph in the context of a super-graph. These test harnesses (graphs) can be found in the `L2/tests/aie/<library_element>/test.hpp` and `L2/tests/aie/<library_element>/test.cpp` file.

Although it is recommended that only L2 (graphs) library elements are instantiated directly in user code, the kernels underlying the graphs can be found in the `L1/include/aie/<library_element>.hpp` and the `L1/src/aie/<library_element>.cpp` files.

An example of how a library element may be configured by a parent graph is provided in the `L2/examples/fir_129t_sym` folder. The example graph, test.h, in the `L2/examples/fir_129t_sym` folder instantiates the fir_sr_sym graph configured to be a 129-tap filter. This example exposes the ports such that the parent graph can be used to replace an existing 129-tap symmetric filter point solution design.

.. _CONFIGURATION_PARAMETERS:

L2 Library Element Configuration Parameters
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. _CONFIGURATION_PARAMETERS_DDS_MIXER:

L2 DDS/Mixer Configuration Parameters
-------------------------------------

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


.. _CONFIGURATION_PARAMETERS_FFT:

L2 FFT configuration parameters
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
    | GEN_INPUT_DATA         |    bool        |    true        | Generate random input data samples.  |
    |                        |                |                |                                      |
    |                        |                |                | When false, use the input file       |
    |                        |                |                | defined in: INPUT_FILE               |
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
    | INPUT_FILE             |    string      | data/input.txt | Input data samples file.             |
    |                        |                |                |                                      |
    |                        |                |                | Only used when GEN_INPUT_DATA=false. |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+

.. note:: The above configurable parameters range may exceed a library element's maximum supported range, in which case the compilation will end with a static_assert error informing about the exceeded range.

.. _CONFIGURATION_PARAMETERS_FFT_WINDOW:

L2 FFT Window configuration parameters
--------------------------------------

For the FFT Window library element the list of configurable parameters and default values is presented below.

.. table:: L2 FFT Window configuration parameters

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
    |                        |                |                | 3 - Keiser                           |
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

.. note:: The above configurable parameters range may exceed a library element's maximum supported range, in which case the compilation will end with a static_assert error informing about the exceeded range.

.. _CONFIGURATION_PARAMETERS_FILTERS:

L2 FIR configuration parameters
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
    | GEN_INPUT_DATA         |    bool        |    true        | Generate input data samples.         |
    |                        |                |                |                                      |
    |                        |                |                | When true, generate stimulus data    |
    |                        |                |                | as defined in: DATA_STIM_TYPE.       |
    |                        |                |                |                                      |
    |                        |                |                | When false, use the input file       |
    |                        |                |                | defined in: INPUT_FILE               |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | GEN_COEFF_DATA         |    bool        |    true        | Generate random coefficients.        |
    |                        |                |                |                                      |
    |                        |                |                | When true, generate stimulus data    |
    |                        |                |                | as defined in: COEFF_STIM_TYPE.      |
    |                        |                |                |                                      |
    |                        |                |                | When false, use the coefficient file |
    |                        |                |                | defined in: COEFF_FILE               |
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
    | INPUT_FILE             |    string      | data/input.txt | Input data samples file.             |
    |                        |                |                |                                      |
    |                        |                |                | Only used when GEN_INPUT_DATA=false. |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | COEFF_FILE             |    string      | data/coeff.txt | Coefficient data file.               |
    |                        |                |                |                                      |
    |                        |                |                | Only used when GEN_COEFF_DATA=false. |
    |                        |                |                |                                      |
    +------------------------+----------------+----------------+--------------------------------------+
    | USE_CHAIN              |    unsigned    |    0           | Connect 2 FIRs back-to-back.         |
    |                        |                |                |                                      |
    |                        |                |                | 0 - connect single FIR               |
    |                        |                |                |                                      |
    |                        |                |                | 1 - connect second FIR back-to-back. |
    |                        |                |                | In/Out interfaces must be            |
    |                        |                |                | compatible.                          |
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

.. note:: The above configurable parameters range may exceed a library element's maximum supported range, in which case the compilation will end with a static_assert error informing about the exceeded range.

.. note:: Not all dsplib elements support all of the above configurable parameters. Unsupported parameters which are not used have no impact on execution, e.g., parameter `INTERPOLATE_FACTOR` is only supported by interpolation filters and will be ignored by other library elements.

.. _CONFIGURATION_PARAMETERS_GEMM:

L2 Matrix Multiply Configuration Parameters
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

.. note:: The above configurable parameters range may exceed a library element's maximum supported range, in which case the compilation will end with a static_assert error informing about the exceeded range.


.. _CONFIGURATION_PARAMETERS_WIDGETS:

L2 Widgets Configuration Parameters
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


