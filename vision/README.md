# Vitis Vision Library

The AMD Vitis™ Vision library is a set of 90+ kernels, optimized for AMD FPGAs, AI Engine, and SoCs, based on the OpenCV computer vision library. The kernels in the Vitis Vision library are optimized and supported in the Vitis Tool Suite.

## Hardware and Software Requirements

The Vitis Vision library is designed to work with Zynq™, Zynq Ultrascale+™, Versal™, and Alveo™ FPGAs. The library has been verified on zcu102, zcu104, vck190, U50, and U200 boards. AIE-ML functions are verified on VEK280 board.

### Prerequisites

* Valid installation of [Vitis™ 2024.1](https://docs.xilinx.com/r/en-US/ug1393-vitis-application-acceleration/Installing-the-Vitis-Software-Platform) or later version and the corresponding licenses.
* Xilinx Runtime ([XRT](https://docs.xilinx.com/r/en-US/ug1393-vitis-application-acceleration/Installing-Xilinx-Runtime-and-Platforms)) must be installed. XRT provides software interface to AMD FPGAs.
* Install [OpenCV-4.4.0]((https://github.com/opencv/opencv/tree/4.4.0)) x86 libraries (with compatible ``libjpeg.so``). x86 libs have to be used for:

   a. L1 flow irrespective of target FPGA device being PCIe or embedded.

   b. L2/L3 flow when the target device is PCIe based.

   c. L2/L3 flow when performing software emulation for an embedded platform.

    For L2/L3 flow targeting embedded platforms (for hardware emulation and hardware build), aarch32/aarch64 version OpenCV shipped within their *sysroot* should be used.

* libOpenCL.so must be [installed](https://docs.xilinx.com/r/en-US/ug1393-vitis-application-acceleration/OpenCL-Installable-Client-Driver-Loader) if not present.
* [Install the card](https://docs.xilinx.com/r/en-US/ug1301-getting-started-guide-alveo-accelerator-cards) for which the platform is supported in Vitis 2024.1 or later versions.
* If targeting an embedded platform, [install]((https://docs.xilinx.com/r/en-US/ug1393-vitis-application-acceleration/Installing-Embedded-Platforms?tocId=hfE7LFeS8mU4dexvgPL31Q)) it and set up the [evaluation board](https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/444006775/Zynq+UltraScale+MPSoC).

#### OpenCV Installation Guidance

It is recommended to do a fresh installation of OpenCV 4.4.0 and not use existing libs of your system, as they may or may not work with the Vitis environment.

**Please make sure you update and upgrade the packages and OS libraries of your system and
have cmake version>3.5 installed before proceeding.**

The following steps can help install the basic libs required to compile and link the OpenCV calls in Vitis Vision host code.

1. Create a directory "source" and clone [opencv-4.4.0](https://github.com/opencv/opencv/tree/4.4.0) into it.
2. Create a directory "source_contrib" and clone [opencv-4.4.0-contrib](https://github.com/opencv/opencv_contrib/tree/4.4.0) into it.
3. Create two directories: ``build`` and ``install``.
4. Open a bash terminal and *cd* to *build* directory
5. Run the command: ``export LIBRARY_PATH=/usr/lib/x86_64-linux-gnu/``
6. Run the command: *cmake -D CMAKE_BUILD_TYPE=RELEASE
  -D CMAKE_INSTALL_PREFIX=< path-to-install-directory>
  -D CMAKE_CXX_COMPILER=< path-to-Vivado-directory-in-Vitis-installation>/tps/lnx64/gcc-6.2.0/bin/g++
  -D OPENCV_EXTRA_MODULES_PATH=< path-to-source_contrib-directory>/modules/
  -D WITH_V4L=ON -DBUILD_TESTS=OFF -DBUILD_ZLIB=ON
  -DBUILD_JPEG=ON -DWITH_JPEG=ON -DWITH_PNG=ON
  -DBUILD_EXAMPLES=OFF -DINSTALL_C_EXAMPLES=OFF
  -DINSTALL_PYTHON_EXAMPLES=OFF -DWITH_OPENEXR=OFF
  -DBUILD_OPENEXR=OFF < path-to-source-directory >*
7. Run the command: ``make all -j8``
8. Run the command: ``make install``

The OpenCV includes and libs will be in the ``install`` directory

## Source Files Organization

The Vitis development environment supports a variety of build flows (based on the target engine (either AI Engine or Programmable Logic) and source type):

* **PL [HLS/RTL]**: Kernels targeting FPGA (PL), coded in C/C++/HDL for Vitis HLS.
* **AIE**: Kernels targeting AI Engine programmed in C/C++ in accordance to AI Engine coding methodology.
* **PL+AIE**: Kernels that target both FPGA (PL) and AI Engine.

To facilitate this, all unit level kernels are located in ``L1/include``. AI Engine kernels are located in ``L1/include/aie``.

Examples and tests in L1, L2, and L3 will use these unit level kernels in various ways for building a project.

The library is organized as follows:

~~~
├── L1/
│   ├── README.md
│   ├── examples/
│   ├── include/
│   ├── lib/
│   └── tests/
├── L2/
│   ├── README.md
│   ├── examples/
│   └── tests/
├── L3/
│   ├── README.md
│   ├── benchmarks/
│   ├── examples/
│   └── tests/
├── data/
├── LICENSE.txt
├── README.md
├── docs/
│   ├── images/
│   ├── include/
│   ├── Makefile
│   ├── Makefile.sphinx
│   ├── rst files
│   ├── conf.py
│   ├── conf_bhp.py
│   ├── env.csh
├── ext/
│   └── xcl2/
└── library.json
~~~

## Application Development

The Vitis library is organized into L1, L2, and L3 folders to facilitate various development stages.

**L1**:
      Makefiles and sources in ``L1/examples`` and ``L1/tests`` facilitate HLS-based flows for a quick check without considering the complexities of Platform, OpenCL/XRT, or framework. the following list shows the various uses:

* Check the functionality of an individual kernel (C-simulation)
* Estimate resource usage, latency etc., (Synthesis)
* Run cycle-accurate simulation (Co-simulation)
* Package as IP, get final resource utilization/timing details (Export RTL)
       
	**Note**:  When the RTL (or ``XO`` file after packaging IP) is generated, the Vivado flow can be invoked for ``XCLBIN`` file generation if required.

**L2** :
       Makefiles and sources in ``L2/examples`` and ``L2/tests`` facilitate building XCLBIN file from various sources (HDL, HLS or XO files) of kernels with host code written in OpenCL/XRT frame work targeting a device.
	   This flow supports:

* Software emulation to check the functionality
* Hardware emulation to check RTL level simulation
* Build and test on hardware

For AI Engine development ``L1/include/aie-ml`` , ``L2/tests/aie-ml`` has sources and Makefiles executing the following tasks:

* AIE simulation
* X86 simulation
* Hardware emulation
* Hardware build and run

**L3** :
       Makefiles and sources in ``L3/examples`` and ``L3/tests`` demonstrate applications developed using multiple kernels in the pipeline. The Makefiles provided can be used for executing tasks same as L2.

### Important

* Vitis and OpenCV-4.4.0 x86 version libs need to be installed prior to the L1 flow. Other prerequisites are optional for the L1 flow.

* Refer to the [L1 readme](https://github.com/Xilinx/Vitis_Libraries/blob/master/vision/L1/README.md) for details on how to setup the environment and run the L1 functions.

* All the prerequisites need to be installed before starting with L2/L3 Vitis flow.

* For embedded devices, platforms and common images have to be downloaded separately from the [download center](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/embedded-platforms.html).

* Refer to [L2/L3 readme](https://github.com/Xilinx/Vitis_Libraries/blob/master/vision/L2/README.md) for details on how to setup the environment and run the L2/L3 functions.
	
* All limitations and constraints of Vitis ([OS support, compatibility](https://docs.xilinx.com/r/en-US/ug1393-vitis-application-acceleration/Installation-Requirements) etc.) are also applicable to Vitis Vision library.



## Other Information

The user guides for Vitis Vision and using OpenCV on AMD devices are available [here](https://docs.xilinx.com/r/en-US/Vitis_Libraries/vision/index.html).

## Support

For questions and to get help on this project or your own projects, visit the [Xilinx Forums](https://support.xilinx.com/s/topic/0TO2E000000YKYAWA4/vitis-acceleration-acceleration?language=en_US)

## License and Contributing to the Repository

The source for this project is licensed under the [Apache License](http://www.apache.org/licenses/LICENSE-2.0)

    Copyright (C) 2024, Advanced Micro Devices, Inc.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

To contribute to this project, follow the guidelines in the [Repository Contribution README][] (link yet to be updated)

## Acknowledgments

This library is written by developers at
- [AMD](http://www.xilinx.com)

## Changelog

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
	    	• YUY2-Filter2d (no GMIO)

    • Updates:
    	• Minor bug fixes
    	• Removed AIE1 functions

**Known issues**

  * Vitis GUI projects on RHEL83 and CEntOS82 may fail because of a lib conflict in the
     ``LD_LIBRARY_PATH`` setting. You need to remove ``${env_var:LD_LIBRARY_PATH}`` from the project
      environment settings for the function to build successfully.
  * ``rgbir2bayer`` and ``isppipeline_rgbir`` PL functions are not supplied with input images.
  * ``lkdensepyroptflow`` fails to meet timing when URAM is enabled.
