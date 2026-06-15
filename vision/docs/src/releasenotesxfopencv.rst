
.. meta::
   :keywords: New features
   :description: Release notes.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _releasenotes-xfopencv:

.. 
   Copyright 2025 Advanced Micro Devices, Inc
  
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

Release notes
##############

This section describes new features, changes to the existing library, and known issues.

-  `New features and functions <#pl-new>`_
-  `Known issues <#known-issues>`_

.. _pl-new:

**PL additions/enhancements:** :

	• New additions:
		• Preprocess
		• Layout Formatter
		• Stitch
		• waitMat
		• Remap bicubic mode
		• AVFIRs L3 pipeline
		
	• Updates:
		• Remap functionality updated to support different input and output sizes.
		• InitUndistorRectofyMapInverse function API updated to use float datatypes.
		• Fixed isp stats bug of x and y index swap.
		• Minor bugs and doc fixes

**AIE additions/enhancements:** :

	• Updates:
		• Bug fixes

.. _known-issues:

Known issues
===============
 
	• Few AIE-ML testcases take a long time to finish hardware emulation because of large input size.
	• ``rgbir2bayer`` and ``isppipeline_rgbir`` PL functions are not supplied with input images.
	• ``lkdensepyroptflow`` fails to meet timing when URAM is enabled.
	• ``lkdensepyrof_uram``, ``tonemapping``, ``meanstddev-pipeline``, ``hls2rgb aiesim``, ``stereo-pipeline-URAM``, 
  		cases fail hw_emu because of a known tool issue. Other targets work fine.
