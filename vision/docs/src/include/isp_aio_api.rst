.. 
   Copyright 2023 Advanced Micro Devices, Inc
  
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

ISP all_in_one Pipeline:
=========================

The ISP all_in_one pipeline contains all the necessary functions that
will enable the user to test several combinations of the image sensor
processing pipeline. In this specific ISP pipeline version, optional
modules can be enabled or disabled using compile-time parameters.

This ISP pipeline includes 19 modules, as follows:

-  **Extract Exposure Frames:** The Extract Exposure Frames module returns
   the Short Exposure Frame and Long Exposure Frame from the input frame
   using the Digital overlap parameter.

-  **HDR Merge:** The HDR Merge module generates the High Dynamic Range
   image from a set of different exposure frames. Usually, image sensors
   have limited dynamic range and it’s difficult to get HDR image with
   single image capture. From the sensor, the frames are collected with
   different exposure times and will get different exposure frames.
   HDR Merge will generate the HDR frame with those exposure frames.

-  **HDR Decompand:** This module decompands or decompresses a piecewise 
   linear (PWL) companded data. Companding is performed in image sensors
   not capable of high bitwidth during data transmission. This decompanding
   module supports Bayer raw data with four knee point PWL mapping and equations
   are provided for 12-bit to 16-bit conversion.    

-  **RGBIR to Bayer (RGBIR):** This module converts the input image with 
   R, G, B, IR pixel data into a standard Bayer pattern image along with 
   a full IR data image. 

-  **Auto Exposure Compensation (AEC):** This module automatically 
   attempts to correct the exposure level of the captured image and also 
   improve the contrast of the image.

-  **Black Level Correction (BLC):** This module corrects the black and white 
   levels of the overall image. Black level leads to the whitening of
   image in dark regions and perceived loss of overall contrast. 
   
-  **Bad Pixel Correction (BPC):** This module removes defective/bad pixels 
   from an image sensor resulting from manufacturing faults or variations 
   in pixel voltage levels based on temperature or exposure.
 

-  **Degamma:** This module linearizes the input from the sensor in order to facilitate ISP processing that operates on the linear domain. 

-  **Lens Shading Correction (LSC):** This module corrects the darkening toward 
   the edge of the image caused by camera lens limitations. This darkening 
   effect is also known as vignetting. 

-  **Gain Control:** This module improves the overall brightness of the image.

-  **Demosaicing:** This module reconstructs RGB pixels from the input Bayer 
   image (RGGB, BGGR, RGBG, GRGB).

-  **Auto White Balance (AWB):** This module improves the color balance of the
   image by using image statistics.

-  **Color Correction Matrix (CCM):** This module converts the input image 
   color format to the output image color format using the Color Correction Matrix 
   provided by the user (CCM_TYPE).

-  **Quantization & Dithering (QnD):** This module is a tone-mapper that 
   dithers the input image using the Floyd-Steinberg dithering method. It is commonly 
   used by image manipulation software, for example when an image is converted 
   into GIF format each pixel intensity value is quantized to 8 bits i.e., 256
   colors.

-  **Global Tone Mapping (GTM):** This module is a tone-mapper that reduces 
   the dynamic range from higher range to display range using tone mapping.
   
-  **Local Tone Mapping (LTM):** This module is a tone-mapper that takes pixel
   neighbor statistics into account and produces images with more contrast and 
   brightness.

-  **Gamma Correction:** This module improve the overall brightness of the image. 

-  **3DLUT:** The 3D LUT module operates on three independent parameters. 
   This drastically increases the number of mapped indexes to value pairs. 
   For example, a combination of 3 individual 1D LUTs can map 2^n \* 3 values 
   where n is the bit depth, whereas a 3D LUT processing 3 channels will have
   2^n \* 2^n \* 2^n possible values.
   
-  **Color Space Conversion (CSC):** The CSS module converts an RGB image to 
   a YUV422(YUYV) image for HDMI display purposes. RGB2YUYV converts the RGB image 
   into Y channel for every pixel and U and V for alternating pixels.


.. image:: ./images/ISP_AIO_Diagram.PNG
   :class: image 
   :width: 1000 
      
.. table:: Table: Runtime Parameters for the Pipeline

    +-------------------------+-----------------------------------+
    | **Parameter**           | **Descriptions**                  |
    +=========================+===================================+
    | height                  | The number of rows in the image   |
    |                         | or height of the image.           |
    +-------------------------+-----------------------------------+
    | width                   | The number of columns in the      |
    |                         | image or width of the image.      |
    +-------------------------+-----------------------------------+
    | dcp_params_16to12       | Params to converts the 16bit      |
    |                         | input image bit depth to 12bit.   |
    +-------------------------+-----------------------------------+
    | dcp_params_12to16       | Params to converts the 12bit      |
    |                         | input image bit depth to 16bit.   |
    +-------------------------+-----------------------------------+
    | wr_hls                  | Lookup table for weight values.   | 
    |                         | Computing the weights LUT in host |
    |                         | side and passing as input to the  |
    |                         | function.                         |
    +-------------------------+-----------------------------------+
    | rgain                   | To configure gain value for the   |
    |                         | red channel.                      |
    +-------------------------+-----------------------------------+
    | bgain                   | To configure gain value for the   |
    |                         | blue channel.                     |
    +-------------------------+-----------------------------------+
    | R_IR_C1_wgts            | 5x5 Weights to calculate R at IR  |
    |                         | location for constellation1.      |
    +-------------------------+-----------------------------------+
    | R_IR_C2_wgts            | 5x5 Weights to calculate R at IR  |
    |                         | location for constellation2.      |
    +-------------------------+-----------------------------------+
    | B_at_R_wgts             | 5x5 Weights to calculate B at R   |
    |                         | location.                         |
    +-------------------------+-----------------------------------+
    | IR_at_R_wgts            | 3x3 Weights to calculate IR at R  |
    |                         | location.                         |
    +-------------------------+-----------------------------------+
    | IR_at_B_wgts            | 3x3 Weights to calculate IR at B  |
    |                         | location.                         |
    +-------------------------+-----------------------------------+
    | sub_wgts                | Weights to perform weighted       |
    |                         | subtraction of IR image from RGB  |
    |                         | image. sub_wgts[0] -> G Pixel,    |
    |                         | sub_wgts[1] -> R Pixel,           |
    |                         | sub_wgts[2] -> B Pixel            |
    |                         | sub_wgts[3] -> calculated B Pixel |
    +-------------------------+-----------------------------------+
    | dgam_params             | Array containing upper limit,     |
    |                         | slope and intercept of linear     |
    |                         | equations for Red, Green and      |
    |                         | Blue colour.                      |
    +-------------------------+-----------------------------------+
    | pawb                    | %top and %bottom pixels are       |
    |                         | ignored while computing min and   |
    |                         | max to improve quality.           |
    +-------------------------+-----------------------------------+
    | paec                    | %top and %bottom pixels are       |
    |                         | ignored while computing min and   |
    |                         | max to improve quality.           |
    +-------------------------+-----------------------------------+
    | blk_height              | Actual block height.              |
    +-------------------------+-----------------------------------+
    | blk_width               | Actual block width.               |
    +-------------------------+-----------------------------------+
    | c1                      | To retain the details in bright   |
    |                         | area using, c1 in the tone        |
    |                         | mapping.                          |
    +-------------------------+-----------------------------------+
    | c2                      | Efficiency factor, ranges from    |
    |                         | 0.5 to 1 based on output device   |
    |                         | dynamic range.                    |
    +-------------------------+-----------------------------------+
    | gamma_lut               | Lookup table for gamma values.    |
    |                         | First 256 will be R, next 256     |
    |                         | values are G and last 256 values  |
    |                         | are B.                            |
    +-------------------------+-----------------------------------+
    | lutDim                  | Dimension of input LUT.           |
    +-------------------------+-----------------------------------+

.. table:: Table: Compile Time Flags 

    +------------------+-----------------------------------+
    | **Parameter**    | **Description**                   |
    +==================+===================================+
    | USE_HDR_FUSION   | Flag to enable or disable HDR     |
    |                  | fusion module.                    |
    +------------------+-----------------------------------+
    | USE_HDR          | Flag to enable or disable HDR     |
    |                  | module.                           |
    +------------------+-----------------------------------+
    | USE_GTM          | Flag to enable or disable GTM     |
    |                  | module.                           |
    +------------------+-----------------------------------+
    | USE_LTM          | Flag to enable or disable LTM     |
    |                  | module.                           |
    +------------------+-----------------------------------+
    | USE_QND          | Flag to enable or disable QND     |
    |                  | module.                           |
    +------------------+-----------------------------------+
    | USE_RGBIR        | Flag to enable or disable RGBIR   |
    |                  | module.                           |
    +------------------+-----------------------------------+
    | USE_3DLUT        | Flag to enable or disable 3DLUT   |
    |                  | module.                           |
    +------------------+-----------------------------------+
    | USE_DEGAMMA      | Flag to enable or disable Degamma |
    |                  | module.                           |
    +------------------+-----------------------------------+
    | USE_AEC          | Flag to enable or disable AEC     |
    |                  | module.                           |
    +------------------+-----------------------------------+ 
    | USE_AWB          | Flag to enable or disable AWB     |
    |                  | module.                           |
    +------------------+-----------------------------------+ 
    | USE_CCM          | Flag to enable or disable CCM     |
    |                  | module.                           |
    +------------------+-----------------------------------+ 
    | USE_CSC          | Flag to enable or disable CSC     |
    |                  | module.                           |
    +------------------+-----------------------------------+
   

