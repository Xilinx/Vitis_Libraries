.. 
  .. Copyright © 2019–2024 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. toctree::
   :hidden:
   :maxdepth: 1 

.. _brief:

==================================
Vitis Ultrasound Library Tutorial
==================================

This tutorial will show how to achieve better ultrasound image quality, speed, and accuracy using AMD technology. The tutorial includes four labs:

* Lab-1: How does AMD Vitis |trade| Ultrasound Library work

* Lab-2: L1/L2 Graph based algorithm acceleration and evaluation for ultrasound tool box case

* Lab-3: L2 Graph based algorithm acceleration and evaluation for ultrasound All in AIE case

* Lab-4: L3 Graph based acceleration for ultrasound All in AIE, intergrated with PL and xrt case


Lab-1: How does Vitis Ultrasound Library work
===============================================

Vitis Ultrasound Library provides three levels of APIs, L1, L2, and L3. 

- The APIs in L1 are 2 sets, tool box APIs and All in AIE APIs (naming with kernel_*). 
- The APIs in L2 are 2 sets, tool box Graphs and All in AIE Graphs (naming with graph_*). 
- The APIs in L1/L2 support x86sim/aiesim flow and are oriented to module-level design that test in L2/tests.
- Meanwhile, L2 cases of tool box (apodization/delay/focusing/image-points/interpolator/samples/_sa) also support board-level acceleratins.
- The APIs in L3 are based on Vitis flow and are designed for board-level accelerations. 
- There are 4 L3 cases. Scanline_AllinAIE is developed with All in AIE method. PW/SA/Scanline are developed with tool box method.

Setup Environment
------------------------------------

.. code-block:: bash

   #!/bin/bash

   # setup vitis env
   source <Vitis_install_path>/Vitis/2024.2/settings64.sh 
   source <XRT_install_path>/2024.2/xbb/xrt/packages/setenv.sh
   export PLATFORM_REPO_PATHS=<path_to_platforms>

   # set petalinux related env
   export SYSROOT=<path_to_platforms>/sw/versal/xilinx-versal-common-v2024.2/sysroots/aarch64-xilinx-linux/
   export ROOTFS=<path_to_platforms>/sw/versal/xilinx-versal-common-v2024.2/rootfs.ext4
   export K_IMAGE=<path_to_platforms>/sw/versal/xilinx-versal-common-v2024.2/Image

Download the Vitis Ultrasound Library
--------------------------------------

.. code-block:: bash

   #!/bin/bash
   git clone https://github.com/Xilinx/Vitis_Libraries.git
   cd Vitis_Libraries/ultrasound


Lab-2: L1/L2 Graph based algorithm acceleration and evaluation for ultrasound tool box case
============================================================================================

Lab purpose
------------------------------------
Before using Vitis flow to build a full-function kernel running on hardware, you might want to use a relative simple flow to
estimate performance and resource consumption for some key modules in a complex algorithm. In this lab, you estimate
a easy module called 'absV,' which returns a new vector composed of the absolute value corresponding to each element of the input vector.
You finally get a libadf.a of module instead of a kernel. It can run on hardware, but it is the first step to a successful design.

Run a L1 Example
------------------------------------

.. code-block:: bash

   #!/bin/bash
   cd L2/tests/absV        # absV is an example case. Please change directory to any other cases in L1/test if interested.
   make run TARGET=x86sim  # run the x86sim
   make run TARGET=aiesim  # run the aiesim

Run a L2 Example
------------------------------------

.. code-block:: bash

   #!/bin/bash
   cd L2/tests/apodization # apodization is an example case. Please change directory to any other cases in L2/test if interested.
   make run TARGET=x86sim  # run the x86sim
   make run TARGET=aiesim  # run the aiesim
   make run TARGET=hw_emu  # run hw_emu. Build host and run hardware emulation, this step would lauch petalinux
   make all TARGET=hw      # build hw

L2 APIs Input Arguments
------------------------------------

.. code-block:: bash

   Usage: host.exe -[xclbin  data]
          -xclbin: the kernel name
          -data: the path to the input data


