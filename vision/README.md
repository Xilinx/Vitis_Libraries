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
The Vitis Vision library is designed to work with Zynq, Zynq Ultrascale+, and Alveo FPGAs. The library has been verified on zcu102, zcu104 and U200 boards.

Vitis 2020.1 Development Environment is required to work with the library.

**Vitis Flow:**

U200 platform, available in the Vitis tool, is required to build and run the library functions on U200 PCIe board. Same applies for Zynq based platforms.

## OTHER INFORMATION
Full User Guide for Vitis Vision and using OpenCV on Xilinx devices Check here:
[Xilinx Vitis Vision User Guide](https://xilinx.github.io/Vitis_Libraries/vision/2020.1/index.html)

## SUPPORT
For questions and to get help on this project or your own projects, visit the [Xilinx Forums][] (link yet to be updated).

## LICENSE AND CONTRIBUTING TO THE REPOSITORY
The source for this project is licensed under the [Apache License](http://www.apache.org/licenses/LICENSE-2.0)

To contribute to this project, follow the guidelines in the [Repository Contribution README][] (link yet to be updated)

## ACKNOWLEDGEMENTS
This library is written by developers at
- [Xilinx](http://www.xilinx.com)

## Changelog:
1. Added new functions:

    • Letter Box
	• ISP pipeline design example
	
Vitis HLS related changes:

2. The xf::cv::Mat class member 'data', which earlier used to be a pointer, has now been changed to an 'hls::stream' type.

3. An extra template parameter 'DEPTH' has been added to the xf::cv::Mat class, to specify the depth of above mentioned hls::stream. The default value is set to 2.

4. The 'read', 'write', 'read_float' and 'write_float' member functions that facilitate the data access of xf::cv::Mat also have been updated accordingly.

5. Enhanced Auto White Balance, Array2xfMat and xfMat2Array functions.

6. The L1 host functions targeting HLS flow have been updated to have pointers at the interface instead of xf::cv::Mat, similar to L2 functions. All the testbench and config files have been updated suitably.


Library related changes:

7. Updated all L2/L3 Makefiles to use images of smaller size for software and hardware emulations.

8. All JSON files have been updated to support auto creation of projects in Vitis GUI.

9. Makefiles and Json files have been moved out of the build folder in the examples directory to be along with the host source files.

10. Emulation flow for embedded devices have been updated in all the Makefiles to use a perl based script. Added the corresponding script under ext/make_utility folder.

11. The 'data' folders inside individual L1 examples have been deleted and all the input argumensts are now being provided from top level data folder in the Makefiles and Json files.

12. 2020.1 code base is not backward-compatible, i.e, all the functions in the library have to be built with 2020.1 Vitis/Vivado tools only. None of the functions in this release can be used with any of the previous versions of Vitis/Vivado.


#### Known Issues:
1. Windows OS has path length limitations, kernel names must be smaller than 25 characters.
  