.. table:: Table: Compile Time Parameter

    +-------------------------+-----------------------------------+
    | **Parameter**           | **Description**                   |
    +=========================+===================================+
    | XF_HEIGHT               | Maximum height of input and       |
    |                         | output image.                     |
    +-------------------------+-----------------------------------+
    | XF_WIDTH                | Maximum width of input and output |
    |                         | image.                            |
    +-------------------------+-----------------------------------+
    | XF_BAYER_PATTERN        | The Bayer format of the RAW input |
    |                         | image. Using XF_BAYER_RG format.  |
    +-------------------------+-----------------------------------+
    | IN_TYPE                 | Input pixel type. Supported pixel |
    |                         | width is 8,10,12,16.              |
    +-------------------------+-----------------------------------+
    | XF_GTM_T                | Tonemapping pixel type. Supported |
    |                         | pixel width is 8.                 |
    +-------------------------+-----------------------------------+
    | OUT_TYPE                | Output pixel type. Supported      |
    |                         | pixel width is 8,10,12,16.        |
    +-------------------------+-----------------------------------+
    | DGAMMA_KP               | Configurable number of knee       |
    |                         | points in degamma.                |
    +-------------------------+-----------------------------------+ 
    | SQLUTDIM                | Squared value of maximum          |
    |                         | dimension of input LUT.           |
    +-------------------------+-----------------------------------+
    | LUTDIM                  | 33x33 dimension of input LUT.     |
    +-------------------------+-----------------------------------+
    | BLOCK_WIDTH             | Maximum block width the image is  |
    |                         | divided into. This can be any     |
    |                         | positive integer greater than or  |
    |                         | equal to 32 and less than input   |
    |                         | image width.                      |
    +-------------------------+-----------------------------------+
    | BLOCK_HEIGHT            | Maximum block height the image is |
    |                         | divided into. This can be any     |
    |                         | positive integer greater than or  |
    |                         | equal to 32 and less than input   |
    |                         | image height.                     |
    +-------------------------+-----------------------------------+
    | XF_NPPCX                | Number of pixels processed per    |
    |                         | cycle.                            |
    +-------------------------+-----------------------------------+
    | NO_EXPS                 | Number of exposure frames to be   |
    |                         | merged in the module.             |
    +-------------------------+-----------------------------------+
    | W_B_SIZE                | W_B_SIZE is used to define the    |
    |                         | array size for storing the weight |
    |                         | values for wr_hls.                |
    |                         | W_B_SIZE should be 2^bit depth.   |
    +-------------------------+-----------------------------------+
    | SIN_CHANNEL_IN_TYPE     | Single channel type. It's pixel   | 
    |                         | value is XF_8UC1                  |
    +-------------------------+-----------------------------------+
    | AEC_SIN_CHANNEL_TYPE    | Single channel type. It's pixel   | 
    |                         | value is XF_16UC1                 |
    +-------------------------+-----------------------------------+
    | WB_TYPE                 | White balance type. Supported     |
    |                         | types are Gray world and simple.  |
    +-------------------------+-----------------------------------+



The following example demonstrates the top-level ISP pipeline:

.. code:: c

            void ISPPipeline_accel(ap_uint<INPUT_PTR_WIDTH>* img_inp,           /* Array2xfMat */
                       ap_uint<OUTPUT_PTR_WIDTH>* img_out,          /* xfMat2Array */
                       ap_uint<OUTPUT_PTR_WIDTH>* img_out_ir,       /* xfMat2Array */
                       int height,                                  /* Height of the image */
                       int width,                                   /* Width of the image */
                       short wr_hls[NO_EXPS * XF_NPPCX * W_B_SIZE], /* HDR */
                       int params_decompand[3][4][3],               /* Decompand */
                       char R_IR_C1_wgts[25],                       /* rgbir2bayer */
                       char R_IR_C2_wgts[25],                       /* rgbir2bayer */
                       char B_at_R_wgts[25],                        /* rgbir2bayer */
                       char IR_at_R_wgts[9],                        /* rgbir2bayer */
                       char IR_at_B_wgts[9],                        /* rgbir2bayer */
                       char sub_wgts[4],                            /* rgbir2bayer */
                       uint16_t pawb,         /* used to calculate thresh which is used in function_awb */
                       unsigned short bayerp, /* hdr_decompand, degamma */
                       uint32_t params_degamma[3][DEGAMMA_KP][3], /*degamma*/
                       uint16_t rgain,                            /* gaincontrol */
                       uint16_t bgain,                            /* gaincontrol */
                       int blk_height,                            /* LTM */
                       int blk_width,                             /* LTM */
                       uint32_t c1,                               /* gtm */
                       uint32_t c2,                               /* gtm */
                       unsigned char gamma_lut[256 * 3],          /* gammacorrection */
                       ap_uint<LUT_PTR_WIDTH>* lut,               /* lut3d */
                       int lutDim,
                       signed int ccm_config_1[3][3],
                       signed int ccm_config_2[3],
                       unsigned short ggain) { /* lut3d */
                                               // clang-format off

            #pragma HLS INTERFACE m_axi port=img_inp          offset=slave bundle=gmem1 
            #pragma HLS INTERFACE m_axi port=img_out          offset=slave bundle=gmem2
            #pragma HLS INTERFACE m_axi port=img_out_ir       offset=slave bundle=gmem3
            #pragma HLS INTERFACE m_axi port=wr_hls           offset=slave bundle=gmem4
            #pragma HLS INTERFACE m_axi port=params_decompand offset=slave bundle=gmem5
            #pragma HLS INTERFACE m_axi port=R_IR_C1_wgts     offset=slave bundle=gmem6
            #pragma HLS INTERFACE m_axi port=R_IR_C2_wgts     offset=slave bundle=gmem7
            #pragma HLS INTERFACE m_axi port=B_at_R_wgts      offset=slave bundle=gmem8
            #pragma HLS INTERFACE m_axi port=IR_at_R_wgts     offset=slave bundle=gmem9
            #pragma HLS INTERFACE m_axi port=IR_at_B_wgts     offset=slave bundle=gmem10
            #pragma HLS INTERFACE m_axi port=sub_wgts         offset=slave bundle=gmem11
            #pragma HLS INTERFACE m_axi port=params_degamma   offset=slave bundle=gmem12
            #pragma HLS INTERFACE m_axi port=gamma_lut        offset=slave bundle=gmem13
            #pragma HLS INTERFACE m_axi port=lut              offset=slave bundle=gmem14
            #pragma HLS INTERFACE m_axi port=ccm_config_1     bundle=gmem15 offset=slave
            #pragma HLS INTERFACE m_axi port=ccm_config_2     bundle=gmem16 offset=slave

            #pragma HLS ARRAY_PARTITION variable=hist0_awb    complete dim=1
            #pragma HLS ARRAY_PARTITION variable=hist1_awb    complete dim=1
            #pragma HLS ARRAY_PARTITION variable=omin dim=1   complete
            #pragma HLS ARRAY_PARTITION variable=omin dim=2   cyclic factor=2
            #pragma HLS ARRAY_PARTITION variable=omin dim=3   cyclic factor=2
            #pragma HLS ARRAY_PARTITION variable=omax dim=1   complete
            #pragma HLS ARRAY_PARTITION variable=omax dim=2   cyclic factor=2
            #pragma HLS ARRAY_PARTITION variable=omax dim=3   cyclic factor=2
               // clang-format on

               static short wr_hls_tmp[NO_EXPS * XF_NPPCX * W_B_SIZE];

            WR_HLS_INIT_LOOP:
               for (int k = 0; k < XF_NPPCX; k++) {
            // clang-format off
            #pragma HLS LOOP_TRIPCOUNT min=XF_NPPCX max=XF_NPPCX
                  // clang-format on
                  for (int i = 0; i < NO_EXPS; i++) {
            // clang-format off
            #pragma HLS LOOP_TRIPCOUNT min=NO_EXPS max=NO_EXPS
                        // clang-format on
                        for (int j = 0; j < (W_B_SIZE); j++) {
            // clang-format off
            #pragma HLS LOOP_TRIPCOUNT min=W_B_SIZE max=W_B_SIZE
                           // clang-format on
                           wr_hls_tmp[(i + k * NO_EXPS) * W_B_SIZE + j] = wr_hls[(i + k * NO_EXPS) * W_B_SIZE + j];
                        }
                  }
               }

               if (!flag) {
                  ISPpipeline(img_inp, img_out, img_out_ir, height, width, wr_hls_tmp, params_decompand, R_IR_C1_wgts,
                              R_IR_C2_wgts, B_at_R_wgts, IR_at_R_wgts, IR_at_B_wgts, sub_wgts, hist0_aec, hist1_aec, pawb, bayerp,
                              params_degamma, rgain, bgain, hist0_awb, hist1_awb, igain_0, igain_1, omin[0], omax[0], omin[1],
                              omax[1], blk_height, blk_width, mean2, mean1, L_max2, L_max1, L_min2, L_min1, c1, c2, gamma_lut,
                              lut, lutDim, ccm_config_1, ccm_config_2, ggain);

                  flag = 1;

               } else {
                  ISPpipeline(img_inp, img_out, img_out_ir, height, width, wr_hls_tmp, params_decompand, R_IR_C1_wgts,
                              R_IR_C2_wgts, B_at_R_wgts, IR_at_R_wgts, IR_at_B_wgts, sub_wgts, hist1_aec, hist0_aec, pawb, bayerp,
                              params_degamma, rgain, bgain, hist1_awb, hist0_awb, igain_1, igain_0, omin[1], omax[1], omin[0],
                              omax[0], blk_height, blk_width, mean1, mean2, L_max1, L_max2, L_min1, L_min2, c1, c2, gamma_lut,
                              lut, lutDim, ccm_config_1, ccm_config_2, ggain);

                  flag = 0;
               }
            }

