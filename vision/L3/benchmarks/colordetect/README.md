## colordetection Benchmark

This example shows how various xfOpenCV funtions can be stitched in pipeline to detect colors of inut image.

This application shows how colordetection can be accelerated.

**Performance:**

| Testcases	| Resolution			| x86(ms)	| arm(ms)	| HW(ms) |
|-----------|-----------------------|-----------|-----------|--------|
| test1	    | 4k(3840x2160)			| 189.3		| 996		| 28.15	 |
| test2	    | FULL-HD(1920x1080)	| 47.67		| 250.15	| 7.5	 |
| test3	   	| SD(640x480)			| 8.94		| 38.71		| 1.5	 |


**Overall Performance (Images/sec) **

software colordetection cv::colordetect on x86    : 20 images(full-hd)/sec

hardware accelerated xf::colordetect     : 133 images(full-hd)/ on sec

### Commands to run:

source < path-to-Vitis-installation-directory >/settings64.sh

source < part-to-XRT-installation-directory >/setenv.sh

export DEVICE=< path-to-platform-directory >/<platform>.xpfm

**For PCIe devices:**

make all TARGET=< sw_emu|hw_emu|hw >

make run TARGET=< sw_emu|hw_emu|hw >

**For embedded devices:**

export SYSROOT=< path-to-platform-sysroot >

make all TARGET=hw BOARD=Zynq

copy the image.ub, xclbins and executable to an SDCARD and run on the board.