Lab-3: L2 Graph based algorithm acceleration and evaluation for ultrasound All in AIE case
============================================================================================

Run a L2 graph_scanline case
------------------------------------

.. code-block:: bash

   #!/bin/bash
   cd L2/tests/graph_scanline # scanline is an example case. Please change directory to any other cases in L2/test if interested.
   make run TARGET=x86sim  # run the x86sim
   make run TARGET=aiesim  # run the aiesim

Example logs of graph_scanline
------------------------------------

1. List of all the scanline parameters of Example 1 defined by you.

.. code-block:: shell 

   Scanline L2 Graph initialization by using init_img_foc()
   : Example 1 for scanline parameters
   : speed_sound     = 1540.0      m/s
   : freq_sound      = 5000000     Hz
   : Wave Length     = 0.000308000 m
   : freq_sampling   = 100000000   Hz
   : num_sample      = 2048        sample / line
   : num_line        = 41          line / image
   : num_element     = 128         elemments on transducer
   : Sampling Length = 0.000007700 m
   : Sampling Depth  = 0.007884800 m
   : Sampling Cycle  = 0.000020480 s
   : Sampling Input  = 12800.000   MSps
   : Imaging output  = 256.250     MPps
   : Imaging spf     = 83968       Pixel per frame
   : Imaging fps     = 1190.930    fps

2. List of all the four sets of intermediate results of the evolution from algorithm models to hardware.

   * module by module mode scanline (shown below)

   * line by line mode scanline

   * element by element mode scanline

   * data-unit-by data-unit mode scanline

   - The intermediate results of each mode are distinguished by file naming. 
   - For example, MbyM_L1_E128_S2048.int.col is the result of interpolation. 
   - In MbyM_L1_S2048.mul.col, L1 means 1 line result, which is used for smoking test.
   - In MbyM_L41_S2048.mul.col, L41 means 41 lines result, which is used for full test to show the png.
   - All files are in a column format.

.. code-block:: shell 

   ************************ Now performacing a scanline testing in data-unit-by data-unit mode ************************
   MODEL_TEST_SCANLINE_UbyU:  ____________________________________________________________________
   MODEL_TEST_SCANLINE_UbyU:   Algorithm | Data unit pattern |        Invoking times
   MODEL_TEST_SCANLINE_UbyU:    Modules  |  Dim1-seg-sample  | [segment] x [element] x [line]
   MODEL_TEST_SCANLINE_UbyU:  --------------------------------------------------------------------
   MODEL_TEST_SCANLINE_UbyU:    obj_img  |       [ 512]      |
   MODEL_TEST_SCANLINE_UbyU:    obj_foc  |       [ 128]      |
   MODEL_TEST_SCANLINE_UbyU:    obj_dly  |       [ 512]      |
   MODEL_TEST_SCANLINE_UbyU:    obj_apo  |       [ 512]      | x [4] x [128] x [41]
   MODEL_TEST_SCANLINE_UbyU:    obj_smp  |       [ 512]      |
   MODEL_TEST_SCANLINE_UbyU:    obj_int  |       [2048]      |
   MODEL_TEST_SCANLINE_UbyU:    obj_mul  |       [2048]      |
   MODEL_TEST_SCANLINE_UbyU:  ____________________________________________________________________
   MODEL_TEST_SCANLINE: Saving img data in file data_model/UbyU_L1_S2048_Dim0.img.col
   MODEL_TEST_SCANLINE: Saving img data in file data_model/UbyU_L1_S2048_Dim2.img.col
   MODEL_TEST_SCANLINE: Saving focusing data in file data_model/UbyU_L1_E128.foc.col
   MODEL_TEST_SCANLINE: Saving dly data in file data_model/UbyU_L1_S2048.dly.col
   MODEL_TEST_SCANLINE: Saving smp data in file data_model/UbyU_L1_E128_S2048.smp.col
   MODEL_TEST_SCANLINE: Saving inside data in file data_model/UbyU_L1_E128_S2048.ins.col
   MODEL_TEST_SCANLINE: Saving int data in file data_model/UbyU_L1_E128_S2048.int.col
   MODEL_TEST_SCANLINE: Saving apo data in file data_model/UbyU_L1_E128_S2048.apo.col
   MODEL_TEST_SCANLINE: Saving mult data in file data_model/UbyU_L1_S2048.mul.col

