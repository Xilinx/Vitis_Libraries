# Vitis Vision Library
The Vitis Vision library is a set of 60+ kernels, optimized for Xilinx FPGAs and SoCs, based on the OpenCV computer vision library. The kernels in the Vitis Vision library are optimized and supported in the Xilinx Vitis Tool Suite.

# DESIGN FILE HIERARCHY
The library is organized into the following folders -

| Folder Name | Contents |
| :------------- | :------------- |
| L1 | Examples that evaluate the Vitis Vision kernels, and demonstrate the kernels' use model in HLS flow |
| L2 | Examples that evaluate the Vitis Vision kernels, and demonstrate the kernels' use model in Vitis flow.  |
| L3 | Applications formed by stitching a pipeline of Vitis Vision functions |
| ext | Utility functions used in the opencl host code |
| data | Input images required to run examples and tests |

The organization of contents in each folder is described in the readmes of the respective folders.

## HARDWARE and SOFTWARE REQUIREMENTS
The Vitis Vision library is designed to work with Zynq, Zynq Ultrascale+, and Alveo FPGAs. The library has been verified on zcu102, zcu104, U50, and U200 boards.

Vitis 2020.2 Development Environment is required to work with the library.

**Vitis Flow:**

U200 platform, available in the Vitis tool, is required to build and run the library functions on U200 PCIe board. Same applies for U50 and Zynq based platforms.

## OTHER INFORMATION
Full User Guide for Vitis Vision and using OpenCV on Xilinx devices Check here:
[Xilinx Vitis Vision User Guide](https://xilinx.github.io/Vitis_Libraries/vision/2020.2/index.html)

## SUPPORT
For questions and to get help on this project or your own projects, visit the [Xilinx Forums](https://forums.xilinx.com/t5/Vitis-Acceleration-SDAccel-SDSoC/bd-p/tools_v)

## LICENSE AND CONTRIBUTING TO THE REPOSITORY
The source for this project is licensed under the [Apache License](http://www.apache.org/licenses/LICENSE-2.0)

To contribute to this project, follow the guidelines in the [Repository Contribution README][] (link yet to be updated)

## ACKNOWLEDGEMENTS
This library is written by developers at
- [Xilinx](http://www.xilinx.com)

## Changelog:

**New Functions and Features**

    • 2020.2 ISP Pipeline supports pixel depth upto 16 bit
	
    • Local tone mapping
	
    • Auto Exposure Correction
	
    • Quantization & Dithering
	
    • Color Correction Marix
	
    • Black Level Correction
	
    • Lens Shading correction
	
    • Brute Force Feature Matching
	
    • Mode Filter
	
    • blobFromImage
	
    • Laplacian Operator
	
    • Distance Transform
	
**Library Related Changes**

    • U50 support of all functions
	
    • GUI support for both edge and DC functions.
	
    • Color Conversion : supporting RGBX or fourth channel support
	
    • Line Stride support in Data Converters
	
	• Removed xf_axi_sdata.hpp file. Axiconverter functions now use the HLS ap_axi_sdata.h file instead.
	
**New Apps in Xilinx App Store**

    • Image Classification using ML-inference engine from Vitis AI Library and Vitis Vision Pre Processing Function
	
    • ISP Pipeline (2020.1 version)
	
    • Stereo Block Matching

#### Known Issues:
1. Windows OS has path length limitations, kernel names must be smaller than 25 characters.
2. 'axiconv' function in L1 fails cosim because of a known HLS issue.
3. 'bilateralfilter' function RO configuration is not supported.