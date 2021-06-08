************
Introduction
************

The AIE DSP Library for AI Engine provides a set of DSP library elements.
Each library elements consists of a main graph class and a corresponding kernel class, but also includes a reference model graph class.


====================================
Navigating Content by Design Process
====================================

Xilinx® documentation is organized around a set of standard design processes to help you find relevant content for your current development task. This document covers the following design processes:

-  **AI Engine Development:** Creating the AI Engine graph and kernels, library use, simulation debugging and profiling, and algorithm development. Also includes the integration of the PL and AI Engine kernels. Topics in this document that apply to this design process include:

   -  `Using Library Elements within User Defined Graphs <#using-library-elements-within-user-defined-graphs>`__

   -  `DSP Library Functions <user_guide/L2/2-dsp-lib-func.rst>`__

   -  `Using the Examples <user_guide/L2/3-using-examples.rst>`__

-  **System and Solution Planning:** Identifying the components, performance, I/O, and data transfer requirements at a system level. Includes application mapping for the solution to PS, PL, and AI Engine. Topics in this document that apply to this design process include:

   -  `Filters <user_guide/L2/2-dsp-lib-func#filters>`__

   -  `FFT/iFFT <user_guide/L2/2-dsp-lib-func#fftifft>`__

   -  `Matrix Multiply <user_guide/L2/2-dsp-lib-func#matrix-multiply>`__

-  **System Integration and Validation:** Integrating and validating the system functional performance, including timing, resource use, and power closure. Topics in this document that apply to this design process include:

   -  `Using the Examples <user_guide/L2/3-using-examples.rst>`__

   -  `DSPLib Functions API Reference <user_guide/L2/4-api-reference.rst>`__

============
Organization
============

The following figure shows the DSPLib organization.
TODO: Picture updated organization:

└── xf_dsp
    ├── docs
    ├── ext
    ├── L1
    │   ├── examples
    │   ├── include
    │   │   ├── aie
    │   │   └── hw
    │   ├── src
    │   │   └── aie
    │   └── tests
    │       ├── aie
    │       └── hw
    └── L2
        ├── benchmarks
        ├── examples
        ├── include
        │   ├── aie
        │   └── hw
        └── tests
            ├── aie
            └── hw


   *Figure 1:* **DSPLib Organization**

X24061-111820

The directories L1, L2 and L3 correspond to AI Engine kernels, AI Engine Graphs and drivers for each function respectively. Inclusion of an L2 graph rather than an L1 element is recommended in your design.

.. note:: The L3 directory is not yet available.

Graph class declarations and constants that allow you to include the library element in your design are located in the `L2/include/aie/`.

Kernel class definitions - the .cpp files and corresponding  .hpp files are located in the `L1/src/aie` and `L1/include/aie` subdirectories respectively.

The `L2/tests/aie/<library_element>` subdirectory contains a test bench for the library element.
Additional testbench files, like stimulus, monitor and other utility modules are contained in `L1/tests/aie/inc/`.

Reference models graph class for each library element are contained in `L2/tests/aie/common/inc`.
Reference models kernel class for each library element are contained in `L1/tests/aie/inc/` and `L1/tests/aie/src`.

The `L2/examples` subdirectory holds example wrapper designs to demonstrate the use of the library elements.

=================================================
Using Library Elements within User Defined Graphs
=================================================

It is recommended that the library element to include in your graph is from the L2 directory, that is, a subgraph. For instance, to include a single rate asymmetrical FIR filter, include `fir_sr_asym_graph.hpp` from the `L2/include/aie/` folder. The test harness for each library unit can be used as a reference example of how to instantiate a parameterized graph. For example, see `L2/tests/aie/<library_element>/test.hpp` and `test.cpp`.
An example `test.h` and `test.cpp` which instantiates a parameterized graph and exposes a configured (point solution) interface is provided in `L2/examples/fir_129t_sym` folder.

Set the environment variable to DSPLIB_ROOT.

.. code-block::

    setenv DSPLIB_ROOT <your-install-directory/xf_dsp>

.. note:: Use setenv for csh and export DSPLIB_ROOT=<path> for bash.

Use the following option in aiecompiler command to provide the path:

.. code-block::

    -include=$DSPLIB_ROOT/L2/include/aie/
    -include=$DSPLIB_ROOT/L1/include/aie
    -include=$DSPLIB_ROOT/L1/src/aie


=============
Benchmark/QoR
=============

TODO: Provide link

============
Known Issues
============

See Xilinx Answer Record `75802 <https://www.xilinx.com/support/answers/75802.html>`__ for the list of known issues.
TODO: Add CRs to AR:
https://jira.xilinx.com/browse/CR-1099818
https://jira.xilinx.com/browse/CR-1095725



