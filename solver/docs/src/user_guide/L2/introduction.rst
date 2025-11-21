..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _INTRODUCTION:

***********************************
Introduction to AIE Solver Library
***********************************

The Solver library for AI Engine provides a set of linear-algebra library elements. Each library element consists of a main graph class and corresponding kernel classes; it also includes reference-model graph and kernel classes.

====================================
Navigating Content by Design Process
====================================

AMD documentation is organized around a set of standard design processes to help you find relevant content for your current development task. This document covers the following design processes:

-  **AI Engine Development:** Creating the AI Engine graph and kernels, library use, simulation debugging and profiling, and algorithm development. Also includes the integration of the programmable logic (PL) and AI Engine kernels. Topics in this document that apply to this design process include:

   -  :ref:`ORGANIZATION`

   -  :ref:`USING`

   -  :ref:`KNOWN_ISSUES`

   -  :ref:`TUTORIALS`

   -  :ref:`SOLVER_LIB_FUNC`

   -  :ref:`COMPILING_AND_SIMULATING`

   -  :ref:`API_REFERENCE`

   -  :ref:`BENCHMARK`


-  **System and Solution Planning:** Identifying the components, performance, I/O, and data transfer requirements at a system level. Includes application mapping for the solution to the processing subsystem (PS), PL, and AI Engine. Topics in this document that apply to this design process include:

   -  :ref:`CHOLESKY`

   -  :ref:`QRD`

-  **System Integration and Validation:** Integrating and validating the system functional performance, including timing, resource use, and power closure. Topics in this document that apply to this design process include:

   -  :ref:`COMPILING_AND_SIMULATING`

   -  :ref:`API_REFERENCE`

.. _ORGANIZATION:

============
Organization
============

The following figure shows the SolverLib organization.

**SolverLib Organization**

.. graphviz::

   digraph solver {
       rankdir=LR;
       node [shape=folder];
       solver -> docs;
       solver -> ext;
       solver -> L1;
       solver -> L2;
       solver -> scripts;

       L1 -> include;
       L1 -> meta;
       L1 -> src;
       L1 -> tests;

       include -> aie;
       include -> hw;

       src -> aie;

       tests -> aie;
       tests -> src;

       L2 -> benchmarks;
       L2 -> examples;
       L2 -> include;
       L2 -> meta;
       L2 -> tests;

       examples -> docs_examples;

       include [label="include"];
       include -> aie [label="aie"];
       include -> hw [label="hw"];

       L2_include [label="include"];
       L2_include -> aie [label="aie"];
       L2_include -> hw [label="hw"];
       L2 -> L2_include [style=invis];

       tests -> aie [label="aie"];
       tests -> hw [label="hw"];
   }



The directories L1 and L2 correspond to the AI Engine kernels and AI Engine graphs for each function, respectively. Inclusion of an L2 graph rather than an L1 element is recommended in your design. L3 is reserved for future software drivers.

.. note::

   The L3 directory is not yet available.

Graph class declarations and constants that allow you to include the library element in your design are located in `L2/include/aie/`. Kernel class definitions, the `.cpp` files and corresponding `.hpp` files are located in the `L1/src/aie` and `L1/include/aie` subdirectories respectively.

The ``L2/tests/aie/<library_element>`` subdirectory contains a testbench for the library element. Additional testbench files, such as stimulus, monitor, and other utility modules, are located in the ``L1/tests/aie/inc/`` folder.

Reference model graph classes for each library element are contained in ``L2/tests/aie/common/inc``. Reference model kernel classes for each library element are contained in ``L1/tests/aie/inc/`` and ``L1/tests/aie/src``.

The `L2/examples` subdirectory holds example wrapper designs to demonstrate the use of the library elements.

.. _USING:

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

.. _KNOWN_ISSUES:

============
Known Issues
============

See Answer Record `75802 <https://www.xilinx.com/support/answers/75802.html>`__ for a list of known issues.


.. _TUTORIALS:

===============
Vitis Tutorials
===============

AMD provides an extensive library of purpose-built tutorials. Visit `Vitis Tutorials <https://github.com/Xilinx/Vitis-Tutorials>`__ to get familiar with the AMD Vitis |trade| in-depth tutorials.

To learn how to use the Vitis core tools to develop for AMD Versal |trade|, the first Adaptive SoC, visit `AI Engine Development Tutorials <https://github.com/Xilinx/Vitis-Tutorials/tree/HEAD/AI_Engine_Development>`__.

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