1. List of all AIE graph Static Parameters to check whether the AIE hardware or design restrictions are met.
   For example, num_invoking is the invoking times of AIE graph.  

.. code-block:: shell 

   ********** Static Parameter ********************
   Const: NUM_LINE_t              = 41
   Const: NUM_ELEMENT_t           = 128
   Const: NUM_SAMPLE_t            = 2048
   Const: VECDIM_foc_t            = 8
   Const: NUM_SEG_t               = 4
   Const: num_invoking            = 20992
   Const: VECDIM_img_t            = 8
   Const: LEN_OUT_img_t           = 512
   Const: LEN32b_PARA_img_t       = 7
   Const: LEN_OUT_foc_t           = 512
   Const: LEN32b_PARA_foc_t       = 6
   Const: LEN32b_PARA_delay_t     = 17
   Const: LEN_IN_delay_t          = 512
   Const: LEN_OUT_delay_t         = 512
   Const: VECDIM_delay_t          = 8
   Const: LEN_OUT_apodi_t         = 512
   Const: LEN_IN_apodi_t          = 512
   Const: LEN32b_PARA_apodi_t     = 12
   Const: VECDIM_apodi_t          = 8
   Const: LEN_IN_apodi_f_t        = 128
   Const: LEN_IN_apodi_d_t        = 512
   Const: NUM_UPSample_t          = 4
   Const: LEN_OUT_interp_t        = 2048
   Const: LEN_IN_interp_t         = 512
   Const: LEN_IN_interp_rf_t      = 2048
   Const: LEN32b_PARA_interp_t    = 9
   Const: VECDIM_interp_t         = 8
   Const: VECDIM_sample_t         = 8
   Const: LEN_IN_sample_t         = 512
   Const: LEN_OUT_sample_t        = 512
   Const: LEN32b_PARA_sample_t    = 12
   Const: VECDIM_mult_t           = 8
   Const: LEN_IN_mult_t           = 2048
   Const: LEN_OUT_mult_t          = 2048
   Const: LEN32b_PARA_mult_t      = 8

1. List of all AIE graph RTP Parameters to check whether design restrictions are met.

.. code-block:: shell 

   ********** RTP Parameter ********************
   RTP: para_mult_const           size = 32 Bytes
   RTP: para_interp_const         size = 36 Bytes
   RTP: para_amain_const          size = 48 Bytes
   RTP: para_apodi_const          size = 48 Bytes
   RTP: g_sam_para_const          size = 48 Bytes
   RTP: g_sam_para_rfdim          size = 164 Bytes
   RTP: g_sam_para_elem           size = 8 Bytes
   RTP: g_delay_para_const        size = 68 Bytes
   RTP: g_delay_t_start           size = 164 Bytes
   RTP: para_foc_const            size = 24 Bytes
   RTP: example_1_xdc_def_pos_xz  size = 2048 Bytes

1. List of the comparison result with model output.
   Only values that meet both conditions of Error judgement are considered computational errors.
   You can also open the macro #define ENABLE_DEBUGGING in L2/include/graph_scanline.hpp to generate all the intermediate results, and it is automatically checked. 

