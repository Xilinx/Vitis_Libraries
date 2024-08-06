
.. meta::
   :keywords: New features
   :description: Release notes.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _releasenotes-xfopencv:

.. 
   Copyright 2024 Advanced Micro Devices, Inc
  
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

Release notes
##############

This section describes new features, changes to the existing library, and known issues.

-  `New features and functions <#pl-new>`_
-  `Known issues <#known-issues>`_

.. _pl-new:

New Features and Functions
============================

**PL additions/enhancements**:
	
	• Updates:
		• Updated ISP Pipeline example in L1 to support runtime reconfiguration
		• Added ISP Mono example in L1
		• Updated runtime configurable support to color-correction-matrix function
		• Updated bayer-pattern as runtime parameter for demosaicing, gaincontrol functions
		• Added green-gain as runtime parameter for gaincontrol function
		• Fixed a bug in ISP Pipeline L1 example for 10 bit input		
		
	• Lib Infra Changes:
		
		• Minor fixes to API JSON file
		• Documentation format fix
		    
**AIE additions/enhancements:** :

	• New Functions:
		• Added 23 new AIE-ML functions (PLIO & GMIO) targeting Versal devices:
			• AccumulateWeighted
			• AWB-Norm-CCM			
			• Blacklevel
    			• Demosaic
    			• Denorm_resize (no GMIO)
    			• Denormalize		
    			• Filter2D
    			• Gain Control
    			• Hybrid ISP		
			• Mask Generation
    			• Mask Generation Tracking (no GMIO)
    			• Normalize
    			• NMS (no PLIO)
    			• PixelWise Select
    			• Resize (no GMIO)
    			• Resize Nomalize (no GMIO)
    			• RGBA2GRAY
    			• RGBA2YUV
    			• Transpose
    			• TopK
    			• Threshold		
    			• YUV2RGBA
    			• YUY2-Filter2d	(no GMIO)
	• Updates:
		• Minor bug fixes
		• Removed AIE1 functions


.. _known-issues:

Known issues
==============
 
	• AMD Vitis™ GUI projects on RHEL83 and CEntOS82 may fail because of a lib conflict in the LD_LIBRARY_PATH setting. You need to remove ${env_var:LD_LIBRARY_PATH} from the project environment settings for the function to build successfully.
	• rgbir2bayer, isppipeline_rgbir PL functions are not supplied with input images.
	• lkdensepyroptflow fails to meet timing when URAM is enabled.
