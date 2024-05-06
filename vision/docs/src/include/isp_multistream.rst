.. vim: syntax=rst

.. 
   Copyright 2023 Advanced Micro Devices, Inc
  
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

ISP Multistream Pipeline
##########################

The ISP multistream pipeline allows you to process input from multi streams using one instance of ISP.
Current multi stream pipeline processes four streams in a Round-Robin method with the input TYPE as XF_16UC1 
and the output TYPE as XF_8UC3(RGB). After the color conversion to the YUV color space, the output TYPE is 
XF_16UC1(YUYV).

This ISP pipeline includes nine blocks, as follows:

-  **Extract Exposure Frames:** The Extract Exposure Frames module returns
   the Short Exposure Frame and Long Exposure Frame from the input frame
   using the Digital overlap parameter.

-  **HDR Merge:** HDR Merge module generates the High Dynamic Range
   image from a set of different exposure frames. Usually, image sensors
   have limited dynamic range and it’s difficult to get HDR image with
   single image capture. From the sensor, the frames are collected with
   different exposure times and will get different exposure frames.
   HDR Merge will generate the HDR frame with those exposure frames.

-  **Black Level Correction:** Black level leads to the whitening of
   image in dark regions and perceived loss of overall contrast. The
   Black level correction algorithm corrects the black and white levels of
   the overall image.

-  **Gain Control**: The Gain control module improves the overall
   brightness of the image.

-  **Demosaicing:** The Demosaic module reconstructs RGB pixels from the
   input Bayer image (RGGB, BGGR, RGBG, GRGB).

-  **Auto White Balance:** The AWB module improves color balance of the
   image by using image statistics.

-  **Color Correction Matrix**: The Color Correction Matrix algorithm
   converts the input image color format to output image color format
   using the Color Correction Matrix provided by the user (CCM_TYPE).

-  **Local Tone Mapping:** Local Tone Mapping takes pixel neighbor statistics 
   into account and produces images with more contrast and brightness.

-  **Gamma Correction:** Gamma Correction improves the overall
   brightness of image.

-  **Color Space Conversion**: Converting RGB image to YUV422(YUYV)
   image for HDMI display purpose. RGB2YUYV converts the RGB image into
   Y channel for every pixel and U and V for alternate pixels.

.. rubric:: ISP multistream Diagram
.. image:: ./images/ISP_multistream.PNG
   :class: image 
   :width: 1000 

.. rubric:: Parameter Descriptions    
  
.. table:: Table: Runtime Parameters

    +------------------+-----------------------------------+
    | **Parameter**    | **Description**                   |
    +==================+===================================+
    | wr_hls           | Lookup table for weight values.   |
    |                  | Computing the weights LUT in host |
    |                  | side and passing as input to the  |
    |                  | function.                         |
    +------------------+-----------------------------------+
    | array_params     | Parameters added in one array for |
    |                  | multistream pipeline.             |
    +------------------+-----------------------------------+
    | gamma_lut        | Lookup table for gamma values.    |
    |                  | First 256 will be R, next 256     |
    |                  | values are G and last 256 values  |
    |                  | are B.                            |
    +------------------+-----------------------------------+
    