.. code-block:: shell 

   ***********     Comparison   ************
   *********** File 1                           : x86simulator_output/data/mult_All.txt
   *********** File 2                           : data_model/UbyU_L41_S2048.mul.col
   *********** Error judgement                  : diff_abs > 1.000000e-06 && diff_ratio > 1.000000e-04
   *********** Total tested data number         : 8192
   *********** Total passed data number         : 8192      100.00%
   ***********       complete same data number  : 4327      52.82%
   ***********       complete zero data number  : 4226      51.59%
   *********** Absolute Errors distributions   ************
   *********** Checked data range of First file : [-2.935699e+05, 3.438067e+05]
   *********** Checked data range of Second file: [-2.935699e+05, 3.438067e+05]
   Differences range from [ 10^-6 to 10^-5 )    :   494     6.03%
   Differences range from [ 10^-5 to 10^-4 )    :   1651    20.15%
   Differences range from [ 10^-4 to 10^-3 )    :   1028    12.55%
   Differences range from [ 10^-3 to 10^-2 )    :   251     3.06%
   Differences range from [ 10^-2 to 10^-1 )    :   172     2.10%
   Differences range from [ 10^-1 to 10^ 0 )    :   52      0.63%
   Differences range from [ 10^ 0 to 10^ 1 )    :   0       0.00%
   Differences range from [ 10^ 1 to 10^ 2 )    :   0       0.00%
   Differences range from [ 10^ 2 to 10^ 3 )    :   0       0.00%
   Differences range from [ 10^ 3 to 10^ 4 )    :   0       0.00%
   Differences range from [ 10^ 4 to 10^ 5 )    :   0       0.00%
   Differences range from [ 10^ 5 to 10^ 6 )    :   0       0.00%
   Differences range from [ 10^ 6 to 10^ 7 )    :   0       0.00%
   Differences range from [ 10^ 7 to 10^ 8 )    :   0       0.00%
   Differences range from [ 10^ 8 to 10^ 9 )    :   0       0.00%
   Differences range from [ 10^ 9 to 10^10 )    :   0       0.00%
   Differences range from [ 10^10 to 10^11 )    :   0       0.00%
   Differences range from [ 10^11 to 10^12 )    :   0       0.00%
   Differences range from [ 10^12 to 10^13 )    :   0       0.00%
   INFO [HLS SIM]: The maximum depth reached by any hls::stream() instance in the design is 2048
   Simulation completed successfully returning zero
         

Lab-4: L3 Graph based acceleration for ultrasound All in AIE, integrated with PL and xrt case
===============================================================================================

Lab purpose
------------------------------------
The ultrasound might be based on an open source project. This lab shows an accelerated process based on an open- 
source project, the classic Scanline method. Scanline method is not only more complex, but also involves AIE/PL/SW partition.
Scanline method is all in AIE in this case.
To learn:

- How to run L3 cases
- How to map customerized parameters to this L3 to get board-level cases.
- L3 accelerated process intergrated with AIE, PL and xrt

Run L3 All in AIE cases
------------------------------------

.. code-block:: bash

   #!/bin/bash
   cd L3/demos/scanline_AllinAIE  # 1. scanline_AllinAIE 2. plane_wave are example cases. Please change directory to any other cases in L3/test if interested.
   make run TARGET=hw_emu  # run hw_emu
   make all TARGET=hw      # build hw

L3 APIs Input Arguments
------------------------------------

.. code-block:: bash

   Usage: host.exe -[xclbin  data]
          -xclbin: the kernel name
          -data: the path to the input data

Example logs of scanline_AllinAIE
------------------------------------

1. List of all the scanline parameters of Example 1 defined by you.

- You can easily set or modify the parameters of their target case in L1/include/us_example_parameter.hpp
- Then generate corresponding board-level cases through L3.


.. code-block:: shell 

   : Example 1 for scanline parameters
   : speed_sound     = 1540.0      m/s
   : freq_sound      = 5000000     Hz
   : Wave Length     = 0.000308000 m
   : freq_sampling   = 100000000   Hz
   : num_sample      = 2048        sample / line
   : num_line        = 41          line / image
   : num_element     = 128         elemments on transducer
   : Sampling Length = 0.000007700 m
   : Sampling Depth  = 0.007884800 m
   : Sampling Cycle  = 0.000020480 s
   : Sampling Input  = 12800.000   MSps
   : Imaging output  = 256.250     MPps
   : Imaging spf     = 83968       Pixel per frame
   : Imaging fps     = 1190.930    fps

2. List of all the four sets of intermediate results of the evolution from algorithm models to hardware.

   * module by module mode scanline (shown below)

   * line by line mode scanline

   * element by element mode scanline

   * data-unit-by data-unit mode scanline

   - The intermediate results of each mode are distinguished by file naming. 
   - For example, MbyM_L1_E128_S2048.int.col is the result of interpolation. 
   - In MbyM_L1_S2048.mul.col, L1 means 1 line result, which is used for smoking test.
   - In MbyM_L41_S2048.mul.col, L41 means 41 lines result, which is used for full test to show the png.
   - All files are in a column format.

.. code-block:: shell 

   ************************ Now performacing a scanline testing in module by module mode ************************
   MODEL_TEST_SCANLINE_MbyM:  ______________________________________________________________________
   MODEL_TEST_SCANLINE_MbyM:   Algorithm |               Data unit pattern           | Invoking times
   MODEL_TEST_SCANLINE_MbyM:    Modules  |  Dim1-element   Dim2-sample   Dim3-line   |
   MODEL_TEST_SCANLINE_MbyM:  ---------------------------------------------------------------------
   MODEL_TEST_SCANLINE_MbyM:    obj_img  |                     [2048]     x    [41]  |
   MODEL_TEST_SCANLINE_MbyM:    obj_foc  |  [ 128]                                   |
   MODEL_TEST_SCANLINE_MbyM:    obj_dly  |              x      [2048]     x    [41]  |
   MODEL_TEST_SCANLINE_MbyM:    obj_apo  |  [ 128]      x      [2048]     x    [41]  |   x [1]
   MODEL_TEST_SCANLINE_MbyM:    obj_smp  |  [ 128]      x      [2048]     x    [41]  |
   MODEL_TEST_SCANLINE_MbyM:    obj_int  |  [ 128]      x      [8192]     x    [41]  |
   MODEL_TEST_SCANLINE_MbyM:    obj_mul  |  [ 128]      x      [8192]     x    [41]  |
   MODEL_TEST_SCANLINE_MbyM:  ______________________________________________________________________
   MODEL_TEST_SCANLINE: Saving img data in file data_model/MbyM_L1_S2048_Dim0.img.col
   MODEL_TEST_SCANLINE: Saving img data in file data_model/MbyM_L1_S2048_Dim2.img.col
   MODEL_TEST_SCANLINE: Saving focusing data in file data_model/MbyM_L1_E128.foc.col
   MODEL_TEST_SCANLINE: Saving dly data in file data_model/MbyM_L1_S2048.dly.col
   MODEL_TEST_SCANLINE: Saving smp data in file data_model/MbyM_L1_E128_S2048.smp.col
   MODEL_TEST_SCANLINE: Saving inside data in file data_model/MbyM_L1_E128_S2048.ins.col
   MODEL_TEST_SCANLINE: Saving int data in file data_model/MbyM_L1_E128_S2048.int.col
   MODEL_TEST_SCANLINE: Saving apo data in file data_model/MbyM_L1_E128_S2048.apo.col
   MODEL_TEST_SCANLINE: Saving mult data in file data_model/MbyM_L1_S2048.mul.col
   MODEL_TEST_SCANLINE: Saving mult data in file data_model/MbyM_L41_S2048.mul.col

1. List of all AIE graph Static Parameters to check whether the AIE hardware or design restrictions are met.
   For example, num_invoking is the invoking times of AIE graph.  

