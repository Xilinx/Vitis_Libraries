.. _overview:

Overview
########

This document describes the FPGA device optimized Vitis vision library,
called the Xilinx® Vitis vision library and is intended for application
developers using Zynq®-7000 SoC and Zynq® UltraScale+™ MPSoC and PCIE
based (Virtex and U200 ...) devices. Vitis vision library has been designed
to work in the Vistis development environment, and provides a software
interface for computer vision functions accelerated on an FPGA device.
Vitis vision library functions are mostly similar in functionality to their
OpenCV equivalent. Any deviations, if present, are documented.

Note: For more information on the Vitis vision library prerequisites, see
prerequisites_. To
familiarize yourself with the steps required to use the Vitis vision library
functions, see the Using the `Vitis vision
Library <getting-started-with-sdsoc.html>`__.

.. _basic-features:

Basic Features
===============

All Vitis vision library functions follow a common format. The following
properties hold true for all the functions.

-  All the functions are designed as templates and all arguments that
   are images, must be provided as ``xf::cv::Mat``.
-  All functions are defined in the ``xf::cv`` namespace.
-  Some of the major template arguments are:

   -  Maximum size of the image to be processed
   -  Datatype defining the properties of each pixel
   -  Number of pixels to be processed per clock cycle
   -  Other compile-time arguments relevent to the functionality.

The Vitis vision library contains enumerated datatypes which enables you to
configure ``xf::cv::Mat``. For more details on ``xf::cv::Mat``, see the `xf::cv::Mat
Image Container Class <api-reference.html>`__.

.. _xfopencv-kernel:

Vitis Vision Kernel on Vitis
============================

The Vitis vision library is designed to be used with the Vitis development
environment. 

The following steps describe the general flow of an example design,
where both the input and the output are image files.

#. Read the image using ``cv::imread()``.
#. Copy the data to ``xf::cv::Mat``.
#. Call the processing function(s) in Vitis vision.
#. Copy the data from ``xf::cv::Mat`` to ``cv::Mat``.
#. Write the output to image using ``cv::imwrite()``.

The entire code is written as the host code for the pipeline , from
which all the calls to Vitis vision functions are moved to hardware.
Functions from Vitis vision are used to read and write images in the memory.
The image containers for Vitis vision library functions are ``xf::cv::Mat``
objects. For more information, see the `xf::cv::Mat Image Container
Class <api-reference.html>`__.

.. _xfopencv-lib-contents:

Vitis Vision Library Contents
==============================

The following table lists the contents of the Vitis vision library.

.. table::  Vitis Vision Library Contents

	+-----------------------------------+-----------------------------------+
	| Folder                            | Details                           |
	+===================================+===================================+
	| L1/examples                       | Contains the sample testbench code|
	|                                   | to facilitate running unit tests. |
	|                                   | The examples/ contains the folders|
	|                                   | with algorithm names.Each algorith|
	|                                   | m folder contains testbench files |
	|                                   | data folder.                      |
	+-----------------------------------+-----------------------------------+
	| L1/include/common                 | Contains the common library       |
	|                                   | infrastructure headers, such as   |
	|                                   | types specific to the library.    |
	+-----------------------------------+-----------------------------------+
	| L1/include/core                   | Contains the core library         |
	|                                   | functionality headers, such as    |
	|                                   | the ``math`` functions.           |
	+-----------------------------------+-----------------------------------+
	| L1/include/features               | Contains the feature extraction   |
	|                                   | kernel function definitions. For  |
	|                                   | example, ``Harris``.              |
	+-----------------------------------+-----------------------------------+
	| L1/include/imgproc                | Contains all the kernel function  |
	|                                   | definitions related to image proce|
	|                                   | ssing definitions.                |
	+-----------------------------------+-----------------------------------+
	| L1/include/video                  | Contains all the kernel function  |
	|                                   | definitions, related to video proc|
	|                                   | essing functions.eg:Optical flow  |
	+-----------------------------------+-----------------------------------+
	| L1/include/dnn                    | Contains all the kernel function  |
	|                                   | definitions, related to deep lea  |
	|                                   | rning preprocessing.              |
	+-----------------------------------+-----------------------------------+
	| L1/tests                          | Contains all test folders to run  |
	|                                   | simulations, synthesis and export |
	|                                   | RTL.The tests folder contains the |
	|                                   | folders with algorithm names.Each |
	|                                   | algorithm folder further contains |
	|                                   | configuration folders, that has   |
	|                                   | makefile and tcl files to run     |
	|                                   | tests.                            |
	+-----------------------------------+-----------------------------------+
	| L1/examples/build                 | Contains makefile, run_hls.tcl and|
	|                                   | xf_config_params.h file to run    |
	|                                   | simulations, synthesis and export |
	|                                   | RTL.                              |
	+-----------------------------------+-----------------------------------+
	| L2/examples                       | Contains the sample test bench    |
	|                                   | code to facilitate running unit   |
	|                                   | tests. The examples/ folder       |
	|                                   | contains the folders with         |
	|                                   | algorithm names. Each algorithm   |
	|                                   | folder contains host files        |
	|                                   | and data folder.                  |
	+-----------------------------------+-----------------------------------+
	| L2/tests                          | Contains all test folders to run  |
	|                                   | software, hardware emulations     |
	|                                   | and hardware build. The tests cont|
	|                                   | ains folders with algorithm names.|
	|                                   | Each algorithm folder further cont|
	|                                   | ains configuration folders, that  |
	|                                   | has makefile and tcl files to run |
	|                                   | tests.                            |
	+-----------------------------------+-----------------------------------+
	| L2/examples/build                 | Contains makefile and             |
	|                                   | xf_config_params.h file to run    |
	|                                   | software,hardware emulation and   |
	|                                   | hardware build                    |
	+-----------------------------------+-----------------------------------+
	| L3/examples                       | Contains the sample test bench    |
	|                                   | code to facilitate running unit   |
	|                                   | tests. The examples/ folder       |
	|                                   | contains the folders with         |
	|                                   | algorithm names. Each algorithm   |
	|                                   | folder contains host files        |
	|                                   | and data folder.The L3/examples   |
	|                                   | contains the pipeline examples usi|
	|                                   | ng L1 modules.                    |
	+-----------------------------------+-----------------------------------+
	| L3/tests                          | Contains all test folders to run  |
	|                                   | software, hardware emulations     |
	|                                   | and hardware build.The tests cont |
	|                                   | ains folders with algorithm names.|
	|                                   | Each algorithm name folder contai |
	|                                   | ns the configuration folders,     |
	|                                   | inside configuration folders      |
	|                                   | makefile is present to run tests. |
	+-----------------------------------+-----------------------------------+
	| L3/examples/build                 | Contains makefile and             |
	|                                   | xf_config_params.h file to run    |
	|                                   | software,hardware emulation and   |
	|                                   | hardware build.                   |
	+-----------------------------------+-----------------------------------+
	| L3/benchmarks                     | Contains benchmark examples to    |
	|                                   | compare the software              |
	|                                   | implementation versus FPGA        |
	|                                   | implementation using Vitis vision |
	|                                   | library.                          |
	+-----------------------------------+-----------------------------------+
	| ext                               | Contains the utility functions    |
	|                                   | related to opencl hostcode.       |
	+-----------------------------------+-----------------------------------+


.. include:: getting-started-with-vitis-vision.rst 
.. include:: getting-started-with-hls.rst
.. include:: migrating-hls-video-library-to-vitis-vision.rst 
.. include:: design-examples.rst 
