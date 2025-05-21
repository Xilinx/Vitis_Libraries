..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _INTRODUCTION:

********************************
Introduction for AIE DSP library
********************************

The DSP Library for AI Engine provides a set of DSP library elements. Each library element consists of a main graph class and a corresponding kernel class, but also includes a reference model graph and kernel classes.

====================================
Navigating Content by Design Process
====================================

AMD documentation is organized around a set of standard design processes to help you find relevant content for your current development task. This document covers the following design processes:

-  **AI Engine Development:** Creating the AI Engine graph and kernels, library use, simulation debugging and profiling, and algorithm development. Also includes the integration of the programmable logic (PL) and AI Engine kernels. Topics in this document that apply to this design process include:

   -  :ref:`ORGANIZATION`

   -  :ref:`USING`

   -  :ref:`KNOWN_ISSUES`

   -  :ref:`TUTORIALS`

   -  :ref:`DSP_LIB_FUNC`

   -  :ref:`COMPILING_AND_SIMULATING`

   -  :ref:`API_REFERENCE`

   -  :ref:`BENCHMARK`


-  **System and Solution Planning:** Identifying the components, performance, I/O, and data transfer requirements at a system level. Includes application mapping for the solution to the processing subsystem (PS), PL, and AI Engine. Topics in this document that apply to this design process include:

   -  :ref:`BITONIC_SORT`

   -  :ref:`CONVOLUTION_CORRELATION`

   -  :ref:`DDS_MIXER`

   -  :ref:`DFT`

   -  :ref:`EUCLIDEAN_DISTANCE`

   -  :ref:`FFT_IFFT`

   -  :ref:`FFT_2D`

   -  :ref:`FFT_WINDOW`

   -  :ref:`FILTERS`

   -  :ref:`FIR_TDM`

   -  :ref:`FUNCTION_APPROXIMATION`

   -  :ref:`HADAMARD_PRODUCT`

   -  :ref:`KRONECKER_MATRIX_PRODUCT`

   -  :ref:`MATRIX_MULTIPLY`

   -  :ref:`MATRIX_VECTOR_MULTIPLY`

   -  :ref:`MIXED_RADIX_FFT`

   -  :ref:`OUTER_TENSOR_PRODUCT`

   -  :ref:`SAMPLE_DELAY`

   -  :ref:`WIDGET_API_CAST`

   -  :ref:`WIDGET_REAL2COMPLEX`

-  **System Integration and Validation:** Integrating and validating the system functional performance, including timing, resource use, and power closure. Topics in this document that apply to this design process include:

   -  :ref:`COMPILING_AND_SIMULATING`

   -  :ref:`API_REFERENCE`

.. _ORGANIZATION:

============
Organization
============

The following figure shows the DSPLib organization.

**DSPLib Organization**

.. code-block::

   dsp
   ├── docs
   ├── ext
   ├── L1
   │   ├── examples
   │   ├── include
   │   │   ├── aie
   │   │   └── hw
   │   ├── meta
   │   ├── src
   │   │   ├── aie
   │   │   └── hw
   │   └── tests
   ├── L2
   │   ├── benchmarks
   │   ├── examples
   │   ├── include
   │   │   ├── aie
   │   │   ├── hw
   │   │   └── vss
   │   ├── meta
   │   └── tests
   │       ├── aie
   │       ├── hw
   │       └── vss
   └── scripts



The directories L1 and L2 correspond to the AI Engine kernels and AI Engine graphs for each function, respectively. Inclusion of an L2 graph rather than an L1 element is recommended in your design. L3 is reserved for future software drivers.

.. note:: The L3 directory is not yet available.

Graph class declarations and constants that allow you to include the library element in your design are located in `L2/include/aie/`. Kernel class definitions, the `.cpp` files and corresponding `.hpp` files are located in the `L1/src/aie` and `L1/include/aie` subdirectories respectively.

The `L2/tests/aie/<library_element>` subdirectory contains a test bench for the library element. Additional test bench files, like stimulus, monitor, and other utility modules are located in the `L1/tests/aie/inc/` folder.

Reference models graph class for each library element are contained in `L2/tests/aie/common/inc`. Reference models kernel class for each library element are contained in `L1/tests/aie/inc/` and `L1/tests/aie/src`.

The `L2/examples` subdirectory holds example wrapper designs to demonstrate the use of the library elements.

.. _USING:

=================================================
Using Library Elements within User Defined Graphs
=================================================

It is recommended that the library element to include in your graph is from the L2 directory, that is, a subgraph. For instance, to include a single rate asymmetrical FIR filter, include `fir_sr_asym_graph.hpp` from the `L2/include/aie/` folder. The test harness for each library unit can be used as a reference example of how to instantiate a parameterized graph. For example, see `L2/tests/aie/<library_element>/test.hpp` and `test.cpp`.

An example `test.h` and `test.cpp`, which instantiates a parameterized graph and exposes a configured (point solution) interface, is provided in the `L2/examples/fir_129t_sym` folder.

Set the environment variable to DSPLIB_ROOT.

.. code-block::

    setenv DSPLIB_ROOT <your-vitis-libraries-install-path/dsp>

.. note:: Use setenv for csh and export DSPLIB_ROOT=<path> for bash.

Use the following option in the aiecompiler command to provide the path:

.. code-block::

    -include=$DSPLIB_ROOT/L2/include/aie/
    -include=$DSPLIB_ROOT/L1/include/aie
    -include=$DSPLIB_ROOT/L1/src/aie

.. _KNOWN_ISSUES:

============
Known Issues
============

See Answer Record `75802 <https://www.xilinx.com/support/answers/75802.html>`__ for a list of known issues.


.. _TUTORIALS:

========================
Vitis Tutorials
========================

AMD provides an extensive library of purpose build tutorials. It is recommended to visit `Vitis Tutorials <https://github.com/Xilinx/Vitis-Tutorials>`__ to get familiar with the AMD Vitis |trade| in-Depth tutorials.

To learn how to use the Vitis core tools to develop for AMD Versal |trade|, the first Adaptive SoC, visit `AI Engine Development Tutorials <https://github.com/Xilinx/Vitis-Tutorials/tree/HEAD/AI_Engine_Development>`__. There is a variety of design, methodology, and feature tutorials, where you can also find a highly recommended `DSP Library Tutorial <https://github.com/Xilinx/Vitis-Tutorials/tree/HEAD/AI_Engine_Development/AIE/Feature_Tutorials/08-dsp-library>`__, which demonstrates how to use kernels provided by the DSP library for a filtering application, how to analyze the design results, and how to use filter parameters to optimize the design's performance using simulation.

Finally, Simulink users might be interested in the `AI Engine DSP Library and Model Composer Tutorial <https://github.com/Xilinx/Vitis-Tutorials/tree/HEAD/AI_Engine_Development/AIE/Feature_Tutorials/10-aie-dsp-lib-model-composer>`__, which shows how to design AI Engine applications using Model Composer.


.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:


