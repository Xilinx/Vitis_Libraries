.. Copyright © 2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.


.. _l3_cornertracker:


Corner Tracker
###############

The Corner Tracker example resides in the ``L3/examples/cornertracker`` directory.

This benchmark tests the performance of the `cornertracker` function with a sequence of two images. This example illustrates how to detect and track the characteristic feature points in a set of successive frames of video. A Harris corner detector is used as the feature detector, and a modified version of the Lucas Kanade optical flow is used for tracking. The core part of the algorithm takes in the current and next frame as the inputs and outputs the list of tracked corners. The current image is the first frame in the set, then corner detection is performed to detect the features to track.

The tutorial provides a step-by-step guide that covers commands for building and running a kernel.

Executable Usage
================

* **Work Directory (Step 1)**

The steps for library download and environment setup can be found in the README of the L3 folder. For getting the design:

.. code-block:: bash

   cd L3/examples/cornertracker

* **Build Kernel (Step 2)**

Run the following make command to build your XCLBIN and host binary targeting a specific device. Be aware that this process can take up to a couple of hours.

.. code-block:: bash

   export OPENCV_INCLUDE=< path-to-opencv-include-folder >
   export OPENCV_LIB=< path-to-opencv-lib-folder >
   export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:< path-to-opencv-lib-folder >
   export PLATFORM=< path-to-platform-directory >/< platform >.xpfm
   make host xclbin TARGET=hw

* **Run Kernel (Step 3)**

To get the benchmark results, run the following command.

.. code-block:: bash

   make run TARGET=hw

* **Example Output (Step 4)** 

.. code-block:: bash
   
   -----------Corner Tracker Design---------------
   Found Platform
   Platform Name: Xilinx
   XCLBIN File Name: krnl_cornertracker
   INFO: Importing vision/L3/tests/cornertracker/cornertrack/Xilinx_Cornertrack_L3_Test_vitis_hw_u200/build_dir.hw.xilinx_u200_xdma_201830_2/krnl_cornertracker.xclbin
   Loading: 'vision/L3/tests/cornertracker/cornertrack/Xilinx_Cornertrack_L3_Test_vitis_hw_u200/build_dir.hw.xilinx_u200_xdma_201830_2/krnl_cornertracker.xclbin'
   ***************************************************
   Test Case no: 1

    Harris Execution

    Harris: Buffers created

    Harris: data copied to device

    Harris: args set

    Harris kernel called
   

    Harris Done

    Pyrdown Execution

    Pyrdown: Buffers created

    Pyrdown: data copied to device

    Pyrdown Args set

    pyr in ht = 144
    pyr in wd = 256
    pyr out ht = 72
    pyr out wd = 128

    Pyrdown kernel called
   

    Pyrdown data copied to host

    Pyrdown Execution done

    Pyrdown: Buffers created

    Pyrdown: data copied to device

    Pyrdown Args set

    pyr in ht = 72
    pyr in wd = 128
    pyr out ht = 36
    pyr out wd = 64

    Pyrdown kernel called
   

    Pyrdown data copied to host

    Pyrdown Execution done

    Pyrdown: Buffers created

    Pyrdown: data copied to device

    Pyrdown Args set

    pyr in ht = 36
    pyr in wd = 64
    pyr out ht = 18
    pyr out wd = 32

    Pyrdown kernel called
   

    Pyrdown data copied to host

    Pyrdown Execution done

    Pyrdown: Buffers created

    Pyrdown: data copied to device

    Pyrdown Args set

    pyr in ht = 18
    pyr in wd = 32
    pyr out ht = 9
    pyr out wd = 16

    Pyrdown kernel called
   

    Pyrdown data copied to host

    Pyrdown Execution done

    **********Optical Flow Computation*******************

    Buffers created

     *********OF Computation Level =4*********

     *********OF Computation iteration =0*********

    Data copied from host to device

    kernel args set

    flow_rows =9flow_cols=16flow_in_rows =9flow_in_cols =16

    level4calls done0

     *********OF Computation iteration =1*********

    Data copied from host to device

    kernel args set

    flow_rows =9flow_cols=16flow_in_rows =9flow_in_cols =16

    level4calls done1

     *********OF Computation iteration =2*********

    Data copied from host to device

    kernel args set

    flow_rows =9flow_cols=16flow_in_rows =9flow_in_cols =16

    level4calls done2

     *********OF Computation iteration =3*********

    Data copied from host to device

    kernel args set

    flow_rows =9flow_cols=16flow_in_rows =9flow_in_cols =16

    level4calls done3

     *********OF Computation iteration =4*********

    Data copied from host to device

    kernel args set

    flow_rows =9flow_cols=16flow_in_rows =9flow_in_cols =16

    level4calls done4

    Buffers created

     *********OF Computation Level =3*********

     *********OF Computation iteration =0*********

    Data copied from host to device

    kernel args set

    flow_rows =18flow_cols=32flow_in_rows =9flow_in_cols =16

    level3calls done0

     *********OF Computation iteration =1*********

    Data copied from host to device

    kernel args set

    flow_rows =18flow_cols=32flow_in_rows =18flow_in_cols =32

    level3calls done1

     *********OF Computation iteration =2*********

    Data copied from host to device

    kernel args set

    flow_rows =18flow_cols=32flow_in_rows =18flow_in_cols =32

    level3calls done2

     *********OF Computation iteration =3*********

    Data copied from host to device

    kernel args set

    flow_rows =18flow_cols=32flow_in_rows =18flow_in_cols =32

    level3calls done3

     *********OF Computation iteration =4*********

    Data copied from host to device

    kernel args set

    flow_rows =18flow_cols=32flow_in_rows =18flow_in_cols =32

    level3calls done4

    Buffers created

     *********OF Computation Level =2*********

     *********OF Computation iteration =0*********

    Data copied from host to device

    kernel args set

    flow_rows =36flow_cols=64flow_in_rows =18flow_in_cols =32

    level2calls done0

     *********OF Computation iteration =1*********

    Data copied from host to device

    kernel args set

    flow_rows =36flow_cols=64flow_in_rows =36flow_in_cols =64

    level2calls done1

     *********OF Computation iteration =2*********

    Data copied from host to device

    kernel args set

    flow_rows =36flow_cols=64flow_in_rows =36flow_in_cols =64

    level2calls done2

     *********OF Computation iteration =3*********

    Data copied from host to device

    kernel args set

    flow_rows =36flow_cols=64flow_in_rows =36flow_in_cols =64

    level2calls done3

     *********OF Computation iteration =4*********

    Data copied from host to device

    kernel args set

    flow_rows =36flow_cols=64flow_in_rows =36flow_in_cols =64

    level2calls done4

    Buffers created

     *********OF Computation Level =1*********

     *********OF Computation iteration =0*********

    Data copied from host to device

    kernel args set

    flow_rows =72flow_cols=128flow_in_rows =36flow_in_cols =64

    level1calls done0

     *********OF Computation iteration =1*********

    Data copied from host to device

    kernel args set

    flow_rows =72flow_cols=128flow_in_rows =72flow_in_cols =128

    level1calls done1

     *********OF Computation iteration =2*********

    Data copied from host to device

    kernel args set

    flow_rows =72flow_cols=128flow_in_rows =72flow_in_cols =128

    level1calls done2

     *********OF Computation iteration =3*********

    Data copied from host to device

    kernel args set

    flow_rows =72flow_cols=128flow_in_rows =72flow_in_cols =128

    level1calls done3

     *********OF Computation iteration =4*********

    Data copied from host to device

    kernel args set

    flow_rows =72flow_cols=128flow_in_rows =72flow_in_cols =128

    level1calls done4

    Buffers created

     *********OF Computation Level =0*********

     *********OF Computation iteration =0*********

    Data copied from host to device

    kernel args set

    flow_rows =144flow_cols=256flow_in_rows =72flow_in_cols =128

    level0calls done0

     *********OF Computation iteration =1*********

    Data copied from host to device

    kernel args set

    flow_rows =144flow_cols=256flow_in_rows =144flow_in_cols =256

    level0calls done1

     *********OF Computation iteration =2*********

    Data copied from host to device

    kernel args set

    flow_rows =144flow_cols=256flow_in_rows =144flow_in_cols =256

    level0calls done2

     *********OF Computation iteration =3*********

    Data copied from host to device

    kernel args set

    flow_rows =144flow_cols=256flow_in_rows =144flow_in_cols =256

    level0calls done3

     *********OF Computation iteration =4*********

    Data copied from host to device

    kernel args set

    flow_rows =144flow_cols=256flow_in_rows =144flow_in_cols =256

    level0calls done4

    OF done

    **********Corner Update Computation*******************

    kernel args set

    flow_rows =144flow_cols=256num of corners=47harris_flag=1

    Corner Update called

    Corner Update done

    OF done

    Num of corners = 47
   ***************************************************
   ***************************************************
   Test Case no: 2

    Harris Execution

    Harris: Buffers created

    Harris: data copied to device

    Harris: args set

    Harris kernel called
   

    Harris Done

    Pyrdown Execution

    Pyrdown: Buffers created

    Pyrdown: data copied to device

    Pyrdown Args set

    pyr in ht = 144
    pyr in wd = 256
    pyr out ht = 72
    pyr out wd = 128

    Pyrdown kernel called
   

    Pyrdown data copied to host

    Pyrdown Execution done

    Pyrdown: Buffers created

    Pyrdown: data copied to device

    Pyrdown Args set

    pyr in ht = 72
    pyr in wd = 128
    pyr out ht = 36
    pyr out wd = 64

    Pyrdown kernel called
   

    Pyrdown data copied to host

    Pyrdown Execution done

    Pyrdown: Buffers created

    Pyrdown: data copied to device

    Pyrdown Args set

    pyr in ht = 36
    pyr in wd = 64
    pyr out ht = 18
    pyr out wd = 32

    Pyrdown kernel called
   

    Pyrdown data copied to host

    Pyrdown Execution done

    Pyrdown: Buffers created

    Pyrdown: data copied to device

    Pyrdown Args set

    pyr in ht = 18
    pyr in wd = 32
    pyr out ht = 9
    pyr out wd = 16

    Pyrdown kernel called
   

    Pyrdown data copied to host

    Pyrdown Execution done

    **********Optical Flow Computation*******************

    Buffers created

     *********OF Computation Level =4*********

     *********OF Computation iteration =0*********

    Data copied from host to device

    kernel args set

    flow_rows =9flow_cols=16flow_in_rows =9flow_in_cols =16

    level4calls done0

     *********OF Computation iteration =1*********

    Data copied from host to device

    kernel args set

    flow_rows =9flow_cols=16flow_in_rows =9flow_in_cols =16

    level4calls done1

     *********OF Computation iteration =2*********

    Data copied from host to device

    kernel args set

    flow_rows =9flow_cols=16flow_in_rows =9flow_in_cols =16

    level4calls done2

     *********OF Computation iteration =3*********

    Data copied from host to device

    kernel args set

    flow_rows =9flow_cols=16flow_in_rows =9flow_in_cols =16

    level4calls done3

     *********OF Computation iteration =4*********

    Data copied from host to device

    kernel args set

    flow_rows =9flow_cols=16flow_in_rows =9flow_in_cols =16

    level4calls done4

    Buffers created

     *********OF Computation Level =3*********

     *********OF Computation iteration =0*********

    Data copied from host to device

    kernel args set

    flow_rows =18flow_cols=32flow_in_rows =9flow_in_cols =16

    level3calls done0

     *********OF Computation iteration =1*********

    Data copied from host to device

    kernel args set

    flow_rows =18flow_cols=32flow_in_rows =18flow_in_cols =32

    level3calls done1

     *********OF Computation iteration =2*********

    Data copied from host to device

    kernel args set

    flow_rows =18flow_cols=32flow_in_rows =18flow_in_cols =32

    level3calls done2

     *********OF Computation iteration =3*********

    Data copied from host to device

    kernel args set

    flow_rows =18flow_cols=32flow_in_rows =18flow_in_cols =32

    level3calls done3

     *********OF Computation iteration =4*********

    Data copied from host to device

    kernel args set

    flow_rows =18flow_cols=32flow_in_rows =18flow_in_cols =32

    level3calls done4

    Buffers created

     *********OF Computation Level =2*********

     *********OF Computation iteration =0*********

    Data copied from host to device

    kernel args set

    flow_rows =36flow_cols=64flow_in_rows =18flow_in_cols =32

    level2calls done0

     *********OF Computation iteration =1*********

    Data copied from host to device

    kernel args set

    flow_rows =36flow_cols=64flow_in_rows =36flow_in_cols =64

    level2calls done1

     *********OF Computation iteration =2*********

    Data copied from host to device

    kernel args set

    flow_rows =36flow_cols=64flow_in_rows =36flow_in_cols =64

    level2calls done2

     *********OF Computation iteration =3*********

    Data copied from host to device

    kernel args set

    flow_rows =36flow_cols=64flow_in_rows =36flow_in_cols =64

    level2calls done3

     *********OF Computation iteration =4*********

    Data copied from host to device

    kernel args set

    flow_rows =36flow_cols=64flow_in_rows =36flow_in_cols =64

    level2calls done4

    Buffers created

     *********OF Computation Level =1*********

     *********OF Computation iteration =0*********

    Data copied from host to device

    kernel args set

    flow_rows =72flow_cols=128flow_in_rows =36flow_in_cols =64

    level1calls done0

     *********OF Computation iteration =1*********

    Data copied from host to device

    kernel args set

    flow_rows =72flow_cols=128flow_in_rows =72flow_in_cols =128

    level1calls done1

     *********OF Computation iteration =2*********

    Data copied from host to device

    kernel args set

    flow_rows =72flow_cols=128flow_in_rows =72flow_in_cols =128

    level1calls done2

     *********OF Computation iteration =3*********

    Data copied from host to device

    kernel args set

    flow_rows =72flow_cols=128flow_in_rows =72flow_in_cols =128

    level1calls done3

     *********OF Computation iteration =4*********

    Data copied from host to device

    kernel args set

    flow_rows =72flow_cols=128flow_in_rows =72flow_in_cols =128

    level1calls done4

    Buffers created

     *********OF Computation Level =0*********

     *********OF Computation iteration =0*********

    Data copied from host to device

    kernel args set

    flow_rows =144flow_cols=256flow_in_rows =72flow_in_cols =128

    level0calls done0

     *********OF Computation iteration =1*********

    Data copied from host to device

    kernel args set

    flow_rows =144flow_cols=256flow_in_rows =144flow_in_cols =256

    level0calls done1

     *********OF Computation iteration =2*********

    Data copied from host to device

    kernel args set

    flow_rows =144flow_cols=256flow_in_rows =144flow_in_cols =256

    level0calls done2

     *********OF Computation iteration =3*********

    Data copied from host to device

    kernel args set

    flow_rows =144flow_cols=256flow_in_rows =144flow_in_cols =256

    level0calls done3

     *********OF Computation iteration =4*********

    Data copied from host to device

    kernel args set

    flow_rows =144flow_cols=256flow_in_rows =144flow_in_cols =256

    level0calls done4

    OF done

    **********Corner Update Computation*******************

    kernel args set

    flow_rows =144flow_cols=256num of corners=47harris_flag=0

    Corner Update called

    Corner Update done

    OF done

    Num of corners = 47
   ***************************************************
   ------------------------------------------------------------