Create and Launch Kernel in the Testbench:


-  The histogram needs two frames to populate the histogram array and to get correct results in the
   auto exposure frame. Auto white balance, GTM and other tone-mapping functions need
   one extra frame in each to populate its parameters and apply those parameters to
   get a correct image. For the specific example below, four iterations
   are needed because the AEC, AWB, and LTM modules are selected.


.. code:: c

        // Create a kernel:
        OCL_CHECK(err, cl::Kernel kernel(program, "ISPPipeline_accel", &err));

        int loop_count = 4;
      for (int i = 0; i < loop_count; i++) {
         OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inVec,      // buffer on the FPGA
                                             CL_TRUE,           // blocking call
                                             0,                 // buffer offset in bytes
                                             vec_in_size_bytes, // Size in bytes
                                             gamma_lut));

         OCL_CHECK(err, q.enqueueWriteBuffer(buffer_R_IR_C1,        // buffer on the FPGA
                                             CL_TRUE,               // blocking call
                                             0,                     // buffer offset in bytes
                                             filter1_in_size_bytes, // Size in bytes
                                             R_IR_C1_wgts));

         OCL_CHECK(err, q.enqueueWriteBuffer(buffer_R_IR_C2,        // buffer on the FPGA
                                             CL_TRUE,               // blocking call
                                             0,                     // buffer offset in bytes
                                             filter1_in_size_bytes, // Size in bytes
                                             R_IR_C2_wgts));

         OCL_CHECK(err, q.enqueueWriteBuffer(buffer_B_at_R,         // buffer on the FPGA
                                             CL_TRUE,               // blocking call
                                             0,                     // buffer offset in bytes
                                             filter1_in_size_bytes, // Size in bytes
                                             B_at_R_wgts));

         OCL_CHECK(err, q.enqueueWriteBuffer(buffer_IR_at_R,        // buffer on the FPGA
                                             CL_TRUE,               // blocking call
                                             0,                     // buffer offset in bytes
                                             filter2_in_size_bytes, // Size in bytes
                                             IR_at_R_wgts));

         OCL_CHECK(err, q.enqueueWriteBuffer(buffer_IR_at_B,        // buffer on the FPGA
                                             CL_TRUE,               // blocking call
                                             0,                     // buffer offset in bytes
                                             filter2_in_size_bytes, // Size in bytes
                                             IR_at_B_wgts));

         OCL_CHECK(err, q.enqueueWriteBuffer(buffer_sub_wgts,        // buffer on the FPGA
                                             CL_TRUE,                // blocking call
                                             0,                      // buffer offset in bytes
                                             sub_wgts_in_size_bytes, // Size in bytes
                                             sub_wgts));

         OCL_CHECK(err, q.enqueueWriteBuffer(buffer_decompand_params,        // buffer on the FPGA
                                             CL_TRUE,                        // blocking call
                                             0,                              // buffer offset in bytes
                                             decompand_params_in_size_bytes, // Size in bytes
                                             params_decomand));

         OCL_CHECK(err, q.enqueueWriteBuffer(buffer_degamma_params,        // buffer on the FPGA
                                             CL_TRUE,                      // blocking call
                                             0,                            // buffer offset in bytes
                                             degamma_params_in_size_bytes, // Size in bytes
                                             params_degamma));

         OCL_CHECK(err, q.enqueueWriteBuffer(buffer_ccm_matrix_int,     // buffer on the FPGA
                                             CL_TRUE,                   // blocking call
                                             0,                         // buffer offset in bytes
                                             ccm_matrix_int_size_bytes, // Size in bytes
                                             ccm_matrix_int));

         OCL_CHECK(err, q.enqueueWriteBuffer(buffer_offsetarray_int,     // buffer on the FPGA
                                             CL_TRUE,                    // blocking call
                                             0,                          // buffer offset in bytes
                                             offsetarray_int_size_bytes, // Size in bytes
                                             offsetarray_int));

         if (USE_HDR_FUSION) {
               OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inVec_Weights,  // buffer on the FPGA
                                                   CL_TRUE,               // blocking call
                                                   0,                     // buffer offset in bytes
                                                   vec_weight_size_bytes, // Size in bytes
                                                   wr_hls));

               OCL_CHECK(err, q.enqueueWriteBuffer(imageToDevice, CL_TRUE, 0, image_in_size_bytes, interleaved_img.data));

         }

         else {
               OCL_CHECK(err, q.enqueueWriteBuffer(imageToDevice, CL_TRUE, 0, image_in_size_bytes, out_img_12bit.data));
         }

         OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inLut,      // buffer on the FPGA
                                             CL_TRUE,           // blocking call
                                             0,                 // buffer offset in bytes
                                             lut_in_size_bytes, // Size in bytes
                                             casted_lut,        // Pointer to the data to copy
                                             nullptr));
        // Profiling Objects
        cl_ulong start = 0;
        cl_ulong end = 0;
        double diff_prof = 0.0f;
        cl::Event event_sp;

        // Launch the kernel
        OCL_CHECK(err, err = q.enqueueTask(kernel, NULL, &event_sp));

        clWaitForEvents(1, (const cl_event*)&event_sp);

        event_sp.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);

        event_sp.getProfilingInfo(CL_PROFILING_COMMAND_END, &end);

        diff_prof = end - start;
        std::cout << (diff_prof / 1000000) << std::endl;
        exec_sum = exec_sum + diff_prof;

        // Copying Device result data to Host memory
        q.enqueueReadBuffer(imageFromDevice, CL_TRUE, 0, image_out_size_bytes, out_img.data);
       
        if (USE_RGBIR) {
            q.enqueueReadBuffer(imageFromDevice_ir, CL_TRUE, 0, image_out_ir_size_bytes, out_img_ir.data);
        }
    }




