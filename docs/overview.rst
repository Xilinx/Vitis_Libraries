.. _xfopencvlibug:

##################################
xfOpenCV Library User Guide
##################################


.. _overview:

Overview
========

This document describes the FPGA device optimized xfOpenCV library,
called the Xilinx® xfOpenCV library and is intended for application
developers using Zynq®-7000 SoC and Zynq® UltraScale+™ MPSoC and PCIE
based (Virtex and U200 ...) devices. xfOpenCV library has been designed
to work in the SDx™ development environment, and provides a software
interface for computer vision functions accelerated on an FPGA device.
xfOpenCV library functions are mostly similar in functionality to their
OpenCV equivalent. Any deviations, if present, are documented.

Note: For more information on the xfOpenCV library prerequisites, see
the `Prerequisites <getting-started-with-sdsoc.html#gyt1504034261161>`__. To
familiarize yourself with the steps required to use the xfOpenCV library
functions, see the `Using the xfOpenCV
Library <getting-started-with-sdsoc.html#xne1504034261508>`__.

.. _basic-features:

Basic Features
--------------

All xfOpenCV library functions follow a common format. The following
properties hold true for all the functions.

-  All the functions are designed as templates and all arguments that
   are images, must be provided as ``xf::Mat``.
-  All functions are defined in the ``xf`` namespace.
-  Some of the major template arguments are:

   -  Maximum size of the image to be processed
   -  Datatype defining the properties of each pixel
   -  Number of pixels to be processed per clock cycle
   -  Other compile-time arguments relevent to the functionality.

The xfOpenCV library contains enumerated datatypes which enables you to
configure ``xf::Mat``. For more details on ``xf::Mat``, see the `xf::Mat
Image Container Class <api-reference.html#owg1504034262157>`__.

.. _xfopencv-kernel:

xfOpenCV Kernel on the reVISION Platform
----------------------------------------

The xfOpenCV library is designed to be used with the SDx development
environment. xfOpenCV kernels are evaluated on the reVISION platform.

The following steps describe the general flow of an example design,
where both the input and the output are image files.

#. Read the image using ``cv::imread()``.
#. Copy the data to ``xf::Mat``.
#. Call the processing function(s) in xfOpenCV.
#. Copy the data from ``xf::Mat`` to ``cv::Mat``.
#. Write the output to image using ``cv::imwrite()``.

The entire code is written as the host code for the pipeline , from
which all the calls to xfOpenCV functions are moved to hardware.
Functions from xfOpenCV are used to read and write images in the memory.
The image containers for xfOpenCV library functions are ``xf::Mat``
objects. For more information, see the `xf::Mat Image Container
Class <api-reference.html#owg1504034262157>`__.

The reVISION platform supports both live and file input-output (I/O)
modes. For more details, see the `reVISION Getting Started
Guide <http://www.wiki.xilinx.com/reVISION+Getting+Started+Guide>`__.

-  File I/O mode enables the controller to transfer images from SD Card
   to the hardware kernel. The following steps describe the file I/O
   mode.

   #. Processing system (PS) reads the image frame from the SD Card and
      stores it in the DRAM.
   #. The xfOpenCV kernel reads the image from the DRAM, processes it
      and stores the output back in the DRAM memory.
   #. The PS reads the output image frame from the DRAM and writes it
      back to the SD Card.

-  Live I/O mode enables streaming frames into the platform, processing
   frames with the xfOpenCV kernel, and streaming out the frames through
   the appropriate interface. The following steps describe the live I/O
   mode.

   #. Video capture IPs receive a frame and store it in the DRAM.
   #. The xfOpenCV kernel fetches the image from the DRAM, processes the
      image, and stores the output in the DRAM.
   #. Display IPs read the output frame from the DRAM and transmits the
      frame through the appropriate display interface.

Following figure shows the reVISION platform with the xfOpenCV kernel
block:

.. figure:: ./images/asl1554997006054.png
   :alt: 
   :figclass: image
   :name: chg1504034262720__image_vxy_pqn_h1b

Note: For more information on the PS-PL interfaces and PL-DDR
interfaces, see the Zynq UltraScale+ Device Technical Reference Manual
(`UG1085 <https://www.xilinx.com/cgi-bin/docs/ndoc?t=user_guides;d=ug1085-zynq-ultrascale-trm.pdf>`__).

.. _xfopencv-lib-contents:

xfOpenCV Library Contents
-------------------------

The following table lists the contents of the xfOpenCV library.

.. table:: Table 1. xfOpenCV Library Contents

+-----------------------------------+-----------------------------------+
| Folder                            | Details                           |
+===================================+===================================+
| include                           | Contains the header files         |
|                                   | required by the library.          |
+-----------------------------------+-----------------------------------+
| include/common                    | Contains the common library       |
|                                   | infrastructure headers, such as   |
|                                   | types specific to the library.    |
+-----------------------------------+-----------------------------------+
| include/core                      | Contains the core library         |
|                                   | functionality headers, such as    |
|                                   | the ``math`` functions.           |
+-----------------------------------+-----------------------------------+
| include/features                  | Contains the feature extraction   |
|                                   | kernel function definitions. For  |
|                                   | example, ``Harris``.              |
+-----------------------------------+-----------------------------------+
| include/imgproc                   | Contains all the kernel function  |
|                                   | definitions, except the ones      |
|                                   | available in the features folder. |
+-----------------------------------+-----------------------------------+
| include/video                     | Contains all the kernel function  |
|                                   | definitions, except the ones      |
|                                   | available in the features and     |
|                                   | imgproc folder.                   |
+-----------------------------------+-----------------------------------+
| examples                          | Contains the sample test bench    |
|                                   | code to facilitate running unit   |
|                                   | tests. The examples/ folder       |
|                                   | contains the folders with         |
|                                   | algorithm names. Each algorithm   |
|                                   | folder contains host files, .json |
|                                   | file, and data folder. For more   |
|                                   | details on how to use the         |
|                                   | xfOpenCV library, see xfOpenCV    |
|                                   | Kernel on the reVISION            |
|                                   | Platform <overview.html#c         |
|                                   | hg1504034262720>.                 |
+-----------------------------------+-----------------------------------+
| examples                          | Contains the sample test bench    |
|                                   | code for 24 functions, which      |
|                                   | shows how to use xfOpenCV library |
|                                   | in SDAccel™ environment.          |
+-----------------------------------+-----------------------------------+
| HLS_Use_Model                     | Contains examples for using       |
|                                   | xfOpenCV functions in Standalone  |
|                                   | Vivado HLS in 2 different modes.  |
+-----------------------------------+-----------------------------------+
| HLS_Use_Model/Standalone_HLS_Exam | Contains sample code and Tcl      |
| ple                               | script for synthesizing xfOpenCV  |
|                                   | functions as is, in standalone    |
|                                   | Vivado HLS tool.                  |
+-----------------------------------+-----------------------------------+
| HLS_Use_Model/Standalone_HLS_AXI_ | Contains sample code and Tcl      |
| Example                           | script for synthesizing functions |
|                                   | with AXI interfaces, in           |
|                                   | standalone Vivado HLS tool.       |
+-----------------------------------+-----------------------------------+

.. include:: getting-started-with-sdsoc.rst
.. include:: getting-started-with-sdaccel.rst
.. include:: getting-started-with-hls.rst
.. include:: design-examples.rst 