.. code-block:: shell 

   ********** Static Parameter ********************
   Const: NUM_LINE_t              = 41
   Const: NUM_ELEMENT_t           = 128
   Const: NUM_SAMPLE_t            = 2048
   Const: VECDIM_foc_t            = 8
   Const: NUM_SEG_t               = 4
   Const: num_invoking            = 20992
   Const: VECDIM_img_t            = 8
   Const: LEN_OUT_img_t           = 512
   Const: LEN32b_PARA_img_t       = 7
   Const: LEN_OUT_foc_t           = 512
   Const: LEN32b_PARA_foc_t       = 6
   Const: LEN32b_PARA_delay_t     = 17
   Const: LEN_IN_delay_t          = 512
   Const: LEN_OUT_delay_t         = 512
   Const: VECDIM_delay_t          = 8
   Const: LEN_OUT_apodi_t         = 512
   Const: LEN_IN_apodi_t          = 512
   Const: LEN32b_PARA_apodi_t     = 12
   Const: VECDIM_apodi_t          = 8
   Const: LEN_IN_apodi_f_t        = 128
   Const: LEN_IN_apodi_d_t        = 512
   Const: NUM_UPSample_t          = 4
   Const: LEN_OUT_interp_t        = 2048
   Const: LEN_IN_interp_t         = 512
   Const: LEN_IN_interp_rf_t      = 2048
   Const: LEN32b_PARA_interp_t    = 9
   Const: VECDIM_interp_t         = 8
   Const: VECDIM_sample_t         = 8
   Const: LEN_IN_sample_t         = 512
   Const: LEN_OUT_sample_t        = 512
   Const: LEN32b_PARA_sample_t    = 12
   Const: VECDIM_mult_t           = 8
   Const: LEN_IN_mult_t           = 2048
   Const: LEN_OUT_mult_t          = 2048
   Const: LEN32b_PARA_mult_t      = 8

1. List of all AIE graph RTP Parameters to check whether design restrictions are met.

.. code-block:: shell 

   ********** RTP Parameter ********************
   RTP: para_mult_const           size = 32 Bytes
   RTP: para_interp_const         size = 36 Bytes
   RTP: para_amain_const          size = 48 Bytes
   RTP: para_apodi_const          size = 48 Bytes
   RTP: g_sam_para_const          size = 48 Bytes
   RTP: g_sam_para_rfdim          size = 164 Bytes
   RTP: g_sam_para_elem           size = 8 Bytes
   RTP: g_delay_para_const        size = 68 Bytes
   RTP: g_delay_t_start           size = 164 Bytes
   RTP: para_foc_const            size = 24 Bytes
   RTP: example_1_xdc_def_pos_xz  size = 2048 Bytes

5. List of the md5sum of result and the comparison result with model output.
   Only values that meet both conditions of Error judgement are considered computational errors.
   Also, the output of mult could be converted to png format use the command like 
   "python L3/models/data2png.py L3/tests/scanline_AllinAIE/package_hw_emu/sd_card/data/xf_output_res.txt".

.. code-block:: shell 

   8fce71b2b4cab289c1b12ab841f05aac  data/xf_output_res.bin
   e93ae81a2de1a855b2a82e22159e4005  data/xf_output_res.txt
   [HOST]: model result in data_model/UbyU_L41_S2048.mul.col
   ***********     Comparison   ************
   *********** File 1                           : data/xf_output_res.txt
   *********** File 2                           : data_model/UbyU_L41_S2048.mul.col
   *********** Error judgement                  : diff_abs > 1.000000e-05 && diff_ratio > 1.000000e-03
   *********** Total tested data number         : 335872
   *********** Total passed data number         : 335872    100.00%
   ***********       complete same data number  : 231982    69.07%
   ***********       complete zero data number  : 229454    68.32%
   *********** Absolute Errors distributions   ************
   *********** Checked data range of First file : [-6.625977e+06, 8.221938e+06]
   *********** Checked data range of Second file: [-6.625977e+06, 8.221938e+06]
   Differences range from [ 10^-5 to 10^-4 )    :   29375   8.75%
   Differences range from [ 10^-4 to 10^-3 )    :   30489   9.08%
   Differences range from [ 10^-3 to 10^-2 )    :   12572   3.74%
   Differences range from [ 10^-2 to 10^-1 )    :   9427    2.81%
   Differences range from [ 10^-1 to 10^ 0 )    :   5144    1.53%
   Differences range from [ 10^ 0 to 10^ 1 )    :   1682    0.50%
   Differences range from [ 10^ 1 to 10^ 2 )    :   0   0.00%
   Differences range from [ 10^ 2 to 10^ 3 )    :   0   0.00%
   Differences range from [ 10^ 3 to 10^ 4 )    :   0   0.00%
   Differences range from [ 10^ 4 to 10^ 5 )    :   0   0.00%
   Differences range from [ 10^ 5 to 10^ 6 )    :   0   0.00%
   Differences range from [ 10^ 6 to 10^ 7 )    :   0   0.00%
   Differences range from [ 10^ 7 to 10^ 8 )    :   0   0.00%
   Differences range from [ 10^ 8 to 10^ 9 )    :   0   0.00%
   Differences range from [ 10^ 9 to 10^10 )    :   0   0.00%
   Differences range from [ 10^10 to 10^11 )    :   0   0.00%
   Differences range from [ 10^11 to 10^12 )    :   0   0.00%
   Differences range from [ 10^12 to 10^13 )    :   0   0.00%
   Differences range from [ 10^13 to 10^14 )    :   0   0.00%
   [HOST]: total error 0
   Releasing remaining XRT objects...
   [HOST]: success!


