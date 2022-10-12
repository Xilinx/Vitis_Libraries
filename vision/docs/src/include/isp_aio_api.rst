ISP All-in-one pipeline:
========================

The ISP All-in-one pipeline contains all of the necessary functions that
will enable the user to test several combinations of the image sensor
processing pipeline.

For example, we can test the Surround view system (SVS), INCABIN and
Forward/Rear view pipeline using this ISP All-in-one pipeline. This ISP
All-in-one pipeline takes an interleaved image which contains Short
exposure frame (SEF) and Long exposure frame (LEF) as input when HDR
modules are enabled enabled for SVS pipeline and returns the HDR merged
output.

-  **Extract Exposure Frames:** The extractExposureFrames module returns
   the Shortexposureframe and Longexposureframe from the input frame
   using the Digital overlap parameter.

-  **HDR Merge:** HDR Merge module generates the High dynamic range
   image from a set of different exposure frames. Usually, image sensors
   have limited dynamic range and it’s difficult to get HDR image with
   single image capture. From the sensor, the frames are collected with
   different exposure times and will get different exposure frames,
   HDRMerge will generate the HDR frame with those exposure frames.

-  **BPC (Bad pixel correction)**: An image sensor may have a certain
   number of defective/bad pixels that may be the result of
   manufacturing faults or variations in pixel voltage levels based on
   temperature or exposure. Bad pixel correction module removes
   defective pixels

-  **Black level correction:** Black level leads to the whitening of
   image in dark regions and perceived loss of overall contrast. The
   Blacklevelcorrection algorithm corrects the black and white levels of
   the overall image

-  **RGBIR to Bayer:** This module converts the input image with R, G,
   B, IR pixel data into a standard Bayer pattern image along with a
   full IR data image.

-  **Gain Control**: The Gain control module improves the overall
   brightness of the image.

-  **Demosaicing:** The demosaic module reconstructs RGB pixels from the
   input Bayer image (RGGB, BGGR, RGBG, GRGB).

-  **Auto white balance:** The AWB module improves color balance of the
   image by using image statistics.

-  **Quantization & Dithering:** This algorithm dithers input image
   using Floyd-Steinberg dithering method. It is commonly used by image
   manipulation software, for example when an image is converted into
   GIF format each pixel intensity value is quantized to 8 bit i.e. 256
   colors.

-  **Global tone mapping:** Reduces the dynamic range from higher range
   to display range using tone mapping.

-  **Local tone mapping:** Local tone mapping takes pixel neighbour
   statistics into account, and produces images with more contrast and
   brightness.

-  **Gamma correction:** Gamma correction improves the overall
   brightness of image.

-  **Color Correction Matrix**: Color correction matrix algorithm
   converts the input image color format to output image color format
   using the color correction matrix provided by the user (CCM_TYPE).

-  **3DLUT:** Operate on three independent parameters. This drastically
   increases the number of mapped indexes to value pairs. For example, a
   combination of 3 individual 1D LUTs can map 2^n \* 3 values where n
   is the bit depth, whereas a 3D LUT processing 3 channels will have
   2^n \* 2^n \* 2^n possible values.

-  **Color Space Conversion**: Converting RGB image to YUV422(YUYV)
   image for HDMI display purpose. RGB2YUYV converts the RGB image into
   Y channel for every pixel and U and V for alternate pixels.
   
.. image:: ./images/ISP_All_in_one_Pipeline.png
   :class: image 
   :width: 1000 
      