Profiling 
=========

The corner tracker design is validated on an Alveo U200 board at 300 MHz frequency. 
Hardware resource utilizations is listed in the following table.

.. table:: Table 1: Hardware resources for Corner Tracker
    :align: center

    +-------------+----------------------------+--------------+-----------+----------+--------+
    |    Name     |           Dataset          |      LUT     |    BRAM   |    FF    |   DSP  |
    +=============+============================+==============+===========+==========+========+
    |cornertracker|    2x4K images, 8 NPPC     |    37547     |    225    |  35089   |   115  |
    +-------------+----------------------------+--------------+-----------+----------+--------+
    |cornertracker|  2xFull HD images, 8 NPPC  |    34624     |    129    |  34198   |   115  |
    +-------------+----------------------------+--------------+-----------+----------+--------+


The performance is shown below

.. table:: Table 2: Performance Numbers in terms of FPS (Frames Per Second) for Two Consecutive Frames for Corner Tracker
    :align: center
	
    +----------------------+--------------+--------------+
    |       Dataset        |   FPS(CPU)   |   FPS(FPGA)  |
    +======================+==============+==============+
    |     4k (3840x2160)   |     0.53     |    3         |
    +----------------------+--------------+--------------+
    |   Full HD(1920x1080) |     2        |    11        |
    +----------------------+--------------+--------------+


.. toctree::
    :maxdepth: 1
