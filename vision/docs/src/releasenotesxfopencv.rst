
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

**PL additions/enhancements:** :

	• New Functions:
		• Added autogain function
		• Created new L3 example of ISP with autogain
		
	• Updates:
    		• Added NPC2 support for Remap function
    		• Corrected Preprocess API in documentation
    		• Updated output format of initUndistoredRectifyMapInverse to 32SC1 (fixed point). Updated L3 stereopipeline example accordingly.
    
**AIE additions/enhancements:** :

	• New Functions:
		• Added NV12-resize function	    

	• Updates:
		• Added g-gain parameter in gain correction. Updated Hybrid-ISP in L3 accordingly.
    		• Fixed yuy2-filter2d GMIO hw-emu hang issue
    		• Fixed TopK hang issue
    		• Updated computeMetadata function in GMIO datamovers
		• Minor bug fixes

.. _known-issues:

Known issues
==============
 
	• AMD Vitis™ GUI projects on RHEL83 and CEntOS82 may fail because of a lib conflict in the LD_LIBRARY_PATH setting. You need to remove ${env_var:LD_LIBRARY_PATH} from the project environment settings for the function to build successfully.
	• rgbir2bayer, isppipeline_rgbir PL functions are not supplied with input images.
	• lkdensepyroptflow fails to meet timing when URAM is enabled.