.. table:: Runtime parameter for the pipeline

    +-----------------------------------+-----------------------------------+
    | **Parameter**                     | **Descriptions**                  |
    +===================================+===================================+
    | height                            | The number of rows in the image   |
    |                                   | or height of the image.           |
    +-----------------------------------+-----------------------------------+
    | width                             | The number of columns in the      |
    |                                   | image or width of the image.      |
    +-----------------------------------+-----------------------------------+
    | wr_hls                            | Lookup table for weight values.   |
    |                                   | Computing the weights LUT in host |
    |                                   | side and passing as input to the  |
    |                                   | function. Weight values are Q1.15 |
    +-----------------------------------+-----------------------------------+
    | rgain                             | To configure gain value for the   |
    |                                   | red channel.                      |
    +-----------------------------------+-----------------------------------+
    | bgain                             | To configure gain value for the   |
    |                                   | blue channel.                     |
    +-----------------------------------+-----------------------------------+
    | R_IR_C1_wgts                      | 5x5 Weights to calculate R at IR  |
    |                                   | location for constellation1       |
    +-----------------------------------+-----------------------------------+
    | R_IR_C2_wgts                      | 5x5 Weights to calculate R at IR  |
    |                                   | location for constellation2       |
    +-----------------------------------+-----------------------------------+
    | B_at_R_wgts                       | 5x5 Weights to calculate B at R   |
    |                                   | location                          |
    +-----------------------------------+-----------------------------------+
    | IR_at_R_wgts                      | 3x3 Weights to calculate IR at R  |
    |                                   | location                          |
    +-----------------------------------+-----------------------------------+
    | IR_at_B_wgts                      | 3x3 Weights to calculate IR at B  |
    |                                   | location                          |
    +-----------------------------------+-----------------------------------+
    | sub_wgts                          | Weights to perform weighted       |
    |                                   | subtraction of IR image from RGB  |
    |                                   | image. sub_wgts[0] -> G Pixel,    |
    |                                   | sub_wgts[1] -> R Pixel,           |
    |                                   | sub_wgts[2] -> B Pixel            |
    |                                   | sub_wgts[3] -> calculated B Pixel |
    +-----------------------------------+-----------------------------------+
    | pawb                              | %top and %bottom pixels are       |
    |                                   | ignored while computing min and   |
    |                                   | max to improve quality.           |
    +-----------------------------------+-----------------------------------+
    | blk_height                        | Actual block height               |
    +-----------------------------------+-----------------------------------+
    | blk_width                         | Actual block width                |
    +-----------------------------------+-----------------------------------+
    | c1                                | To retain the details in bright   |
    |                                   | area using, c1 in the tone        |
    |                                   | mapping.                          |
    +-----------------------------------+-----------------------------------+
    | c2                                | Efficiency factor, ranges from    |
    |                                   | 0.5 to 1 based on output device   |
    |                                   | dynamic range.                    |
    +-----------------------------------+-----------------------------------+
    | gamma_lut                         | Lookup table for gamma            |
    |                                   | values.first 256 will be R, next  |
    |                                   | 256 values are G gamma and last   |
    |                                   | 256 values are B values           |
    +-----------------------------------+-----------------------------------+
    | mode_reg                          | Flag to enable/disable optional   |
    |                                   | module                            |
    +-----------------------------------+-----------------------------------+
    | lutDim                            | Dimension of input lut            |
    +-----------------------------------+-----------------------------------+

