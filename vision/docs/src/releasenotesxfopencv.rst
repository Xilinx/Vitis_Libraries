
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
		    • HDR Decompanding : Compress(compand) data in a piece-wise linear (PWL) mapping to a lower bit depth
		    • Degamma : Designed to linearize the input from sensor or any pre-processing IP
		    • ISPStats : collects histogram based stats of bayer and color images
		    • ISP all-in-one pipeline : All the ISP related functions stitched in one pipeline with option to exclude unwanted functions during runtime and compile time.
		    • Multi-stream ISP : Multiple input stream ISP pipeline
    • Updates:
		    • Added new template parameter XFCVDEPTH for xf::cv::Mat class that can be used to assign custom depth to the Mat's internal hls::stream.
		    • All APIs in the library updated with newly added XFCVDEPTH parameter for xf::cv::Mat
		    • Remove deprecated __SDSVHLS__ macro from all files
		    • Replaced deprecated RESOURCE pragma with BIND_STORAGE/BIND_OP pragmas
		    • Rename NO, RO in all files to SPC (Single Pixel per Clock) and MPC (Multiple Pixels per Clock)
		    • Add missing reference functions in L1, L2, L3 testbench files
		    • Fixed Gaussian Difference incorrect implementation
		    • Fixed incorrect dst Mat assignment in xf::cv::Mat member function convertBitdepth
		    • Updated analyzeDiff in L1/include/common/xf_sw_utils.hpp to a static function
		    • Added missing "Test Passed/Failed/Finished" check in all L1/L2/L3 functions.
		    • Added 16 bit and 4 channel support, corrected B and R channel swap issue for channel extract function.
		    • Fixed a bug in BGR2HLS module of cvtcolor function
		    • Restructured L1 channel combine accel and testbench code
		    • Fixed SVM emulation and cosim hang issue
		    • Updated loop tripcounts of pyrDown, histogram, HDR extract, rgb2yuyv module in cvtColor to fix synthesis latency numbers
		    • Fix array reshape pragma in xf_sobel.hpp, xf_video_mem.hpp files
			• Modified XFCVDEPTH values in all functions
		    • Stride support added in the preprocess kernel of L3 Defect Detection pipeline
		    • Order of kernels changed in all-in-one pipeline and renamed as all-in-one-adas
		    • Array partitions in accel file moved to kernel file in isp multistream pipeline
	
    • Lib Infra Changes:
		    • Added frequency setting in L2/L3 JSON files. 300 MHz for NPPC1 and 150MHz for NPPC8 for most cases.
		    • Updated JSON and Makefiles to use ps_on_x86 feature for software emulation targeting embedded platforms. Software emulation for embedded platforms no longer uses qemu but regular g++ compilation flow only.
		    • Added missing environment checks in all JSON and Makefiles
		    
**AIE additions/enhancements:** :

    • New functions/features :
    	• Resize / Resize + Normalize
    	• Smart tiling for x86 64-bit platforms
	
    • Updates:
    	• RTL Data movers 
		- 8-bit PL / 8-bit AIE data movers
		- Multi-channel support
		- Optimized implementation
    	• Optimized smart tiling / stitching for higher performance
    	• Fix Random crashes in hardware emulation flow
    	• Miscellaneous bug fixes

.. _known-issues:

Known issues
==============
 
    • Vitis GUI projects on RHEL83 and CEntOS82 may fail because of a lib conflict in the LD_LIBRARY_PATH setting. User needs to remove ${env_var:LD_LIBRARY_PATH} from the project environment settings for the function to build successfully.
    • rgbir2bayer, isppipeline_rgbir PL functions are not supplied with input images
    • Software emulation for Warptransform L2 testcases doesn't work because of a known issue with platform.
    • Warptransform L1 URAM cases fail CSim because of a known HLS issue.    





















