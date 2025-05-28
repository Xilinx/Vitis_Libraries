
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

	• Updates:
    		• Corrected ISP pipeline API in documentation
    
**AIE additions/enhancements:** :

	• New Functions:
		• Added Bicubic-resize function	    

	• Updates:
		• Minor bug fixes

.. _known-issues:

Known issues
==============
 
	• AMD Vitis™ GUI projects on RHEL83 and CEntOS82 may fail because of a lib conflict in the LD_LIBRARY_PATH setting. You need to remove ${env_var:LD_LIBRARY_PATH} from the project environment settings for the function to build successfully.
	• rgbir2bayer, isppipeline_rgbir PL functions are not supplied with input images.
	• lkdensepyroptflow fails to meet timing when URAM is enabled.
