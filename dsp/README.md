# Vitis DSP Library

Vitis DSP library provides implementation of different L1/L2/L3 primitives for digital signal processing. Current version only provides implementation of Discrete Fourier Transform using Fast Fourier Transform algorithm for acceleration on Xilinx FPGAs.

Note: For FFT only L1 level primitives are provided.

[Comprehensive documentation](https://xilinx.github.io/Vitis_Libraries/dsp/2020.2/)

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

# AIE OVERVIEW

The AIE DSP library contains DSP building blocks in the form of AIE graphs.
The AIE DSP library is delivered as a tarball to support Vitis 2020.2.
For details refer to UG1295.

# SOFTWARE/HARDWARE REQUIREMENTS
The library in this tarball supports Xilinx Vitis 2020.2 only.
`DSPLIB_ROOT` (environment variable) should be set to the location of the
dsplib folder once extracted for use of scripts and Makefiles.

# HIGHLIGHTS
The library currently contains the following parameterized elements:
- fir_sr_asym                - asymmetric single rate FIR
- fir_sr_sym                 - symmetric single rate FIR
- fir_interpolate_hb         - halfband interpolation FIR
- fir_interpolate_asym       - asymmetric interpolation FIR
- fir_decimate_hb            - halfband decimation FIR
- fir_decimate_sym           - decimation FIR using symmetrical coefficients
- fir_decimate_asym          - asymmetric decimation FIR
- fir_interpolate_fract_asym - asymmetric fractional rate interpolator FIR

- fft_ifft_dit_1ch           - single channel FFT/iFFT transform using decimation in time.
- matrix_mult                - matrix multiply

For details refer to User Guide UG1295.

For each DSPLIB element listed above the recommended entry point is a graph class.
These graph source files are located in the dsplib/L2/include/hw folder.
There is a test harness for each DSPLIB element, located in:
dsplib/L2/tests/DSPLIB_ELEMENT_NAME>/proj/  e.g. dsplib/L2/tests/fir_sr_asym/proj.
Test harness consists of `test.cpp` and `test.hpp`.
While these test files are the top level for a functional regression test, together
they offer an example of how to instantiate eacg UUT graph class.


## EXAMPLE DESIGNS
The AIE DSP library comes with several example designs: fir_129t_sym, fir_129t_sym_reload,
fir_91t_interpolate_hb_reload.

'dsplib/L2/examples/fir_129t_sym' project includes an example design of a 129
tap single rate symmetric FIR. Its purpose is to illustrate a DSPLIB element
instantiation in user's graphs, including graph initialization with argument
passing.

'dsplib/L2/examples/fir_129t_sym_reload' is the above example altered to use the coefficient
reload feature.

'dsplib/L2/examples/fir_91t_interpolate_hb_reload' shows a 91 tap halfband interpolator
with reloadable coefficients.

Each example comes with a Readme.txt file explaining how the example may be run.
This Readme.txt file can be found in dsplib/L2/examples/<examplename>/proj/

## BENCHMARK/QOR

Please see the lounge for benchmark data.

## VITIS LIBRARIES PORTALS

## MAKEFILE USAGE
The default flow gets invoked with from `L2/tests/<DSPLIB_element>/proj` directory,
using the make command with a default (all) target.

    make all

This will perform reference model compilation and simulation targeting x86sim
simulator.
Then, <DSPLib_element> will be compiled and simulated. This defaults to target aie,
but the optional argument UUT_TARGET=x86sim may be added to the make all command
to target x86sim.
Finally, outputs will be compared, results will be gathered and a summary displayed.

To pass user parameters use the `make` command with default (all target) overwriting
Makefile parameters:

    make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=24
This will perform all actions as described above for the default target with the
specified parameters overwritten with user specified values.

For a list of configurable parameters see below **CONFIGURATION PARAMETERS**.

For more details see UG1295.

### EXAMPLE MAKEFILE USAGE
1. Test a 129t single rate symmetric FIR.
    `cd L2/tests/fir_sr_sym/proj`
    `make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=129 SHIFT=15 ROUND_MODE=0 INPUT_WINDOW_VSIZE=256`
2. Test the example from point 1 with user defined coefficients
    `cd L2/tests/fir_sr_sym/proj`
    `make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=129 SHIFT=15 ROUND_MODE=0 INPUT_WINDOW_VSIZE=256 GEN_COEFF_DATA=false COEFF_FILE=$USER_DEFINED_COEFF_FILE`
3. Test the example from point 1 with user defined stimulus data:
    `cd L2/tests/fir_sr_sym/proj`
    `make all DATA_TYPE=cint16 COEFF_TYPE=int16 FIR_LEN=129 SHIFT=15 ROUND_MODE=0 INPUT_WINDOW_VSIZE=256 GEN_INPUT_DATA=false INPUT_FILE=$USER_DEFINED_INPUT_FILE`

## CONFIGURATION PARAMETERS
The list below consists of configurable parameters for FIR library elements with their default values.
DATA_TYPE           - cint16
COEFF_TYPE          - int16
FIR_LEN             - 81
SHIFT               - 16
ROUND_MODE          - 0
INPUT_WINDOW_VSIZE  - 512
NITER               - 4
CASC_LEN            - 1
INTERPOLATE_FACTOR  - 1
DECIMATE_FACTOR     - 1
DUAL_IP             - 0
USE_COEFF_RELOAD    - 0
GEN_INPUT_DATA      - true
GEN_COEFF_DATA      - true
STIM_TYPE           - 0
INPUT_FILE          - data/input.txt
COEFF_FILE          - data/coeff.txt
HEAPSIZE_VAL        - This is determined by an equation of other parameters above, see makefile
STACKSIZE_VAL       - This is determined by an equation of other parameters above, see makefile

For the FFT/iFFT library element the list of configurable parameters and default values is:
DATA_TYPE           - cint16
TWIDDLE_TYPE        - cint16
POINT_SIZE          - 16
FFT_NIFFT           - 1
GEN_INPUT_DATA      - true
GEN_COEFF_DATA      - true
STIM_TYPE           - 0
INPUT_FILE          - data/input.txt
HEAPSIZE_VAL        - This is determined by an equation of other parameters above, see makefile
STACKSIZE_VAL       - This is determined by an equation of other parameters above, see makefile


Note: Not all dsplib elements support all of the above configurable parameters.
Unsupported parameters which are not used have no impact on execution,
e.g. parameter `INTERPOLATE_FACTOR` is only supported by interpolation filters
and will be ignored by other library elements.

## KNOWN ISSUES
1. AIEcompilation may fail during linking process with an error similar to the one below:
    `Core 1_0: Estimated stack size 3968 bytes exceeds the alloted stack size 2496 bytes`
    `gmake[1]: *** [1_0] Error 1`
    `ERROR: [aiecompiler 77-753] This application has discovered an exceptional condition from which it cannot recover while executing the following command`

    This error is a result of insufficient stack size allocation.
Stack size is allocated based on estimated formula, so that minimum sufficient
amount of stack is allocated. It has been observed that the estimation is not
accurate, particularly with relatively large coefficient arrays.
To solve this issue, increase the allocated stack size to at least the
amount referred in the error message.

2. AIEcompilation may fail during linking process with an error similar to the one below:
    `Error: Could not find free space`
    ... `Could not find space for SpaceSymbol i2 in memory DMb`
    ... Symbol was found in file   : `../Release/1_0.o`
    `Error in "../Release/1_0.map": Linking ended with errors, check the map file for an overview of already mapped symbols.`
    `Compilation not finished (2 errors, 0 warnings)`
    `xchessmk Failed `

    This error is a result of insufficient heap size allocation.
Heap size is allocated based on estimated formula, so that minimum sufficient
amount of heap is allocated.
To solve this issue, increase the allocated heap size using HEAPSIZE_VAL switch.

3. AIEcompilation may fail with an error similar to the one below:
    `ERROR:DMAAllocation :ERROR: [aiecompiler 77-1159] BufferInfo* vector size should be > 0`

    This may be preceeded with:
    `DEBUG:MapperPartitioner:Create Edge: Name=net1 SrcPort=i2_po0 DstPort=i1_pi0 Size=36864 type=mem
    ERROR  exceeding 32k bytes not supported`

    This error is a result of ping-pong synchronization buffer exceeding 32k Bytes limit.
Ping-pong buffers are double the size of the window buffer used within the kernel, therefore,
kernel`s window size should not exceed 16 kBytes.
To solve this issue, reduce input/output window size to less than 16k Bytes.

4. AIEcompilation may fail during mapping process with an error similar to the one below:
    `GP ILP solver failed!`
    `ERROR: [aiecompiler 47-51] AIE Mapper failed to find a legal solution. Please try to relax constraints and/or try alternate strategies like disableFloorplanning.`

    This error may be a result of ping-pong synchronization buffer exceeding 32k Bytes limit.
Ping-pong buffers are double the size of the window buffer used within the kernel, therefore,
kernel`s window size should not exceed 16 kBytes.
This could be configrem looking at MapperPartitioner messages:
    `DEBUG:MapperPartitioner:Create Edge: Name=net1 SrcPort=i2_po0 DstPort=i1_pi0 Size=20480 type=mem`

    To solve this issue, reduce input/output window size to less than 16k Bytes.

5. Some FFT cases using DATA_TYPE=cfloat may apparently fail the supplied test run. The same configuration may be seen to pass
by targeting x86sim for both reference model and aie source code.

6. Larger FFT sizes (POINT_SIZE=4096 and even 2048 when DATA_TYPE = cint32 or cfloat may fail with a
mapper error such as that cited in point 4. above. This is due to the double-buffering of input and output windows.
It may be overcome by forcing single buffer use or by co-locating stream to memorymap and memory map to stream kernels
in the same processor, which also has the effect of requiring only a single buffer.

7. FFT or IFFT of POINT_SIZE=16 for DATA_TYPE of cint16 or cint32 fails to complete simulation in either x86sim or aie.
There is no workaround for this at present.




