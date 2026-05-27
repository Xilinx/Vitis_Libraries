## Level 2: Kernels and Engines

Level 2 contains the OpenCL host-callable kernels and engines for various Vitis Vision functions.

The 'examples' folder contains the OpenCL host code file and a C++ accel file that demonstrate the call of Vitis Vision functions to build for Vitis.

The 'examples/config' folder contains the configuration file used to modify the default configuration of the function.

The 'tests' folder has sub-folders named according to the function and the configuration it would run. Each individual folder has Makefiles and config files that would perform software emulation, hardware emulation and hardware build of the corresponding function in examples folder, based on the platform you select.

The 'tests/aie-ml' directory contains tests that are targeted for AI engine.

### Commands to Run AIE Tests

    source < path-to-Vitis-installation-directory >/settings64.sh
    export PLATFORM=< path-to-platform-directory >/< platform >.xpfm
    export SYSROOT=< path-to-platform-sysroot >
    make run TARGET=< aiesim / x86sim / hw_emu / hw >

### Commands to Run PL Tests

**For PCIe Devices:**

    source < path-to-Vitis-installation-directory >/settings64.sh
    source < path-to-XRT-installation-directory >/setup.sh
    export PLATFORM=< path-to-platform-directory >/< platform >.xpfm
    export OPENCV_INCLUDE=< path-to-opencv-include-folder >
    export OPENCV_LIB=< path-to-opencv-lib-folder >
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:< path-to-opencv-lib-folder >
    make host xclbin TARGET=< hw_emu|hw >
    make run TARGET=< hw_emu|hw >

**For Embedded Devices**

Hardware Emulation and Hardware Build:

	Download the platform, and common-image from Xilinx Download Center. Run the sdk.sh script from the common-image directory to install sysroot using the command : "./sdk.sh -y -d ./ -p"

	Unzip the rootfs file : "gunzip ./rootfs.ext4.gz"

    source < path-to-Vitis-installation-directory >/settings64.sh
    export PLATFORM=< path-to-platform-directory >/< platform >.xpfm
    export SYSROOT=< path-to-platform-sysroot >
    make host xclbin TARGET=< hw_emu|hw > 
    make run TARGET=< hw_emu|hw > #This command will generate only the sd_card folder in case of hardware build.

**Note**: For hw runs on embedded devices, copy the generated ``sd_card`` folder content under ``package_hw`` to an SD Card. More information on preparing the SD Card is available [here](https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/18842385/How+to+format+SD+card+for+SD+boot#HowtoformatSDcardforSDboot-CopingtheImagestotheNewPartitions). After successful booting of the board, run the following commands:

    cd /mnt

    export XCL_BINDIR=< xclbin-folder-present-in-the-sd_card > #For example, "export XCL_BINDIR=xclbin_zcu102_base_hw"

    ./run_script.sh