.. table:: Table: Compile Time Parameters

    +------------------+-----------------------------------+
    | **Parameter**    | **Description**                   |
    +==================+===================================+
    | XF_HEIGHT        | Maximum height of input and       |
    |                  | output image.                     |
    +------------------+-----------------------------------+
    | XF_WIDTH         | Maximum width of input and output |
    |                  | image.                            |
    +------------------+-----------------------------------+
    | XF_SRC_T         | Input pixel type. Supported pixel |
    |                  | width is 16.                      |
    +------------------+-----------------------------------+
    | NUM_STREAMS      | Total number of streams.          |
    +------------------+-----------------------------------+
    | STRM1_ROWS       | Maximum number of rows to be      |
    |                  | processed for stream 1 in one     |
    |                  | burst.                            |
    +------------------+-----------------------------------+
    | STRM2_ROWS       | Maximum number of rows to be      |
    |                  | processed for stream 2 in one     |
    |                  | burst.                            |
    +------------------+-----------------------------------+
    | STRM3_ROWS       | Maximum number of rows to be      |
    |                  | processed for stream 3 in one     |
    |                  | burst.                            |
    +------------------+-----------------------------------+
    | STRM4_ROWS       | Maximum number of rows to be      |
    |                  | processed for stream 4 in one     |
    |                  | burst.                            |
    +------------------+-----------------------------------+
    | BLOCK_WIDTH      | Maximum block width the image is  |
    |                  | divided into. This can be any     |
    |                  | positive integer greater than or  |
    |                  | equal to 32 and less than input   |
    |                  | image width.                      |
    +------------------+-----------------------------------+
    | BLOCK_HEIGHT     | Maximum block height the image is |
    |                  | divided into. This can be any     |
    |                  | positive integer greater than or  |
    |                  | equal to 32 and less than input   |
    |                  | image height.                     |
    +------------------+----------------+------------------+
    | XF_NPPC          | Number of pixels processed per    |
    |                  | cycle. Only XF_NPPC1 and XF_NPPC2 |
    |                  | are supported.                    |
    +------------------+-----------------------------------+
    | NO_EXPS          | Number of exposure frames to be   |
    |                  | merged in the module.             |
    +------------------+-----------------------------------+
    | W_B_SIZE         | W_B_SIZE is used to define the    |
    |                  | array size for storing the weight |
    |                  | values for wr_hls.                |
    |                  | W_B_SIZE should be 2^bit depth.   |
    +------------------+-----------------------------------+

.. table:: Table: Descriptions of array_params 

    +------------------+-----------------------------------+
    | **Parameter**    | **Description**                   |
    +==================+===================================+
    | rgain            | To configure gain value for the   |
    |                  | red channel.                      |
    +------------------+-----------------------------------+
    | bgain            | To configure gain value for the   |
    |                  | blue channel.                     |
    +------------------+-----------------------------------+
    | ggain            | To configure gain value for the   |
    |                  | green channel.                    |
    +------------------+-----------------------------------+
    | pawb             | %top and %bottom pixels are       |
    |                  | ignored while computing min and   |
    |                  | max to improve quality.           |
    +------------------+-----------------------------------+
    | bayer_p          | The Bayer format of the RAW input |
    |                  | image.                            |
    +------------------+-----------------------------------+
    | black_level      | Black level value to adjust       |
    |                  | overall brightness of the image.  |
    +------------------+-----------------------------------+
    | height           | The number of rows in the image   |
    |                  | or height of the image.           |
    +------------------+-----------------------------------+
    | width            | The number of columns in the      |
    |                  | image or width of the image.      |
    +------------------+-----------------------------------+
    | blk_height       | Actual block height.              |
    +------------------+-----------------------------------+
    | blk_width        | Actual block width.               |
    +------------------+-----------------------------------+
      	
   
The following example demonstrates the top-level ISP pipeline:

.. code:: c

        void ISPPipeline_accel(ap_uint<INPUT_PTR_WIDTH>* img_inp1,
                               ap_uint<INPUT_PTR_WIDTH>* img_inp2,
                               ap_uint<INPUT_PTR_WIDTH>* img_inp3,
                               ap_uint<INPUT_PTR_WIDTH>* img_inp4,
                               ap_uint<OUTPUT_PTR_WIDTH>* img_out1,
                               ap_uint<OUTPUT_PTR_WIDTH>* img_out2,
                               ap_uint<OUTPUT_PTR_WIDTH>* img_out3,
                               ap_uint<OUTPUT_PTR_WIDTH>* img_out4,
                               unsigned short array_params[NUM_STREAMS][10],
                               unsigned char gamma_lut[NUM_STREAMS][256 * 3],
                               short wr_hls[NUM_STREAMS][NO_EXPS * XF_NPPC * W_B_SIZE]){ 
       
        // clang-format off
        #pragma HLS INTERFACE m_axi     port=img_inp1      offset=slave bundle=gmem1
        #pragma HLS INTERFACE m_axi     port=img_inp2      offset=slave bundle=gmem2
        #pragma HLS INTERFACE m_axi     port=img_inp3      offset=slave bundle=gmem3
        #pragma HLS INTERFACE m_axi     port=img_inp4      offset=slave bundle=gmem4
        #pragma HLS INTERFACE m_axi     port=img_out1      offset=slave bundle=gmem5
        #pragma HLS INTERFACE m_axi     port=img_out2      offset=slave bundle=gmem6
        #pragma HLS INTERFACE m_axi     port=img_out3      offset=slave bundle=gmem7
        #pragma HLS INTERFACE m_axi     port=img_out4      offset=slave bundle=gmem8
        #pragma HLS INTERFACE m_axi     port=array_params  offset=slave bundle=gmem9
        #pragma HLS INTERFACE m_axi     port=gamma_lut     offset=slave bundle=gmem10
        #pragma HLS INTERFACE m_axi     port=wr_hls        offset=slave bundle=gmem11    

           // clang-format on

           struct ispparams_config params[NUM_STREAMS];

           uint32_t tot_rows = 0;
           int rem_rows[NUM_STREAMS];

           static short wr_hls_tmp[NUM_STREAMS][NO_EXPS * XF_NPPC * W_B_SIZE];
           static unsigned char gamma_lut_tmp[NUM_STREAMS][256 * 3];

           unsigned short height_arr[NUM_STREAMS], width_arr[NUM_STREAMS];

        ARRAY_PARAMS_LOOP:
           for (int i = 0; i < NUM_STREAMS; i++) {
        // clang-format off
        #pragma HLS LOOP_TRIPCOUNT min=1 max=NUM_STREAMS
                // clang-format on
                height_arr[i] = array_params[i][6];
                width_arr[i] = array_params[i][7];
                height_arr[i] = height_arr[i] * 2;
                tot_rows = tot_rows + height_arr[i];
                rem_rows[i] = height_arr[i];
           }
              
           int glut_TC = 256 * 3;

        GAMMA_LUT_LOOP:
           for (int n = 0; n < NUM_STREAMS; n++) {
        // clang-format off
        #pragma HLS LOOP_TRIPCOUNT min=NUM_STREAMS max=NUM_STREAMS
                // clang-format on        
                for(int i=0; i < glut_TC; i++){
        // clang-format off
        #pragma HLS LOOP_TRIPCOUNT min=glut_TC max=glut_TC
                   // clang-format on           
           
                   gamma_lut_tmp[n][i] = gamma_lut[n][i];
       
                }
           }     
           
        WR_HLS_INIT_LOOP:  
           for(int n =0; n < NUM_STREAMS; n++) {
        // clang-format off
        #pragma HLS LOOP_TRIPCOUNT min=NUM_STREAMS max=NUM_STREAMS
              // clang-format on
              for (int k = 0; k < XF_NPPC; k++) {
        // clang-format off
        #pragma HLS LOOP_TRIPCOUNT min=XF_NPPC max=XF_NPPC
                // clang-format on
                for (int i = 0; i < NO_EXPS; i++) {
        // clang-format off
        #pragma HLS LOOP_TRIPCOUNT min=NO_EXPS max=NO_EXPS
                   // clang-format on
                   for (int j = 0; j < (W_B_SIZE); j++) {
        // clang-format off
        #pragma HLS LOOP_TRIPCOUNT min=W_B_SIZE max=W_B_SIZE
                      // clang-format on
                      wr_hls_tmp[n][(i + k * NO_EXPS) * W_B_SIZE + j] = wr_hls[n][(i + k * NO_EXPS) * W_B_SIZE + j];
                   }
                }
              }
           }

           const uint16_t pt[NUM_STREAMS] = {STRM1_ROWS, STRM2_ROWS, STRM3_ROWS, STRM4_ROWS};
           uint16_t max = STRM1_ROWS;
           for (int i = 1; i < NUM_STREAMS; i++) {
                if (pt[i] > max) max = pt[i];
           }

           const uint16_t TC = tot_rows / max;
           uint32_t addrbound, wr_addrbound, num_rows;

           int strm_id = 0, idx = 0;
           bool eof_awb[NUM_STREAMS] = {0};
           bool eof_ltm[NUM_STREAMS] = {0};

           uint32_t rd_offset1 = 0, rd_offset2 = 0, rd_offset3 = 0, rd_offset4 = 0;
           uint32_t wr_offset1 = 0, wr_offset2 = 0, wr_offset3 = 0, wr_offset4 = 0;

        TOTAL_ROWS_LOOP:
           for (int r = 0; r < tot_rows;) {
        // clang-format off
        #pragma HLS LOOP_TRIPCOUNT min=(XF_HEIGHT/STRM_HEIGHT)*NUM_STREAMS max=(XF_HEIGHT/STRM_HEIGHT)*NUM_STREAMS
              // clang-format on

        // Compute no.of rows to process
              if (rem_rows[idx] / 2 > pt[idx]) { // Check number for remaining rows of 1 interleaved image
                 num_rows = pt[idx];
                 eof_awb[idx] = 0; // 1 interleaved image/stream is not done
                 eof_ltm[idx] = 0;
              } else {
                 num_rows = rem_rows[idx] / 2;
                 eof_awb[idx] = 1; // 1 interleaved image/stream done
                 eof_ltm[idx] = 1;
              }

              strm_id = idx;

              if (idx == 0 && num_rows > 0) {
                   Streampipeline(img_inp1 + rd_offset1, img_out1 + wr_offset1, num_rows, width_arr[idx], hist0_awb, hist1_awb,
                           igain_0, igain_1, flag_awb, eof_awb, array_params, gamma_lut_tmp, wr_hls_tmp, omin_r, omax_r,
                           omin_w, omax_w, flag_ltm, eof_ltm, idx);

                   rd_offset1 += (2 * num_rows * ((width_arr[idx] + 8) >> XF_BITSHIFT(XF_NPPC))) / 2;
                   wr_offset1 += (num_rows * (width_arr[idx] >> XF_BITSHIFT(XF_NPPC))) / 2;

              } else if (idx == 1 && num_rows > 0) {
                   Streampipeline(img_inp2 + rd_offset2, img_out2 + wr_offset2, num_rows, width_arr[idx], hist0_awb, hist1_awb,
                           igain_0, igain_1, flag_awb, eof_awb, array_params, gamma_lut_tmp, wr_hls_tmp, omin_r, omax_r,
                           omin_w, omax_w, flag_ltm, eof_ltm, idx);

                   rd_offset2 += (2 * num_rows * ((width_arr[idx] + 8) >> XF_BITSHIFT(XF_NPPC))) / 2;
                   wr_offset2 += (num_rows * (width_arr[idx] >> XF_BITSHIFT(XF_NPPC))) / 2;

              } else if (idx == 2 && num_rows > 0) {
                   Streampipeline(img_inp3 + rd_offset3, img_out3 + wr_offset3, num_rows, width_arr[idx], hist0_awb, hist1_awb,
                           igain_0, igain_1, flag_awb, eof_awb, array_params, gamma_lut_tmp, wr_hls_tmp, omin_r, omax_r,
                           omin_w, omax_w, flag_ltm, eof_ltm, idx);

                   rd_offset3 += (2 * num_rows * ((width_arr[idx] + 8) >> XF_BITSHIFT(XF_NPPC))) / 2;
                   wr_offset3 += (num_rows * (width_arr[idx] >> XF_BITSHIFT(XF_NPPC))) / 2;
              } else if (idx == 3 && num_rows > 0) {
                   Streampipeline(img_inp4 + rd_offset4, img_out4 + wr_offset4, num_rows, width_arr[idx], hist0_awb, hist1_awb,
                           igain_0, igain_1, flag_awb, eof_awb, array_params, gamma_lut_tmp, wr_hls_tmp, omin_r, omax_r,
                           omin_w, omax_w, flag_ltm, eof_ltm, idx);

                   rd_offset4 += (2 * num_rows * ((width_arr[idx] + 8) >> XF_BITSHIFT(XF_NPPC))) / 2;
                   wr_offset4 += (num_rows * (width_arr[idx] >> XF_BITSHIFT(XF_NPPC))) / 2;
              }  
              // Update remaining rows to process
              rem_rows[idx] = rem_rows[idx] - num_rows * 2;

              // Next stream selection
              if (idx == NUM_STREAMS - 1)
                  idx = 0;

              else
                  idx++;

              // Update total rows to process
              r += num_rows * 2;
           }

           return;
        }
        