.. table:: Description of mode_reg

    +-----------------------------------+-----------------------------------+
    | **Bit position**                  | **Descriptions**                  |
    +===================================+===================================+
    | mode_reg[0:0]                     | This bit of mode_reg dedicated    |
    |                                   | to enable/disable AWB module.     |
    +-----------------------------------+-----------------------------------+
    | mode_reg[1:1]                     | This of mode_reg dedicated to     |
    |                                   | enable/disable HDR module.        |
    +-----------------------------------+-----------------------------------+
    | mode_reg[2:2]                     | Don't care.                       |
    +-----------------------------------+-----------------------------------+
    | mode_reg[3:3]                     | This bit of mode_reg dedicated    |
    |                                   | to enable/disable RGBIR module.   |
    +-----------------------------------+-----------------------------------+
    | mode_reg[4:4]                     | This bit of mode_reg dedicated    |
    |                                   | for tone mapper, always           |
    |                                   | set to 0.                         |
    +-----------------------------------+-----------------------------------+
    | mode_reg[5:5]                     | This bit of mode_reg dedicated    |
    |                                   | to enable/disable QnD module      |
    +-----------------------------------+-----------------------------------+
    | mode_reg[6:6]                     | This bit of mode_reg dedicated    |
    |                                   | to enable/disable LTM module      |
    +-----------------------------------+-----------------------------------+
    | mode_reg[7:7]                     | This bit of mode_reg dedicated    |
    |                                   | to enable/disable GTM module      |
    +-----------------------------------+-----------------------------------+
    | mode_reg[8:8]                     | This bit of mode_reg dedicated    |
    |                                   | to enable/disable CCM module      |
    +-----------------------------------+-----------------------------------+
    | mode_reg[9:9]                     | This bit of mode_reg dedicated    |
    |                                   | to enable/disable 3DLUT module    |
    +-----------------------------------+-----------------------------------+
    | mode_reg[10:10]                   | This bit of mode_reg dedicated    |
    |                                   | to enable/disable CSC module      |
    +-----------------------------------+-----------------------------------+
    | mode_reg[15:11]                   | Don't care                        |
    +-----------------------------------+-----------------------------------+
    
.. table:: Compile time parameter

    +-----------------------------------+-----------------------------------+
    | **Parameter**                     | **Description**                   |
    +===================================+===================================+
    | XF_HEIGHT                         | Maximum height of input and       |
    |                                   | output image                      |
    +-----------------------------------+-----------------------------------+
    | XF_WIDTH                          | Maximum width of input and output |
    |                                   | image (Must be multiple of NPC)   |
    +-----------------------------------+-----------------------------------+
    | XF_BAYER_PATTERN                  | The Bayer format of the RAW input |
    |                                   | image. Supported formats are      |
    |                                   | BGGR, GRBG.                       |
    +-----------------------------------+-----------------------------------+
    | XF_SRC_T                          | Input pixel type;supported pixel  |
    |                                   | widths are 8,10,12,16             |
    +-----------------------------------+-----------------------------------+
    | SQLUTDIM                          | Squared value of maximum          |
    |                                   | dimension of input LUT            |
    +-----------------------------------+-----------------------------------+
    | LUTDIM                            | 33x33 dimension of input LUT      |
    +-----------------------------------+-----------------------------------+



The following example demonstrates the top-level ISP pipeline:

.. code:: c

            void ISPPipeline_accel(ap_uint<INPUT_PTR_WIDTH>* img_inp,                 /* Array2xfMat */
                                   ap_uint<OUTPUT_PTR_WIDTH>* img_out,                /* xfMat2Array */
                                   ap_uint<OUTPUT_PTR_WIDTH>* img_out_ir,             /* xfMat2Array */
                                   int height,                                        /* HDR, rgbir2bayer, fifo_copy */
                                   int width,                                         /* HDR, rgbir2bayer, fifo_copy */
                                   short* wr_hls,                                     /* HDR */
                                   uint16_t rgain,                                    /* gaincontrol */
                                   uint16_t bgain,                                    /* gaincontrol */
                                   char *R_IR_C1_wgts,                                /* rgbir2bayer */
                                   char *R_IR_C2_wgts,                                /* rgbir2bayer */
                                   char *B_at_R_wgts,                                 /* rgbir2bayer */
                                   char *IR_at_R_wgts,                                /* rgbir2bayer */
                                   char *IR_at_B_wgts,                                /* rgbir2bayer */
                                   char *sub_wgts,                                    /* rgbir2bayer */
                                   uint16_t pawb,                                     /* awb */
                                   int blk_height,                                    /* LTM */
                                   int blk_width,                                     /* LTM */
                                   float c1,                                          /* gtm */
                                   float c2,                                          /* gtm */
                                   unsigned char gamma_lut[256 * 3],                  /* gammacorrection */
                                   unsigned short mode_reg,
                                   ap_uint<INPUT_PTR_WIDTH>* lut,                     /* lut3d */
                                   int lutDim                                         /* lut3d */ ){
            // clang-format off
            #pragma HLS INTERFACE m_axi port=img_inp          offset=slave bundle=gmem1
            #pragma HLS INTERFACE m_axi port=img_out          offset=slave bundle=gmem2
            #pragma HLS INTERFACE m_axi port=img_out_ir       offset=slave bundle=gmem3
            #pragma HLS INTERFACE m_axi port=R_IR_C1_wgts     offset=slave bundle=gmem4
            #pragma HLS INTERFACE m_axi port=R_IR_C2_wgts     offset=slave bundle=gmem4
            #pragma HLS INTERFACE m_axi port=B_at_R_wgts      offset=slave bundle=gmem4
            #pragma HLS INTERFACE m_axi port=IR_at_R_wgts     offset=slave bundle=gmem4
            #pragma HLS INTERFACE m_axi port=IR_at_B_wgts     offset=slave bundle=gmem4
            #pragma HLS INTERFACE m_axi port=sub_wgts         offset=slave bundle=gmem5
            #pragma HLS INTERFACE m_axi port=gamma_lut        offset=slave bundle=gmem6
            #pragma HLS INTERFACE m_axi port=wr_hls           offset=slave bundle=gmem7

            #pragma HLS ARRAY_PARTITION variable=IR_at_B_wgts complete dim=1
            #pragma HLS ARRAY_PARTITION variable=bgain        complete dim=1
            #pragma HLS ARRAY_PARTITION variable=rgain        complete dim=1
            #pragma HLS ARRAY_PARTITION variable=R_IR_C2_wgts complete dim=1
            #pragma HLS ARRAY_PARTITION variable=R_IR_C1_wgts complete dim=1
            #pragma HLS ARRAY_PARTITION variable=sub_wgts     complete dim=1
            #pragma HLS ARRAY_PARTITION variable=IR_at_R_wgts complete dim=1
            #pragma HLS ARRAY_PARTITION variable=mode_reg     complete dim=1
            #pragma HLS ARRAY_PARTITION variable=pawb         complete dim=1
            #pragma HLS ARRAY_PARTITION variable=hist0_awb    complete dim=1
            #pragma HLS ARRAY_PARTITION variable=hist1_awb    complete dim=1

            #pragma HLS ARRAY_PARTITION variable=omin dim=1   complete
            #pragma HLS ARRAY_PARTITION variable=omin dim=2   cyclic factor=2
            #pragma HLS ARRAY_PARTITION variable=omin dim=3   cyclic factor=2
            #pragma HLS ARRAY_PARTITION variable=omax dim=1   complete
            #pragma HLS ARRAY_PARTITION variable=omax dim=2   cyclic factor=2
            #pragma HLS ARRAY_PARTITION variable=omax dim=3   cyclic factor=2
            // clang-format on
            if (!flag) {
                ISPpipeline(img_inp, img_out, img_out_ir, mode_reg, height, width, wr_hls, R_IR_C1_wgts, R_IR_C2_wgts,
                            B_at_R_wgts, IR_at_R_wgts, IR_at_B_wgts, sub_wgts, rgain, bgain,  hist0_awb, hist1_awb,
                            igain_0, igain_1, pawb, gamma_lut, omin[0], omax[0], omin[1], omax[1], blk_height,blk_width,
                            mean2, mean1, L_max2, L_max1, L_min2, L_min1, c1, c2, lut, lutDim);
                 flag = 1;
                } else {
                ISPpipeline(img_inp, img_out, img_out_ir, mode_reg, height, width, wr_hls, R_IR_C1_wgts, R_IR_C2_wgts, 
                            B_at_R_wgts, IR_at_R_wgts, IR_at_B_wgts, sub_wgts, rgain, bgain,  hist1_awb, hist0_awb,
                            igain_1, igain_0, pawb, gamma_lut, omin[1], omax[1], omin[0], omax[0], blk_height, blk_width,
                            mean1, mean2, L_max1, L_max2, L_min1, L_min2, c1, c2, lut, lutDim);
                flag = 0;
                }
            }
    


