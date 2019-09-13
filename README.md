# Xilinx xfOpenCV Library
The xfOpenCV library is a set of 60+ kernels, optimized for Xilinx FPGAs and SoCs, based on the OpenCV computer vision library. The kernels in the xfOpenCV library are optimized and supported in the Xilinx SDx Tool Suite.

# DESIGN FILE HIERARCHY
The library is organized into the following folders -

| Folder Name | Contents |
| :------------- | :------------- |
| examples | Examples that evaluate the xfOpenCV kernels, and demonstrate the kernels' use model in SDSoC flow |
| examples_sdaccel | 24 examples that evaluate the xfOpenCV kernels, and demonstrate the kernels' use model in SDAccel flow. These examples serve as reference on how to use all other supported xfOpenCV kernels in SDAccel |
| include | The relevant headers necessary to use the xfOpenCV kernels |
| HLS_Use_model | Examples that evaluate the xfOpenCV kernels, and demonstrate the kernels' use model in Standalone Vivado HLS tool |

The organization of contents in each folder is described in the readmes of the respective folders.

For more information on the xfOpenCV libraries and their use models, please refer to the [Xilinx OpenCV User Guide][].

## HOW TO DOWNLOAD THE REPOSITORY
To get a local copy of the repository, clone this repository to the local system with the following command:
```
git clone https://github.com/Xilinx/xfopencv xfopencv
```
Where 'xfopencv' is the name of the directory where the repository will be stored on the local system.This command needs to be executed only once to retrieve the latest version of the xfOpenCV library. The only required software is a local installation of git.

## HARDWARE and SOFTWARE REQUIREMENTS
The xfOpenCV library is designed to work with Zynq, Zynq Ultrascale+, and Alveo FPGAs. The library has been verified on zcu102, zcu104 and U200 boards.

SDx 2019.1 Development Environment is required to work with the library.

**SDSoC Flow:**

zcu102 base or zcu102 reVISION-min platform is required to run the library on zcu102 board. Similarly, zcu104 base or zcu104 reVISION platform is needed to run on zcu104 board. Base platforms are available within the SDx tool and reVISION platform(s) can be downloaded from here: [reVISION Platform] (link yet to be updated)

**SDAccel Flow:**

U200 platform, available in the SDx tool, is required to build and run the library functions on U200 PCIe board.

## OTHER INFORMATION
Full User Guide for xfOpenCV and using OpenCV on Xilinx devices Check here:
[Xilinx OpenCV User Guide][]

For information on getting started with the reVISION stack check here:
[reVISION Getting Started Guide]

For more information about SDSoC check here:
[SDSoC User Guide][]

For more information about SDAccel check here:
[SDAccel User Guide][]

## SUPPORT
For questions and to get help on this project or your own projects, visit the [SDSoC Forums][].

## LICENSE AND CONTRIBUTING TO THE REPOSITORY
The source for this project is licensed under the [3-Clause BSD License][]

To contribute to this project, follow the guidelines in the [Repository Contribution README][]

## ACKNOWLEDGEMENTS
This library is written by developers at
- [Xilinx](http://www.xilinx.com)

## REVISION HISTORY

Date      | Readme Version | Release Notes
--------  |----------------|-------------------------
June2017  | 1.0            | Initial Xilinx release <br> -Windows OS support is in Beta.
September2017  | 2.0            | 2017.2 Xilinx release <br>
December2017  | 3.0            | 2017.4 Xilinx release <br>
June2018  | 4.0            | 2018.2 Xilinx release <br>
December2018  | 5.0            | 2018.3 Xilinx release <br>
June2019  | 6.0            | 2019.1 Xilinx release <br>

## Changelog:
1. Added new functions:

    • Bounding Box

    • Crop

2. xfOpenCV library now supports color image processing. All the functions that have multi-channel support in [OpenCV][], their available counterparts in xfOpencv also do. (except [convertTo](https://github.com/Xilinx/xfopencv/tree/master/examples/convertbitdepth))

3. 2019.1 code base is not backward-compatible, i.e, all the functions in the library have to be built with 2019.1 SDx tools only. None of the functions in this release can be used with any of the previous versions of SDx.


4. Increased the number of SDAccel examples to 24, provided in *examples_sdaccel* directory. For more details, refer "Getting Started with SDAccel" chapter in [UG1233][].

5. Added 51 new conversions in cvtColor function.

6. Merged HLS Use model document into [UG1233][] as one of the chapters.

7. Fixed the issue in HLS use model, that throws segmentation fault in C-sim and Co-sim with large size image inputs.

8. Renamed Scale function to ConvertScaleAbs to match OpenCV.

9. Moved Kalman filter, Dense Optical Flow and Non-Pyramidal Optical flow hpp files to *include/video* folder.

10. The datatype of *xf::Mat* Class pointer member '*data*' , which earlier used to be always inferred as HLS arbitrary precision type, has been modified to be inferred as a Structure or a HLS arbitrary precision type based on the stage of the build.

11. Introduced *read, write, read_float* and *write_float* member functions to facilitate data access of *xf::Mat* objects.

12. WarpAffine and WarpPerspective functions have been deprecated. Warptransform serves the purpose of both.

13. Extended Kalman filter feature added into Kalman filter function. Extra template parameter added in API.

14. 8 pixel parallelism support added in Bilateral filter function.
15. Added latency calculation logic around OpenCV/reference function calls in the testbench files under *examples* directory.

16. Updated the format of SDSoC Makefiles.
17. All the Makefiles in SDSoC examples, by default, now will build for 75MHz and point to zcu102 base platform instead of reVISION platform. For more details, refer "Building a Project Using the Example Makefiles on Linux" section of [UG1233][].

18. Minor bug fixes.

#### Known Issues:
1. Windows OS has path length limitations, kernel names must be smaller than 25 characters.

2. The following functions will have increased latency in **URAM** configurations, because of a known issue in Vivado HLS synthesis:

	• Warptransform  (PERSPECTIVE_BILINEAR_NPPC1, PERSPECTIVE_BILINEAR_RGB_NPPC1,  AFFINE_BILINEAR_RGB_NPPC1 and AFFINE_BILINEAR_NPPC1 configurations)

	• Remap

	• Stereo Pipeline

	• Corner tracker

	• Dense Pyramidal optical flow

	• Kalman filter

  [reVISION Getting Started Guide]: https://github.com/Xilinx/Revision-Getting-Started-Guide
  [HLS Video Library]:
  https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/18841665/HLS+Video+Library
  [reVISION Platform]: https://github.com/Xilinx/reVISION-Getting-Started-Guide/blob/master/Docs/software-tools-system-requirements.md#32-software
  [SDSoC Forums]: https://forums.xilinx.com/t5/SDSoC-Development-Environment/bd-p/sdsoc
  [SDSoC User Guide]: https://www.xilinx.com/support/documentation/sw_manuals/xilinx2019_1/ug1027-sdsoc-user-guide.pdf
  [3-Clause BSD License]: LICENSE.txt
  [Repository Contribution README]: CONTRIBUTING.md
  [Xilinx OpenCV User Guide]: https://www.xilinx.com/support/documentation/sw_manuals/xilinx2019_1/ug1233-xilinx-opencv-user-guide.pdf
  [UG1233]:
  https://www.xilinx.com/support/documentation/sw_manuals/xilinx2019_1/ug1233-xilinx-opencv-user-guide.pdf
  [SDAccel User Guide]:
  https://www.xilinx.com/support/documentation/sw_manuals/xilinx2019_1/ug1023-sdaccel-user-guide.pdf
  [OpenCV]:
  https://github.com/opencv/opencv
