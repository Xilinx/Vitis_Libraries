
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
		• 3D depth
		• 3D point cloud
		• New L3 pipeline examples
		
	• Updates:
    		• Fixed isp stats bug of x and y index swap.
    		• Minor bug fixes

**AIE additions/enhancements:** :

	• New additions:
		• hls2rgb
		• hsv2rgba
		• mean_rgb888
		• mean_yuv400
		• nv12-resize
		• polyphase resize
		• bicubic resize
		• resize-yuv420
		• resize-yuv422
		• resize-yuv444
		• rgb2hls
		• rgb2ycrcb
		• rgba2hsv
		• rgba2rgb
		• stdev-rgb888
		• stddev-yuv400
		• ycrcb2rgb
	• Updates:
		• Bug fixes

.. _known-issues:

Known issues
===============
 
	• Few AIE-ML testcases take a long time to finish hardware emulation because of large input size.
	• ``rgbir2bayer`` and ``isppipeline_rgbir`` PL functions are not supplied with input images.
	• ``lkdensepyroptflow`` fails to meet timing when URAM is enabled.
	• AWB-npc8, customconv-npc8, lkdensepyrof_uram, tonemapping, meanstddev-pipeline, hls2rgb, rgb2hls, cases fail hw_emu because of a known tool issue. Other targets work fine.