The log below shows the log generated by the Planewave method, using tool box modules.

Example logs of plane_wave
------------------------------------

.. code-block:: shell 

   INFO: Initializing ADF API...
   INFO: XAIEFAL: INFO: Resource group Avail is created.
   INFO: XAIEFAL: INFO: Resource group Static is created.
   INFO: XAIEFAL: INFO: Resource group Generic is created.
   INFO: file:data/start_positions.txt size:64
   INFO: file:data/directions.txt size:64
   INFO: file:data/samples_arange.txt size:128
   INFO: file:data/image_points.txt size:128
   INFO: file:data/image_points.txt size:128
   INFO: file:data/tx_def_delay_distance.txt size:128
   INFO: file:data/tx_def_delay_distance.txt size:128
   INFO: file:data/tx_def_ref_point.txt size:128
   INFO: file:data/tx_def_focal_point.txt size:128
   INFO: file:data/t_start.txt size:128
   INFO: file:data/apo_ref_0.txt size:64
   INFO: file:data/xdc_def_0.txt size:64
   INFO: file:data/apo_ref_1.txt size:64
   INFO: file:data/xdc_def_1.txt size:64
   INFO: file:data/image_points.txt size:128
   INFO: file:data/delay_from_PL.txt size:206
   INFO: file:data/xdc_def_positions.txt size:130
   INFO: file:data/sampling_frequency.txt size:136
   INFO: file:data/image_points.txt size:128
   INFO: file:data/apodization_reference.txt size:156
   INFO: file:data/apo_distance_k.txt size:128
   INFO: file:data/F_number.txt size:188
   INFO: file:data/P1.txt size:32
   INFO: file:data/P2.txt size:32
   INFO: file:data/P3.txt size:32
   INFO: file:data/P4.txt size:32
   INFO: file:data/P5.txt size:32
   INFO: file:data/P6.txt size:32
   INFO: Input memory0 virtual addr 0x0xffffac8bc000
   INFO: Input memory1 virtual addr 0x0xffffac8bb000
   INFO: Input memory2 virtual addr 0x0xffffac8ba000
   INFO: Input memory3 virtual addr 0x0xffffac8b9000
   INFO: Input memory4 virtual addr 0x0xffff8ee67000
   INFO: Input memory5 virtual addr 0x0xffff8ee66000
   INFO: Input memory6 virtual addr 0x0xffff8ee65000
   INFO: Input memory7 virtual addr 0x0xffff8ee64000
   INFO: Input memory8 virtual addr 0x0xffff8ee63000
   INFO: Input memory9 virtual addr 0x0xffff8ee62000
   INFO: Input memory10 virtual addr 0x0xffff8ee61000
   INFO: Input memory11 virtual addr 0x0xffff8ee60000
   INFO: Input memory12 virtual addr 0x0xffff8ee5f000
   INFO: Input memory13 virtual addr 0x0xffff8ee5e000
   INFO: Input memory14 virtual addr 0x0xffff8ee5d000
   INFO: Input memory15 virtual addr 0x0xffff8ee5c000
   INFO: Input memory16 virtual addr 0x0xffff8ee5b000
   INFO: Input memory17 virtual addr 0x0xffff8ee5a000
   INFO: Input memory18 virtual addr 0x0xffff8ee59000
   INFO: Input memory19 virtual addr 0x0xffff8ee58000
   INFO: Input memory20 virtual addr 0x0xffff8ee57000
   INFO: Input memory21 virtual addr 0x0xffff8ee56000
   INFO: Input memory22 virtual addr 0x0xffff8ee55000
   INFO: Input memory23 virtual addr 0x0xffff8ee54000
   INFO: Input memory24 virtual addr 0x0xffff8ee53000
   INFO: Input memory25 virtual addr 0x0xffff8ee52000
   INFO: Input memory26 virtual addr 0x0xffff8ee51000
   INFO: Input memory27 virtual addr 0x0xffff8ee50000
   INFO: Output memory0 virtual addr 0x 0xffff8ee4f000
   INFO: Output memory1 virtual addr 0x 0xffff8ee4e000
   INFO: Output memory2 virtual addr 0x 0xffff8ee4d000
   INFO: Output memory3 virtual addr 0x 0xffff8ee4c000
   INFO: Output memory4 virtual addr 0x 0xffff8ee4b000
   INFO: Output memory5 virtual addr 0x 0xffff8ee4a000
   INFO: input kernel complete
   INFO: output kernel complete
   INFO: graph init
   INFO: graph run
   INFO: graph end
   INFO: mm2s1 completed with status(4)
   INFO: mm2s2 completed with status(4)
   INFO: mm2s3 completed with status(4)
   INFO: mm2s4 completed with status(4)
   INFO: mm2s5 completed with status(4)
   INFO: mm2s6 completed with status(4)
   INFO: mm2s7 completed with status(4)
   INFO: mm2s8 completed with status(4)
   INFO: mm2s9 completed with status(4)
   INFO: mm2s10 completed with status(4)
   INFO: mm2s11 completed with status(4)
   INFO: mm2s12 completed with status(4)
   INFO: mm2s13 completed with status(4)
   INFO: mm2s14 completed with status(4)
   INFO: mm2s15 completed with status(4)
   INFO: mm2s16 completed with status(4)
   INFO: mm2s17 completed with status(4)
   INFO: mm2s18 completed with status(4)
   INFO: mm2s19 completed with status(4)
   INFO: mm2s20 completed with status(4)
   INFO: mm2s21 completed with status(4)
   INFO: mm2s22 completed with status(4)
   INFO: mm2s23 completed with status(4)
   INFO: mm2s24 completed with status(4)
   INFO: mm2s25 completed with status(4)
   INFO: mm2s26 completed with status(4)
   INFO: mm2s27 completed with status(4)
   INFO: mm2s28 completed with status(4)
   INFO: mm2s wait complete
   INFO: s2mm1completed with status(4)
   INFO: s2mm2completed with status(4)
   INFO: s2mm3completed with status(4)
   INFO: s2mm4completed with status(4)
   INFO: s2mm5completed with status(4)
   INFO: s2mm6completed with status(4)
   INFO: s2mm wait compete
   INFO: file:data/golden/image_points.txt size:128
   INFO: file:data/golden/delay_to_PL.txt size:32
   INFO: file:data/golden/focusing_output.txt size:32
   INFO: file:data/golden/samples_to_PL.txt size:32
   INFO: file:data/golden/apodization.txt size:32
   INFO: file:data/golden/C.txt size:128
   INFO: data/golden/image_points.txt
   INFO: data/golden/delay_to_PL.txt
   INFO: data/golden/focusing_output.txt
   INFO: data/golden/samples_to_PL.txt
   INFO: data/golden/apodization.txt
   INFO: data/golden/C.txt
   INFO: Releasing remaining XRT objects...
   INFO: Test Done, err_cnt:0
   INFO: INFO: TEST PASSED, RC=0
   INFO: INFO: Embedded host run completed.
   INFO: 
   INFO: Total test time: 12.821966409683228
   INFO: Power OFF complete.
   INFO: Test passed!
   INFO: Total time on board: 716.9993252754211

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
