..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _SOLVER_INTRODUCTION:

***********************************
Introduction to AIE Solver Library
***********************************

The Solver library for AI Engine provides a set of linear-algebra library elements. Each library element consists of a main graph class and corresponding kernel classes; it also includes reference-model graph and kernel classes.

====================================
Navigating Content by Design Process
====================================

AMD documentation is organized around a set of standard design processes to help you find relevant content for your current development task. This document covers the following design processes:

-  **AI Engine Development:** Creating the AI Engine graph and kernels, library use, simulation debugging and profiling, and algorithm development. Also includes the integration of the programmable logic (PL) and AI Engine kernels. Topics in this document that apply to this design process include:

   -  :ref:`SOLVER_ORGANIZATION`

   -  :ref:`SOLVER_USING`

   -  :ref:`SOLVER_KNOWN_ISSUES`

   -  :ref:`SOLVER_TUTORIALS`

   -  :ref:`SOLVER_SOLVER_LIB_FUNC`

   -  :ref:`SOLVER_COMPILING_AND_SIMULATING`

   -  :ref:`SOLVER_API_REFERENCE`

   -  :ref:`SOLVER_BENCHMARK`


-  **System and Solution Planning:** Identifying the components, performance, I/O, and data transfer requirements at a system level. Includes application mapping for the solution to the processing subsystem (PS), PL, and AI Engine. Topics in this document that apply to this design process include:

   -  :ref:`SOLVER_CHOLESKY`

   -  :ref:`SOLVER_QRD`

-  **System Integration and Validation:** Integrating and validating the system functional performance, including timing, resource use, and power closure. Topics in this document that apply to this design process include:

   -  :ref:`SOLVER_COMPILING_AND_SIMULATING`

   -  :ref:`SOLVER_API_REFERENCE`

.. _SOLVER_ORGANIZATION:

============
Organization
============

The following figure shows the SolverLib organization.

**SolverLib Organization**

.. graphviz::

   digraph solver {
       node [shape=folder];
       solver -> docs;
       solver -> ext;
       solver -> L1;
       solver -> L2;
       solver -> scripts;

       L1 -> L1_include;
       L1 -> L1_meta;
       L1 -> L1_src;
       L1 -> L1_tests;

       L1_include -> L1_include_aie;
       L1_include -> L1_include_hw;

       L1_src -> L1_src_aie;

       L1_tests -> L1_tests_aie;
       L1_tests -> L1_tests_src;

       L2 -> L2_benchmarks;
       L2 -> L2_examples;
       L2 -> L2_include;
       L2 -> L2_meta;
       L2 -> L2_tests;

       L2_examples -> L2_examples_docs;

       L2_include -> L2_include_aie;
       L2_include -> L2_include_hw;

       L2_tests -> L2_tests_aie;
       L2_tests -> L2_tests_hw;

       L1_include [label="include"];
       L1_include_aie [label="aie"];
       L1_include_hw [label="hw"];
       L1_meta [label="meta"];
       L1_src [label="src"];
       L1_src_aie [label="aie"];
       L1_tests [label="tests"];
       L1_tests_aie [label="aie"];
       L1_tests_src [label="src"];

       L2_include [label="include"];
       L2_include_aie [label="aie"];
       L2_include_hw [label="hw"];
       L2_benchmarks [label="benchmarks"];
       L2_examples [label="examples"];
       L2_examples_docs [label="docs"];
       L2_meta [label="meta"];
       L2_tests [label="tests"];
       L2_tests_aie [label="aie"];
       L2_tests_hw [label="hw"];
   }




The directories L1 and L2 correspond to the AI Engine kernels and AI Engine graphs for each function, respectively. Inclusion of an L2 graph rather than an L1 element is recommended in your design. L3 is reserved for future software drivers.

.. note::

   The L3 directory is not yet available.

Graph class declarations and constants that allow you to include the library element in your design are located in `L2/include/aie/`. Kernel class definitions, the `.cpp` files and corresponding `.hpp` files are located in the `L1/src/aie` and `L1/include/aie` subdirectories respectively.

The ``L2/tests/aie/<library_element>`` subdirectory contains a testbench for the library element. Additional testbench files, such as stimulus, monitor, and other utility modules, are located in the ``L1/tests/aie/inc/`` folder.

Reference model graph classes for each library element are contained in ``L2/tests/aie/common/inc``. Reference model kernel classes for each library element are contained in ``L1/tests/aie/inc/`` and ``L1/tests/aie/src``.

The `L2/examples` subdirectory holds example wrapper designs to demonstrate the use of the library elements.

.. _SOLVER_USING:

=================================================
Using Library Elements within User-Defined Graphs
=================================================

It is recommended to include the L2 (graph) library element in your design, i.e., a subgraph. For example, to include Cholesky, include ``cholesky_graph.hpp`` from ``L2/include/aie/``. The test harness for each library unit illustrates how to instantiate a parameterized graph; see ``L2/tests/aie/<library_element>/test.hpp`` and ``test.cpp``.

Set the environment variable to SOLVERLIB_ROOT.

.. code-block::

    setenv SOLVERLIB_ROOT <your-vitis-libraries-install-path/solver>

.. note::

   Use setenv for csh and export SOLVERLIB_ROOT=<path> for bash.

Use the following option in the aiecompiler command to provide the path:

.. code-block::

    -include=$SOLVERLIB_ROOT/L2/include/aie/
    -include=$SOLVERLIB_ROOT/L1/include/aie
    -include=$SOLVERLIB_ROOT/L1/src/aie

.. _SOLVER_KNOWN_ISSUES:

============
Known Issues
============

See Answer Record `75802 <https://www.xilinx.com/support/answers/75802.html>`__ for a list of known issues.


.. _SOLVER_TUTORIALS:

===============
Vitis Tutorials
===============

AMD provides an extensive library of purpose-built tutorials. Visit `Vitis Tutorials <https://github.com/Xilinx/Vitis-Tutorials>`__ to get familiar with the AMD Vitis |trade| in-depth tutorials.

To learn how to use the Vitis core tools to develop for AMD Versal |trade|, the first Adaptive SoC, visit `AI Engine Development Tutorials <https://github.com/Xilinx/Vitis-Tutorials/tree/HEAD/AI_Engine_Development>`__.

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
