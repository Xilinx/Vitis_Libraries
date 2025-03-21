.. 
   Copyright 2023 Advanced Micro Devices, Inc
  
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

ISP autogain Pipeline:
=========================

The ISPPipeline_accel function is an image signal processing (ISP) pipeline 
accelerator designed for hardware implementation on an FPGA. It processes 
an input image (img_inp) and generates an output image (img_out) while applying various 
ISP operations.

This ISP autogain pipeline includes 9 modules, as follows:

-  **Black Level Correction (BLC):** This module corrects the black and white 
   levels of the overall image. Black level leads to the whitening of
   image in dark regions and perceived loss of overall contrast. 
   
-  **Auto gain Control:** The autogain function calculates automatic gain adjustment for an image 
   by analyzing its histogram, grouping bins into clusters, and selecting appropriate gain values 
   for the red, green, and blue channels based on the most populated histogram cluster. The computed 
   gain values (rgain, ggain, bgain) help in adjusting the image brightness and contrast dynamically.

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

-  **Gamma Correction:** This module improve the overall brightness of the image. 
   
-  **Color Space Conversion (CSC):** The CSS module converts an RGB image to 
   a YUV422(YUYV) image for HDMI display purposes. RGB2YUYV converts the RGB image 
   into Y channel for every pixel and U and V for alternating pixels.

      
.. table:: Table: Runtime Parameters for the Pipeline

    +-------------------------+-----------------------------------------------------------+
    | **Parameter**           | **Descriptions**                                          |
    +=========================+===========================================================+
    | height                  | The number of rows in the image                           |
    |                         | or height of the image.                                   |
    +-------------------------+-----------------------------------------------------------+
    | width                   | The number of columns in the                              |
    |                         | image or width of the image.                              |
    +-------------------------+-----------------------------------------------------------+
    | gamma_lut               | Lookup table for gamma values.                            |
    |                         | First 256 will be R, next 256                             |
    |                         | values are G and last 256 values                          |
    |                         | are B.                                                    |
    +-------------------------+-----------------------------------------------------------+
    | mode_reg                | Enables awb if "1" else enables                           |
    |                         | fifo.                                                     |
    +-------------------------+-----------------------------------------------------------+
    | pawb                    | %top and %bottom pixels are                               |
    |                         | ignored while computing min and                           |
    |                         | max to improve quality.                                   |
    +-------------------------+-----------------------------------------------------------+
    | ccm_config_1            | ccm_matrix Fixed-point Q(12,20)                           |
    |                         | Computed as (signed int)(ccm_matrix_float * 1048576)      |
    +-------------------------+---------------------------------------------------------- +
    | ccm_config_2            | offsetarray Fixed-point Q(12,20)                         |
    |                         | Computed as (signed int)(offsetarray_float * 1048576)     |
   +--------------------------+-----------------------------------------------------------+
    | bformat                 | Input Bayer pattern. XF_BAYER_BG, XF_BAYER_GB,            |
    |                         | XF_BAYER_GR, and XF_BAYER_RG are the supported            |
    |                         | values.                                                   |
    +-------------------------+-----------------------------------------------------------+
   

.. table:: Table: Compile Time Parameters

    +-------------------------+-----------------------------------+
    | **Parameter**           | **Description**                   |
    +=========================+===================================+
    | XF_HEIGHT               | Maximum height of input and       |
    |                         | output image.                     |
    +-------------------------+-----------------------------------+
    | XF_WIDTH                | Maximum width of input and output |
    |                         | image.                            |
    +-------------------------+-----------------------------------+
    | IN_TYPE                 | Input pixel type. Supported pixel |
    |                         | width is 8,10,12 and 16.          |
    +-------------------------+-----------------------------------+
    | XF_NPPC                 | Number of pixels processed per    |
    |                         | cycle. Supports XF_NPPC1,         |
    |                         | XF_NPPC2, XF_NPPC4, XF_NPPC8      |
    +-------------------------+-----------------------------------+

The following example demonstrates the top-level ISP pipeline:

.. code:: c

           void ISPPipeline_accel(ap_uint<INPUT_PTR_WIDTH>* img_inp,
                       ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                       int height,
                       int width,
                       unsigned char gamma_lut[256 * 3],
                       unsigned char mode_reg,
                       uint16_t pawb,
                       int ccm_config_1[3][3],
                       int ccm_config_2[3],
                       uint16_t bformat) {
            // clang-format off
            #pragma HLS INTERFACE m_axi     port=img_inp  offset=slave bundle=gmem1
            #pragma HLS INTERFACE m_axi     port=img_out  offset=slave bundle=gmem2

            #pragma HLS INTERFACE m_axi port=ccm_config_1     bundle=gmem3 offset=slave
            #pragma HLS INTERFACE m_axi port=ccm_config_2     bundle=gmem4 offset=slave

            // clang-format on

            // clang-format off
            #pragma HLS ARRAY_PARTITION variable=hist0_awb complete dim=1
            #pragma HLS ARRAY_PARTITION variable=hist1_awb complete dim=1

               // clang-format on

               if (!flag) {
                  ISPpipeline(img_inp, img_out, height, width, hist0_awb, hist1_awb, igain_0, igain_1, gain0_agc, gain1_agc,
                              gamma_lut, mode_reg, pawb, ccm_config_1, ccm_config_2, bformat);
                  flag = 1;

               } else {
                  ISPpipeline(img_inp, img_out, height, width, hist1_awb, hist0_awb, igain_1, igain_0, gain1_agc, gain0_agc,
                              gamma_lut, mode_reg, pawb, ccm_config_1, ccm_config_2, bformat);
                  flag = 0;
               }
            }

