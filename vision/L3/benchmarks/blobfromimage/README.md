## BlobFromImage Benchmark

In machine learning, data preprocessing is an integral step required to convert input data into a clean data set. A machine learning application receives data from multiple sources using multiple formats; this data needs to be transformed to format feasible for analysis before being passed to the model.

This example shows how various Xilinx ® Vitis™ Vision accelerated library funtions can be used to accelerate preprocessing of input images before feeding them to a Deep Neural Network (DNN) accelerator.

This specific application shows how pre-processing for Googlenet_v1 can be accelerated which involves resizing the input image to 224 x 224 size followed by mean subtraction. The below figure depicts the pipeline.

![Googlenet pre-processing](./gnet_pp.JPG)


The below code snippet shows the top level wrapper function which contains various Xilinx ® Vitis™ Vision accelerated library funtion calls.

```c++
void preprocessing ()
{
...
	xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC, XF_CV_DEPTH_IN> imgInput0(in_img_height, in_img_width);
	xf::cv::Mat<OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, XF_CV_DEPTH_RESIZE_OUT> resize_out_mat(resize_height, resize_width);
	
        xf::cv::Array2xfMat<INPUT_PTR_WIDTH,XF_8UC3,HEIGHT, WIDTH, NPC1>  (img_inp, imgInput0);
        xf::cv::resize<INTERPOLATION,TYPE,HEIGHT,WIDTH,NEWHEIGHT,NEWWIDTH,NPC_T,MAXDOWNSCALE> (imgInput0, resize_out_mat);
        xf::cv::preProcess<IN_TYPE, OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, WIDTH_A, IBITS_A, WIDTH_B, IBITS_B, WIDTH_OUT,
                       IBITS_OUT, XF_CV_DEPTH_RESIZE_OUT, XF_CV_DEPTH_OUT>(resize_out_mat, out_mat, params);


```

**Performance:**

Table below shows the speed up achieved compared to various CPU implementations.

|              |  Intel(R) Xeon (R)   Silver 4100 CPU @ 2.10GHz, 8 core |  Intel(R) Core(TM) i7-4770 CPU @ 3.40GHz, 4 core |  FPGA   (Alveo-U200) |  Speedup   (Xeon/i7) |
|:------------:|:------------------------------------------------------:|:------------------------------------------------:|:--------------------:|:--------------------:|
| Googlenet_v1 |                         5.63 ms                        |                      59.9 ms                     |        1.1 ms        |        5x/54x        |



### Commands to run:

**For PCIe Devices:**

    source < path-to-Vitis-installation-directory >/settings64.sh
    source < path-to-XRT-installation-directory >/setup.sh
    export PLATFORM=< path-to-platform-directory >/< platform >.xpfm
	export OPENCV_INCLUDE=< path-to-opencv-include-folder >
	export OPENCV_LIB=< path-to-opencv-lib-folder >
	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:< path-to-opencv-lib-folder >
    make host xclbin TARGET=< sw_emu|hw_emu|hw >
    make run TARGET=< sw_emu|hw_emu|hw >

**For Embedded Devices:**

Software Emulation:

    source < path-to-Vitis-installation-directory >/settings64.sh
    source < path-to-XRT-installation-directory >/setup.sh
    export PLATFORM=< path-to-platform-directory >/< platform >.xpfm
	export OPENCV_INCLUDE=< path-to-opencv-include-folder >
	export OPENCV_LIB=< path-to-opencv-lib-folder >
	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:< path-to-opencv-lib-folder >
    make run TARGET=sw_emu

Hardware Emulation and Hardware Build:

	Download the platform, and common-image from Xilinx Download Center. Run the sdk.sh script from the common-image directory to install sysroot using the command : "./sdk.sh -y -d ./ -p"

	Unzip the rootfs file : "gunzip ./rootfs.ext4.gz"

    source < path-to-Vitis-installation-directory >/settings64.sh
    export PLATFORM=< path-to-platform-directory >/< platform >.xpfm
    export SYSROOT=< path-to-platform-sysroot >
    make host xclbin TARGET=< hw_emu|hw > 
    make run TARGET=< hw_emu|hw > #This command will generate only the sd_card folder in case of hardware build.
    
**Note**. For hw run on embedded devices, copy the generated sd_card folder content under package_hw to an SD Card. More information on preparing the SD Card is available [here](https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/18842385/How+to+format+SD+card+for+SD+boot#HowtoformatSDcardforSDboot-CopingtheImagestotheNewPartitions):

    cd /mnt

    export XCL_BINDIR=< xclbin-folder-present-in-the-sd_card > #For example, "export XCL_BINDIR=xclbin_zcu102_base_hw"

    ./run_script.sh
