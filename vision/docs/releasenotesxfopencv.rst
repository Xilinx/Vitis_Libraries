
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
		- Rotate
		- TV-L1 optical flow
		- Multi-stream ISP (basic) support
    • Updates:
		- Added Demosaicing kernel (xf_demosaicing_rt.hpp) having the input Bayer pattern as run time parameter
    • Lib Infra Changes:
		- Added API JSON for L2, that helps in usage of a given function's API in Vitis GUI	

**AIE additions/enhancements:**
    • Updates:
		- Introduced RTL Data-movers with improved latency over HLS data-movers
		- All tests updated with RTL data-movers

.. _known-issues:

Known issues
==============
**`** 
    • Vitis GUI projects on RHEL83 and CEntOS82 may fail because of a lib conflict in the LD_LIBRARY_PATH setting. User needs to remove ${env_var:LD_LIBRARY_PATH} from the project environment settings for the function to build successfully.
    • SVM L2 PL function fails hardware emulation with 2022.1 Vitis. Please use 2021.1 Vitis for this function.
    • rgbir2bayer, isppipeline_rgbir PL functions are not supplied with input images
    • Software emulation for Warptransform L2 testcases doesn't work because of a known issue with platform.
    • Warptransform L1 URAM cases fail CSim because of a known HLS issue.    
    • Hardware emulation in AIE testcases may throw segmentation fault at the end, although completing the functional test successfully.





