Create and Launch Kernel in the Testbench:
===========================================

Histogram needs 1 frame to populate the bins and next frame to process the bin values 
to compute the autogain and to get correct results in the auto exposure frame. 
Auto white balance, GTM and other tone-mapping functions need one extra frame in each 
to populate its parameters and apply those parameters to get a correct image. For the 
specific example below, 3  iterations are needed because the AWB module is selected.


.. code:: c

   // Create a kernel:
    OCL_CHECK(err, cl::Kernel kernel(program, "ISPPipeline_accel", &err));

    std::vector<cl::Memory> inBufVec, outBufVec;
    OCL_CHECK(err, cl::Buffer imageToDevice(context, CL_MEM_READ_ONLY, image_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer imageFromDevice(context, CL_MEM_WRITE_ONLY, image_out_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_inVec(context, CL_MEM_READ_ONLY, vec_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_ccm_matrix_int(context, CL_MEM_READ_ONLY, ccm_matrix_int_size_bytes, NULL, &err));
    OCL_CHECK(err,
              cl::Buffer buffer_offsetarray_int(context, CL_MEM_READ_ONLY, offsetarray_int_size_bytes, NULL, &err));
    // Set the kernel arguments
    OCL_CHECK(err, err = kernel.setArg(0, imageToDevice));
    OCL_CHECK(err, err = kernel.setArg(1, imageFromDevice));
    OCL_CHECK(err, err = kernel.setArg(2, height));
    OCL_CHECK(err, err = kernel.setArg(3, width));
    OCL_CHECK(err, err = kernel.setArg(4, buffer_inVec));
    OCL_CHECK(err, err = kernel.setArg(5, mode_reg));
    OCL_CHECK(err, err = kernel.setArg(6, pawb));
    OCL_CHECK(err, err = kernel.setArg(7, buffer_ccm_matrix_int));
    OCL_CHECK(err, err = kernel.setArg(8, buffer_offsetarray_int));
    OCL_CHECK(err, err = kernel.setArg(9, bformat));

    for (int i = 0; i < 3; i++) {
        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inVec,      // buffer on the FPGA
                                            CL_TRUE,           // blocking call
                                            0,                 // buffer offset in bytes
                                            vec_in_size_bytes, // Size in bytes
                                            gamma_lut));
        OCL_CHECK(err, q.enqueueWriteBuffer(imageToDevice, CL_TRUE, 0, image_in_size_bytes, bayer_img.data));

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
        std::cout << (diff_prof / 1000000) << "ms" << std::endl;
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

        // Copying Device result data to Host memory
        q.enqueueReadBuffer(imageFromDevice, CL_TRUE, 0, image_out_size_bytes, out_img.data);
    }




.. rubric:: Resource Utilization

The following table summarizes the resource utilization of ISP autogain generated using Vitis 
HLS 2024.2 tool on ZCU104 board.

.. table:: Table: ISP autogain Resource Utilization Summary


    +----------------+---------------------------+-------------------------------------------------+
    | Operating Mode | Operating Frequency (MHz) |            Utilization Estimate                 |
    +                +                           +------------+-----------+-----------+------------+
    |                |                           |    BRAM    |    DSP    | CLB       |    CLB     |      
    |                |                           |            |           | Registers |    LUT     | 
    +================+===========================+============+===========+===========+============+
    | 2 Pixel        |            300            |    88      |    96     | 35387     |    23140   |     
    +----------------+---------------------------+------------+-----------+-----------+------------+

.. rubric:: Performance Estimate    

The following table summarizes the performance of the ISP autogain in 2-pixel
mode as generated using Vitis HLS 2024.2 tool on ZCU104 board.
 
Estimated average latency is obtained by running the accel with four iterations. 
The input to the accel is a 8-bit 4k (2160x3840) image.

.. table:: Table: ISP autogain Performance Estimate Summary

    +-----------------------------+-------------------------+
    |                             | Latency Estimate        |
    +      Operating Mode         +-------------------------+
    |                             | Average latency(ms)     |             
    +=============================+=========================+
    | 2 pixel operation (300 MHz) |        13.824           | 
    +-----------------------------+-------------------------+
          
