.. Copyright © 2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.


.. _l1_pyroptflow:


Dense Pyramidal LK Optical Flow
################################

The Dense Pyramidal LK Optical Flow example resides in the ``L2/examples/lkdensepyrof`` directory.

This benchmark tests the performance of the `lkdensepyrof` function with a pair of images. Optical flow is the pattern of apparent motion of image objects between two consecutive frames, caused by the movement of an object or camera. It is a 2D vector field, where each vector is a displacement vector showing the movement of points from the first frame to the second.

The tutorial provides a step-by-step guide that covers commands for building and running a kernel.

Executable Usage
================

* **Work Directory (Step 1)**

The steps for library download and environment setup can be found in the README of the L2 folder. For getting the design:

.. code-block:: bash

   cd L2/examples/lkdensepyrof

* **Build Kernel (Step 2)**

Run the following make command to build your XCLBIN and host binary targeting a specific device. Be aware that this process can take up to a couple of hours.

.. code-block:: bash

   export OPENCV_INCLUDE=< path-to-opencv-include-folder >
   export OPENCV_LIB=< path-to-opencv-lib-folder >
   export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:< path-to-opencv-lib-folder >
   export PLATFORM=< path-to-platform-directory >/< platform >.xpfm
   make host xclbin TARGET=hw

* **Run Kernel (Step 3)**

To get the benchmark results, please run the following command.

.. code-block:: bash

   make run TARGET=hw

* **Example Output (Step 4)** 

.. code-block:: bash
   
   -----------Optical Flow Design---------------
   Found Platform
   Platform Name: Xilinx
   XCLBIN File Name: krnl_pyr_dense_optical_flow
   INFO: Importing vision/L2/examples/lkdensepyrof/Xilinx_Lkdensepyrof_L2_Test_vitis_hw_u200/build_dir.hw.xilinx_u200_xdma_201830_2/krnl_pyr_dense_optical_flow.xclbin
   Loading: 'vision/L2/examples/lkdensepyrof/Xilinx_Lkdensepyrof_L2_Test_vitis_hw_u200/build_dir.hw.xilinx_u200_xdma_201830_2/krnl_pyr_dense_optical_flow.xclbin'

    *********Pyr Down Execution*********

    CL buffer created

    data copied to host

    Kernel args set
   opencv

   0 image  0 level pyrdown done

    CL buffer created

    data copied to host

    Kernel args set
   opencv

   0 image  1 level pyrdown done

    CL buffer created

    data copied to host

    Kernel args set
   opencv

   0 image  2 level pyrdown done

    CL buffer created

    data copied to host

    Kernel args set
   opencv

   0 image  3 level pyrdown done

    One image done

    CL buffer created

    data copied to host

    Kernel args set
   opencv

   1 image  0 level pyrdown done

    CL buffer created

    data copied to host

    Kernel args set
   opencv

   1 image  1 level pyrdown done

    CL buffer created

    data copied to host

    Kernel args set
   opencv

   1 image  2 level pyrdown done

    CL buffer created

    data copied to host

    Kernel args set
   opencv

   1 image  3 level pyrdown done

    One image done

    *********Pyr Down Done*********

    *********Starting OF Computation*********

   Buffers created

    *********OF Computation Level = 4*********

    *********OF Computation iteration = 0*********

   Data copied from host to device

   kernel args set

   4 level 0 calls done

    *********OF Computation iteration = 1*********

   Data copied from host to device

   kernel args set

   4 level 1 calls done

    *********OF Computation iteration = 2*********

   Data copied from host to device

   kernel args set

   4 level 2 calls done

    *********OF Computation iteration = 3*********

   Data copied from host to device

   kernel args set

   4 level 3 calls done

    *********OF Computation iteration = 4*********

   Data copied from host to device

   kernel args set

   4 level 4 calls done

   Buffers created

    *********OF Computation Level = 3*********

    *********OF Computation iteration = 0*********

   Data copied from host to device

   kernel args set

   3 level 0 calls done

    *********OF Computation iteration = 1*********

   Data copied from host to device

   kernel args set

   3 level 1 calls done

    *********OF Computation iteration = 2*********

   Data copied from host to device

   kernel args set

   3 level 2 calls done

    *********OF Computation iteration = 3*********

   Data copied from host to device

   kernel args set

   3 level 3 calls done

    *********OF Computation iteration = 4*********

   Data copied from host to device

   kernel args set

   3 level 4 calls done

   Buffers created

    *********OF Computation Level = 2*********

    *********OF Computation iteration = 0*********

   Data copied from host to device

   kernel args set

   2 level 0 calls done

    *********OF Computation iteration = 1*********

   Data copied from host to device

   kernel args set

   2 level 1 calls done

    *********OF Computation iteration = 2*********

   Data copied from host to device

   kernel args set

   2 level 2 calls done

    *********OF Computation iteration = 3*********

   Data copied from host to device

   kernel args set

   2 level 3 calls done

    *********OF Computation iteration = 4*********

   Data copied from host to device

   kernel args set

   2 level 4 calls done

   Buffers created

    *********OF Computation Level = 1*********

    *********OF Computation iteration = 0*********

   Data copied from host to device

   kernel args set

   1 level 0 calls done

    *********OF Computation iteration = 1*********

   Data copied from host to device

   kernel args set

   1 level 1 calls done

    *********OF Computation iteration = 2*********

   Data copied from host to device

   kernel args set

   1 level 2 calls done

    *********OF Computation iteration = 3*********

   Data copied from host to device

   kernel args set

   1 level 3 calls done

    *********OF Computation iteration = 4*********

   Data copied from host to device

   kernel args set

   1 level 4 calls done

   Buffers created

    *********OF Computation Level = 0*********

    *********OF Computation iteration = 0*********

   Data copied from host to device

   kernel args set

   0 level 0 calls done

    *********OF Computation iteration = 1*********

   Data copied from host to device

   kernel args set

   0 level 1 calls done

    *********OF Computation iteration = 2*********

   Data copied from host to device

   kernel args set

   0 level 2 calls done

    *********OF Computation iteration = 3*********

   Data copied from host to device

   kernel args set

   0 level 3 calls done

    *********OF Computation iteration = 4*********

   Data copied from host to device

   kernel args set

   0 level 4 calls done
   ------------------------------------------------------------

Profiling 
==========

The lkdensepyrof design is validated on an Alveo U200 board at 300 MHz frequency. 
Fardware resource utilizations is listed in the following table.

.. table:: Table 1: Hardware Resources for LK Dense Pyramidal Optical Flow
    :align: center

    +------------------------------------------+-----------------+------------+------------+------------+
    |            Dataset                       |      LUT        |    BRAM    |     FF     |    DSP     |
    +------------+------+----------------------+                 |            |            |            |
    | Resolution | NPPC | other params         |                 |            |            |            |
    +============+======+======================+=================+============+============+============+
    |     4K     |   1  | 5 iterations,        |     30781       |    182     |    26169   |     83     |
    |            |      | 5 levels             |                 |            |            |            |
    +------------+------+----------------------+-----------------+------------+------------+------------+
    |     FHD    |   1  | 5 iterations,        |     30839       |    107     |    25714   |     83     |
    |            |      | 5 levels             |                 |            |            |            |
    +------------+------+----------------------+-----------------+------------+------------+------------+


The performance is shown below

.. table:: Table 2: Performance Numbers in terms of FPS (Frames Per Second) for Two Consecutive Frames for LK Dense Pyramidal Optical Flow
    :align: center
	
    +----------------------+--------------+--------------+
    |       Dataset        |   FPS(CPU)   |   FPS(FPGA)  |
    +======================+==============+==============+
    |     4k (3840x2160)   |     0.15     |    3         |
    +----------------------+--------------+--------------+
    |   Full HD(1920x1080) |     0.63     |    12        |
    +----------------------+--------------+--------------+

.. toctree::
    :maxdepth: 1
