
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
		• Added new functions in ISP-Multistream pipeline
		• Added NPPC 2,4,8 support for RGBIR function
		• Added URAM support for AWB, AEC, 3DLUT, Otsu-Threshold, HDRMerge, Histogram, Equalization
		• Improved performance and utilization for ISP Stats
		• Fixed the missing template parameters issue in 'axiStrm2xfMat' and 'xfMat2axiStrm' functions

	• Lib Infra Changes:
		• Renamed all existing testcases and added new cases in tests directory of L1, L2.
		• Replaced 'xf_<algoName>_config.h' with 'xf_<algoName>_accel_config.h', 'xf_<algoName>_tb_config.h' 
			files which are included in 'accel.cpp' and 'tb.cpp' respectively.
		• All configurable parameters moved to 'xf_config_params.h'
		• Renamed 'build' folder in the function directories under 'examples' directory to 'config'
		• Standardized few variable names across 'accel' and 'testbench' files
		    
**AIE additions/enhancements:** :

	• Updates:
		• Improved RTL Data movers 
		• Miscellaneous bug fixes

.. _known-issues:

Known issues
==============
 
	• Vitis GUI projects on RHEL83 and CEntOS82 may fail because of a lib conflict in the LD_LIBRARY_PATH setting. User needs to remove ${env_var:LD_LIBRARY_PATH} from the project environment settings for the function to build successfully.
	• rgbir2bayer, isppipeline_rgbir PL functions are not supplied with input images
	• lkdensepyroptflow fails to meet timing when URAM is enabled
	• AWB, ISPStats output will not match with reference only on VCK190, because of a known XRT issue





