Create and Launch Kernel in the Testbench:
=============================================

The histogram function needs two frames to populate the histogram array and to get correct
auto white balance results. For the example below, two iterations are also needed because the AWB function is used.


.. code:: c

        // Create a kernel:
        OCL_CHECK(err, cl::Kernel kernel(program, "ISPPipeline_accel", &err));

        for (int i = 0; i < 2; i++) {
          OCL_CHECK(err, q.enqueueWriteBuffer(buffer_array,     // buffer on the FPGA
                                            CL_TRUE,            // blocking call
                                            0,                  // buffer offset in bytes
                                            array_size_bytes,   // Size in bytes
                                            array_params));

          OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inVec,      // buffer on the FPGA
                                            CL_TRUE,             // blocking call
                                            0,                   // buffer offset in bytes
                                            vec_in_size_bytes,   // Size in bytes
                                            gamma_lut));

          OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inVec_Weights,  // buffer on the FPGA
                                            CL_TRUE,                 // blocking call
                                            0,                       // buffer offset in bytes
                                            vec_weight_size_bytes,   // Size in bytes
                                            wr_hls));
          OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inImage1, CL_TRUE, 0, image_in_size_bytes, interleaved_img1.data));
          OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inImage2, CL_TRUE, 0, image_in_size_bytes, interleaved_img2.data));
          OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inImage3, CL_TRUE, 0, image_in_size_bytes, interleaved_img3.data));
          OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inImage4, CL_TRUE, 0, image_in_size_bytes, interleaved_img4.data));

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
          // Copying Device result data to Host memory
          q.enqueueReadBuffer(buffer_outImage1, CL_TRUE, 0, image_out_size_bytes, out_img1.data);
          q.enqueueReadBuffer(buffer_outImage2, CL_TRUE, 0, image_out_size_bytes, out_img2.data);
          q.enqueueReadBuffer(buffer_outImage3, CL_TRUE, 0, image_out_size_bytes, out_img3.data);
          q.enqueueReadBuffer(buffer_outImage4, CL_TRUE, 0, image_out_size_bytes, out_img4.data);
        }

        
