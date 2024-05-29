.. meta::
   :keywords: Vision, Library, Vitis Vision AIE Library, overview, features, kernel
   :description: Using the Vitis Vision AIE library.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. 
   Copyright 2024 Advanced Micro Devices, Inc
  
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _overview_aie:

Overview 
#########
 
AMD Vitis™ Vision library added some functions which are implemented on the AI Engine of AMD Versal Adaptive SoC devices and validated on `VEK280`_ board. These implementations exploit the VLIW, SIMD vector processing capabilities of `AI Engines`_.  

.. _AI Engines: https://docs.amd.com/r/en-US/ug1076-ai-engine-environment/Overview

.. _basic-features-aie:

Basic Features
===============
To process high resolution images, the xfcvDataMovers component is provided. This divides the image into tiled units and uses efficient data-movers to manage the transfer of tiled units to and from AI Engine cores. You can find more information on the types of data-movers and their usage, in the :ref:`Getting Started with Vitis Vision AIEngine Library Functions <aie_prerequisites>` section.  


Vitis Vision AIE Library Contents
==================================

Vitis Vision AIEngine files are organized into the following directories: 

.. table:: Table: Vitis Vision AIE Library Contents

   +----------------------------------------------------------+---------------------------------------------------------------------------------------------+
   | Folder                                                   | Details                                                                                     |
   +==========================================================+=============================================================================================+
   | L1/include/aie-ml/imgproc                                | contains header files of Vision AIEngine™ functions                                         |
   +----------------------------------------------------------+---------------------------------------------------------------------------------------------+
   | L1/include/aie/common                                    | contains header files of data-movers and other utility functions                            |
   +----------------------------------------------------------+---------------------------------------------------------------------------------------------+
   | L1/lib/sw                                                | contains the data-movers library object files                                               |
   +----------------------------------------------------------+---------------------------------------------------------------------------------------------+
   | L2/tests/aie-ml                                          | contains the ADF graph code and host-code using data-movers and Vision AIEngine™ functions  |
   |                                                          | from L1/include/aie-ml                                                                      |
   +----------------------------------------------------------+---------------------------------------------------------------------------------------------+
   
   
.. include:: include/getting-started-with-vitis-vision-aie.rst
.. include:: include/vitis-aie-design-methodology.rst
.. include:: include/functionality-evaluation-aie.rst
.. include:: include/design-example-aie.rst
