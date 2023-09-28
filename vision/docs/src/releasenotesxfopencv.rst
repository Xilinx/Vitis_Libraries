
.. meta::
   :keywords: New features
   :description: Release notes.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _releasenotes-xfopencv:


Release notes
##############

The below section explains the new features added and also changes in the existing library along with the known issues.

-  `New features and functions <#pl-new>`_
-  `Known issues <#known-issues>`_

.. _pl-new:

New features and functions
============================

**PL additions/enhancements**:
	
	• New functions:
		• Added 24 bits-per-channel L3 ISP pipeline	
		• Added all-in-one L3 ISP pipeline
		• Pin-cushion, Barrel distortion support added in L1, L2 Remap testbench.

	• Updates:
		• Added reference functions for extractExposureFrames, autoexposurecorrection_sin, LTM, bgr2yuyv.
		• Added reference function for all-in-one L3 ISP pipeline.
		• Fixed border rows issue in Bad Pixel Correction function.
		• Fixed divide-by-zero condition in GTM.
		• Improved accuracy of AWB and AEC functions.
		• Optimized resource utilization of 3DLUT function.	
		• Other minor bug fixes.
		
	• Lib Infra Changes:
		
		• Added L1 api.json to autofill the function APIs in Vitis HLS GUI.
		• Updated APIs in L2 api.json
		• Updated Makefiles of L1 examples and testcases to a new template.
		    
**AIE additions/enhancements:** :

	• Updates:
		• Host code of all AIE1 cases modified to use new graph coding methodology.

.. _known-issues:

Known issues
==============
 
	• Vitis GUI projects on RHEL83 and CEntOS82 may fail because of a lib conflict in the LD_LIBRARY_PATH setting. User needs to remove ${env_var:LD_LIBRARY_PATH} from the project environment settings for the function to build successfully.
	• rgbir2bayer, isppipeline_rgbir PL functions are not supplied with input images
	• lkdensepyroptflow fails to meet timing when URAM is enabled
	• AWB, ISPStats output will not match with reference only on VCK190, because of a known XRT issue





