Accel call in the testbench:

Histogram needs two frames to populate the histogram and to get correct
auto white balance results. GTM and other tone-mapping functions need
three frames to populate its parameters and apply those parameters to
get a corrected image. For the specific example below, three iterations
are needed because the GTM function is selected.


.. code:: c

            for (int i = 0; i < 3; i++) {
                OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inVec,                     // buffer on the FPGA
                                                    CL_TRUE,                          // blocking call
                                                    0,                                // buffer offset in bytes
                                                    vec_in_size_bytes,                // Size in bytes
                                                    gamma_lut));
                                                    
                OCL_CHECK(err, q.enqueueWriteBuffer(buffer_R_IR_C1,                   // buffer on the FPGA
                                                    CL_TRUE,                          // blocking call
                                                    0,                                // buffer offset in bytes
                                                    filter1_in_size_bytes,            // Size in bytes
                                                    R_IR_C1_wgts));                   
                                                                                      
                OCL_CHECK(err, q.enqueueWriteBuffer(buffer_R_IR_C2,                   // buffer on the FPGA
                                                    CL_TRUE,                          // blocking call
                                                    0,                                // buffer offset in bytes
                                                    filter1_in_size_bytes,            // Size in bytes
                                                    R_IR_C2_wgts));                   
                                                                                      
                OCL_CHECK(err, q.enqueueWriteBuffer(buffer_B_at_R,                    // buffer on the FPGA
                                                    CL_TRUE,                          // blocking call
                                                    0,                                // buffer offset in bytes
                                                    filter1_in_size_bytes,            // Size in bytes
                                                    B_at_R_wgts));                    
                                                                                      
                OCL_CHECK(err, q.enqueueWriteBuffer(buffer_IR_at_R,                   // buffer on the FPGA
                                                    CL_TRUE,                          // blocking call
                                                    0,                                // buffer offset in bytes
                                                    filter2_in_size_bytes,            // Size in bytes
                                                    IR_at_R_wgts));                   
                                                                                      
                OCL_CHECK(err, q.enqueueWriteBuffer(buffer_IR_at_B,                   // buffer on the FPGA
                                                    CL_TRUE,                          // blocking call
                                                    0,                                // buffer offset in bytes
                                                    filter2_in_size_bytes,            // Size in bytes
                                                    IR_at_B_wgts));                   
                                                                                      
                OCL_CHECK(err, q.enqueueWriteBuffer(buffer_sub_wgts,                   // buffer on the FPGA
                                                    CL_TRUE,                           // blocking call
                                                    0,                                 // buffer offset in bytes
                                                    sub_wgts_in_size_bytes,            // Size in bytes
                                                    sub_wgts));
                if(hdr_en){   
                    OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inVec_Weights,         // buffer on the FPGA
                                                        CL_TRUE,                      // blocking call
                                                        0,                            // buffer offset in bytes
                                                        vec_weight_size_bytes,        // Size in bytes
                                                        wr_hls));
          
                    OCL_CHECK(err, q.enqueueWriteBuffer(imageToDevice, 
                                                        CL_TRUE, 
                                                        0, 
                                                        image_in_size_bytes, 
                                                        interleaved_img.data)); 
          
                } else {   
                    OCL_CHECK(err, q.enqueueWriteBuffer(imageToDevice, 
                                                        CL_TRUE, 
                                                        0, 
                                                        image_in_size_bytes, 
                                                        in_img1.data));          
                }
            }