.. rubric:: Resource Utilization

The following table summarizes the resource utilization of ISP all_in_one generated using Vitis 
HLS 2023.1 tool on ZCU102 board.

.. table:: Table: ISP all_in_one Resource Utilization Summary


    +----------------+---------------------------+-------------------------------------------------+
    | Operating Mode | Operating Frequency (MHz) |            Utilization Estimate                 |
    +                +                           +------------+-----------+-----------+------------+
    |                |                           |    BRAM    |    DSP    | CLB       |    CLB     |      
    |                |                           |            |           | Registers |    LUT     | 
    +================+===========================+============+===========+===========+============+
    | 1 Pixel        |            150            |    111     |    302    | 42504     |    44000   |     
    +----------------+---------------------------+------------+-----------+-----------+------------+

.. rubric:: Performance Estimate    

The following table summarizes the performance of the ISP all_in_one in 1-pixel
mode as generated using Vitis HLS 2023.1 tool on ZCU102 board.
 
Estimated average latency is obtained by running the accel with four iterations. 
The input to the accel is a 12-bit non-linearized full-HD (1920x1080) image.

.. table:: Table: ISP all_in_one Performance Estimate Summary

    +-----------------------------+-------------------------+
    |                             | Latency Estimate        |
    +      Operating Mode         +-------------------------+
    |                             | Average latency(ms)     |             
    +=============================+=========================+
    | 1 pixel operation (150 MHz) |        22.357           | 
    +-----------------------------+-------------------------+
          
