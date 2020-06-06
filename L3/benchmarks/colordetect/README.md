## colordetection Benchmark

This example shows how various Vitis Vision funtions can be stitched in pipeline to detect colors in the input image.

This application shows how colordetection can be accelerated.

**Performance:**

| Testcases	| Resolution			| x86(ms) - Intel(R) Xeon(R) Silver 4110 CPU @ 2.10GHz, 8 core	| x86(ms) - Intel(R) Core(TM) i7-4770 CPU @ 3.40GHz, 4 core | arm(ms)	| HW(ms) |
|-----------|-----------------------|---------------------------------------------------------------|-----------------------------------------------------------|-----------|--------|
| test1	    | 4k(3840x2160)			| 							97.89								|						120.70					  			| 996		| 28.15	 |
| test2	    | FULL-HD(1920x1080)	| 							28.24								|						75.08					  			| 250.15	| 7.5	 |
| test3	   	| SD(640x480)			| 							11.35								|						67.38					  			| 38.71		| 1.5	 |


**Overall Performance (Images/sec) **

software colordetection cv::colordetect on x86    : 35 images(full-hd)/sec

hardware accelerated xf::cv::colordetect on FPGA  : 133 images(full-hd)/sec

### Commands to run:

    source < path-to-Vitis-installation-directory >/settings64.sh

    source < path-to-XRT-installation-directory >/setup.sh

    export DEVICE=< path-to-platform-directory >/< platform >.xpfm

**For PCIe devices:**

    make all TARGET=< sw_emu|hw_emu|hw >

    make run TARGET=< sw_emu|hw_emu|hw >

**For embedded devices:**

    export SYSROOT=< path-to-platform-sysroot >

    make host xclbin TARGET=< sw_emu|hw_emu|hw > BOARD=Zynq ARCH=< aarch32 | aarch64 >

    make run TARGET=< sw_emu|hw_emu|hw >  BOARD=Zynq ARCH=< aarch32 | aarch64 > #This command will generate the sd_card folder

**Note1**. For non-DFX platforms, BOOT.BIN has to be manually copied from < build-directory >/< xclbin-folder >/sd\_card / to the top level sd_card folder.

**Note2**. For hw run on embedded devices, copy the generated sd_card folder content to an SDCARD and either run the "init.sh" or run the following commands on the board:

    cd /mnt
	   
    export XCL_BINDIR=< xclbin-folder-present-in-the-sd_card > #For example, "export XCL_BINDIR=xclbin_zcu102_base_hw"
	   
    ./< executable > < arguments >