.. rubric:: Resource Utilization

The following table summarizes the resource utilization of ISP multistream generated using Vitis 
HLS 2022.2 tool on ZCU102 board.

.. table:: Table . ISP multistream Resource Utilization Summary


    +----------------+---------------------------+-------------------------------------------------+
    | Operating Mode | Operating Frequency (MHz) |            Utilization Estimate                 |
    +                +                           +------------+-----------+-----------+------------+
    |                |                           |    BRAM    |    DSP    | CLB       |    CLB     |      
    |                |                           |            |           | Registers |    LUT     | 
    +================+===========================+============+===========+===========+============+
    | 2 Pixel        |            150            |    638     |    310    | 64964     |    64103   |     
    +----------------+---------------------------+------------+-----------+-----------+------------+

.. rubric:: Performance Estimate    

The following table summarizes the performance of the ISP multistream in 2-pixel
mode as generated using Vitis HLS 2022.2 tool on ZCU102 board.
 
Estimated average latency is obtained by running the accel with 200 iterations. 
The input to the accel is an interleaved image containing one long-exposure frame 
and one short-exposure frame which are both full-HD (1920x1080) images.

.. table:: Table . ISP multistream Performance Estimate Summary

    +-----------------------------+-------------------------+
    |                             | Latency Estimate        |
    +      Operating Mode         +-------------------------+
    |                             | Average latency(ms)     |             
    +=============================+=========================+
    | 2 pixel operation (150 MHz) |        64.871           | 
    +-----------------------------+-------------------------+
          
    
        


