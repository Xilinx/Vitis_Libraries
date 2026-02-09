
.. 
   Copyright 2023 Advanced Micro Devices, Inc
  
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

ISP 24-bit Pipeline
====================

Imaging sensors that do not equip with a high bit-width on the transmission side can be compressed (compand) in a piece-wise linear (PWL) mapping to a lower bit depth.
 Through the HDR De-companding kernel, the data can be recovered to higher bit-widths and processed through further kernels of ISP by converting to 14-bit for efficient use of resources.
	
This ISP 24-bit pipeline includes the following 17 blocks:
	
    *	HDR Decompand
    *   ConvertTo
    *   RGBIR
    *	Auto Exposure Correction(AEC)
    *	Black level correction
    *	Bad pixel correction
    *	Degamma
    *   Lens shading correction
    *   Gain Control 
    *   Demosaicing 
    *   Tone Mapping
    *	Auto white balance
    *	ISP Stats
    *   Colorcorrection matrix
    *	Gamma correction
    *	3D LUT
    *   Color space conversion
      
The current design example demonstrates how to use ISP functions in a pipeline. 

.. image:: ./images/block_diagram_24bit.png
   :class: image 
   :width: 1000 


The following table describes the parameters of the pipeline which can be configured dynamically.
      
.. table:: Table: Runtime Parameter for the Pipeline


    +-----------------------------+--------------------------------------------------+
    | **Parameter**               | **Descriptions**                                 |
    +=============================+==================================================+
    | height                      | The number of rows in the image                  |
    |                             | or height of the image.                          |
    +-----------------------------+--------------------------------------------------+
    | width                       | The number of columns in the                     |
    |                             | image or width of the image.                     |
    +-----------------------------+--------------------------------------------------+
    | rgain                       | To configure gain value for the                  |
    |                             | red channel.                                     |
    +-----------------------------+--------------------------------------------------+
    | bgain                       | To configure gain value for the                  |
    |                             | blue channel.                                    |
    +-----------------------------+--------------------------------------------------+
    | R_IR_C1_wgts                | 5x5 Weights to calculate R at IR                 |
    |                             | location for constellation1                      |
    +-----------------------------+--------------------------------------------------+
    | R_IR_C2_wgts                | 5x5 Weights to calculate R at IR                 |
    |                             | location for constellation2                      |
    +-----------------------------+--------------------------------------------------+
    | B_at_R_wgts                 | 5x5 Weights to calculate B at R                  |
    |                             | location                                         |
    +-----------------------------+--------------------------------------------------+
    | IR_at_R_wgts                | 3x3 Weights to calculate IR at R                 |
    |                             | location                                         |
    +-----------------------------+--------------------------------------------------+
    | IR_at_B_wgts                | 3x3 Weights to calculate IR at B                 |
    |                             | location                                         |
    +-----------------------------+--------------------------------------------------+
    | sub_wgts                    | Weights to perform weighted                      |
    |                             | subtraction of IR image from RGB                 |
    |                             | image. sub_wgts[0] -> G Pixel,                   |
    |                             | sub_wgts[1] -> R Pixel,                          |
    |                             | sub_wgts[2] -> B Pixel                           |
    |                             | sub_wgts[3] -> calculated B Pixel                |
    +-----------------------------+--------------------------------------------------+
    | bayerp                      | Input Bayer pattern. XF_BAYER_BG,                |
    |                             | XF_BAYER_GB, XF_BAYER_GR, XF_BAYER_RG            |
    |                             | are the supported values.                        |
    +-----------------------------+--------------------------------------------------+
    | params                      | consists of slope and intercept                  |
    |                             | values of four knee points in hdr                |
    |                             | decompand kernel for R, G, B.                    |
    +-----------------------------+--------------------------------------------------+
    | params_14bit                | consists of slope and intercept                  |
    |                             | values of four knee points to                    |
    |                             | truncate 24bit to 14bit.                         |
    +-----------------------------+--------------------------------------------------+
    | params_degamma              | consists of slope and intercept                  |
    |                             | values of knee points till 64 to                 |
    |                             | linearize the image.                             |
    +-----------------------------+--------------------------------------------------+
    | paec                        | %top and %bottom pixels are                      |
    |                             | ignored while computing min and                  |
    |                             | max to improve quality in AEC.                   |
    +-----------------------------+--------------------------------------------------+
    | pawb                        | %top and %bottom pixels are                      |
    |                             | ignored while computing min and                  |
    |                             | max to improve quality in AWB.                   |
    +-----------------------------+--------------------------------------------------+
    | aec_stats                   | Calculted histogram of the AEC input image.      |
    +-----------------------------+--------------------------------------------------+
    | awb_stats                   | Calculted histogram of the AWB input image.      |
    +-----------------------------+--------------------------------------------------+
    | aec_max_bins                | List of maximum values per range of bins.        | 
    |                             | This is only applicable if merge bins feature    | 
    |                             | is enabled.                                      |
    +-----------------------------+--------------------------------------------------+
    | awb_max_bins                | List of maximum values per range of bins.        | 
    |                             | This is only applicable if merge bins feature    | 
    |                             | is enabled.                                      |
    +-----------------------------+--------------------------------------------------+
    | roi_tlx                     | Top left x coordinate of ROI                     |
    +-----------------------------+--------------------------------------------------+
    | roi_tly                     | Top left y coordinate of ROI                     |
    +-----------------------------+--------------------------------------------------+
    | roi_brx                     | Bottom right x coordinate of ROI                 |
    +-----------------------------+--------------------------------------------------+
    | roi_bry                     | Bottom right y coordinate of ROI                 |
    +-----------------------------+--------------------------------------------------+
    | zone_col_num                | Number of zones across column.                   |
    +-----------------------------+--------------------------------------------------+
    | zone_row_num                | Number of zones across rows.                     |
    +-----------------------------+--------------------------------------------------+
    | blk_height                  | Actual block height                              |
    +-----------------------------+--------------------------------------------------+
    | blk_width                   | Actual block width                               |
    +-----------------------------+--------------------------------------------------+
    | c1                          | To retain the details in bright                  |
    |                             | area using, c1 in the tone                       |
    |                             | mapping.                                         |
    +-----------------------------+--------------------------------------------------+
    | c2                          | Efficiency factor, ranges from                   |
    |                             | 0.5 to 1 based on output device                  |
    |                             | dynamic range.                                   |
    +-----------------------------+--------------------------------------------------+
    | gamma_lut                   | Lookup table for gamma                           |
    |                             | values.first 256 will be R, next                 |
    |                             | 256 values are G gamma and last                  |
    |                             | 256 values are B values                          |
    +-----------------------------+--------------------------------------------------+
    | lutDim                      | Dimension of input lut                           |
    +-----------------------------+--------------------------------------------------+


The following table describes the template parameters which can be configured.


.. table:: Table: Compile Time Parameters


    +----------------------+------------------------------------------------------+
    | Parameter            | Description                                          |
    +======================+======================================================+
    | XF_HEIGHT            | Maximum height of input and output image             |
    +----------------------+------------------------------------------------------+
    | XF_WIDTH             | Maximum width of input and output image              |
    |                      | (Must be multiple of NPC)                            |
    +----------------------+------------------------------------------------------+
    | XF_INP_T             |Input pixel type,Supported pixel widths are 8,10,12,16|
    +----------------------+------------------------------------------------------+
    | XF_BAYER_PATTERN     | The Bayer format of the RAW input  image.            |
    |                      | Supported formats are BGGR, GRBG and GBRG.           |
    +----------------------+------------------------------------------------------+
    | BLACK_LEVEL          | black level value.                                   |
    +----------------------+------------------------------------------------------+
    | DEGAMMA_KP           | Number of knee points in degamma.                    |
    +----------------------+------------------------------------------------------+
    | MAX_ZONES            | Maximum number of possible zones.                    |
    +----------------------+------------------------------------------------------+
    | STATS_SIZE           | Number of bins per channel for the input image.      |
    |                      | This is equal to the number of output bins if        |
    |                      | merge bins feature is disabled.                      |
    +----------------------+------------------------------------------------------+
    | FINAL_BINS_NUM       |  Number of output bins per channel if merge          | 
    |                      |  bins feature is enabled.                            |  
    +----------------------+------------------------------------------------------+
    | MERGE_BINS           | To disable or enable merge bins feature.             |
    +----------------------+------------------------------------------------------+
    | SQLUTDIM             | Squared value of maximum dimension of input LUT      | 
    |                      |                                                      |
    +----------------------+------------------------------------------------------+
    | LUTDIM               | 33x33 dimension of input LUT                         |
    +----------------------+------------------------------------------------------+
   
The following example demonstrates the ISP pipeline with the above list of functions.

.. code:: c

			void ISPpipeline(ap_uint<INPUT_PTR_WIDTH>* img_inp,
						 ap_uint<OUTPUT_PTR_WIDTH>* img_out,
						 ap_uint<OUTPUT_PTR_WIDTH>* img_out_ir,
						 unsigned short height,
						 unsigned short width,
						 int params[3][4][3],
						 ap_ufixed<48, 24> params_14bit[3][4][3],
						 char R_IR_C1_wgts[25],
						 char R_IR_C2_wgts[25],
						 char B_at_R_wgts[25],
						 char IR_at_R_wgts[9],
						 char IR_at_B_wgts[9],
						 char sub_wgts[4],
						 unsigned short bayerp,
						 uint16_t rgain,
						 uint16_t bgain,
						 ap_ufixed<32, 16> params_degamma[3][DEGAMMA_KP][3],
						 uint32_t aec_hist0[HIST_SIZE_AEC],    /* function_aec */
						 uint32_t aec_hist1[HIST_SIZE_AEC],    /* function_aec */
						 uint32_t awb_hist0[3][HIST_SIZE_AWB], /* function_awb */
						 uint32_t awb_hist1[3][HIST_SIZE_AWB], /* function_awb */
						 int gain0[3],                         /* function_awb */
						 int gain1[3],                         /* function_awb */
						 uint16_t paec,
						 uint16_t pawb,
						 unsigned int* aec_stats,
						 unsigned int* awb_stats,
						 ap_uint<13>* aec_max_bins,
						 ap_uint<13>* awb_max_bins,
						 int roi_tlx,
						 int roi_tly,
						 int roi_brx,
						 int roi_bry,
						 int zone_col_num, // N
						 int zone_row_num, // M
						 unsigned char gamma_lut[256 * 3],
						 XF_CTUNAME(XF_SRC_T, XF_NPPC) omin_r[MinMaxVArrSize][MinMaxHArrSize], /* LTM */
						 XF_CTUNAME(XF_SRC_T, XF_NPPC) omax_r[MinMaxVArrSize][MinMaxHArrSize], /* LTM */
						 XF_CTUNAME(XF_SRC_T, XF_NPPC) omin_w[MinMaxVArrSize][MinMaxHArrSize], /* LTM */
						 XF_CTUNAME(XF_SRC_T, XF_NPPC) omax_w[MinMaxVArrSize][MinMaxHArrSize], /* LTM */
						 int blk_height,                                                       /* LTM */
						 int blk_width,                                                        /* LTM */
						 ap_ufixed<16, 4>& mean1,                                              /* gtm */
						 ap_ufixed<16, 4>& mean2,                                              /* gtm */
						 ap_ufixed<16, 4>& L_max1,                                             /* gtm */
						 ap_ufixed<16, 4>& L_max2,                                             /* gtm */
						 ap_ufixed<16, 4>& L_min1,                                             /* gtm */
						 ap_ufixed<16, 4>& L_min2,                                             /* gtm */
						 float c1,                                                             /* gtm */
						 float c2,                                                             /* gtm */
						 ap_uint<LUT_PTR_WIDTH>* lut,
						 int lutDim) {

					#pragma HLS INLINE OFF

					xf::cv::Mat<XF_INP_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_imgInput> imgInput1(height, width);
					xf::cv::Mat<XF_HDR_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_hdr_out> hdr_out(height, width);
					xf::cv::Mat<XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_hdr_out> img_14bit(height, width);
					xf::cv::Mat<XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_rggb_out> rggb_out(height, width);
					xf::cv::Mat<XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_aecin> aec_in1(height, width);
					xf::cv::Mat<XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_aecin> aec_in2(height, width);
					xf::cv::Mat<XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_aec_out> aec_out(height, width);
					xf::cv::Mat<XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_bpc_out> bpc_out(height, width);
					xf::cv::Mat<XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_blc_out> blc_out(height, width);
					xf::cv::Mat<XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_dgamma_out> dgamma_out(height, width);
					xf::cv::Mat<XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_lsc_out> LscOut(height, width);
					xf::cv::Mat<XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_gain_out> gain_out(height, width);
					xf::cv::Mat<XF_DST_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_demosaic_out> demosaic_out(height, width);
					xf::cv::Mat<XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_ltm_out> ltm_out(height, width);
					xf::cv::Mat<XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_awb_out> awb_out(height, width);
					xf::cv::Mat<XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_awbin> awb_in1(height, width);
					xf::cv::Mat<XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_awbin> awb_in2(height, width);
					xf::cv::Mat<XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_dst> gamma_out(height, width);
					xf::cv::Mat<XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_ccm> ccm_out(height, width);
					xf::cv::Mat<XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_lut_out> lut_out(height, width);

				// clang-format off
				#pragma HLS DATAFLOW
					// clang-format on

					float awb_thresh = (float)pawb / 256;
					float aec_thresh = (float)paec / 256;
					float inputMax = (1 << (XF_DTPIXELDEPTH(XF_SRC_T, XF_NPPC))) - 1; // 65535.0f;

					float mul_fact = (inputMax / (inputMax - BLACK_LEVEL));
                    unsigned int blc_config_1 = (int)(mul_fact * 65536); // mul_fact int Q16_16 format
                    unsigned int blc_config_2 = BLACK_LEVEL; 
					float inputmin = 0.0f;
					float inputmax1 = 255.0f;
					float outputmin = 0.0f;
					float outputmax1 = 255.0f;
					float inputmax2 = 16383.0f;
					float outputmax2 = 16383.0f;
					int outdepth = (1 << XF_DTPIXELDEPTH(XF_GTM_T, XF_NPPC));
					
					xf::cv::Array2xfMat<INPUT_PTR_WIDTH, XF_INP_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_imgInput>(img_inp,
																											   imgInput1);

					xf::cv::hdr_decompand<XF_INP_T, XF_HDR_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_imgInput, XF_CV_DEPTH_hdr_out>(
						imgInput1, hdr_out, params, bayerp);

					xf::cv::convert24To14bit<XF_HDR_T, XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_hdr_out,
											 XF_CV_DEPTH_hdr_out>(hdr_out, img_14bit, params_14bit, bayerp);

					function_rgbir_or_fifo<XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_hdr_out, XF_CV_DEPTH_rggb_out,
										   XF_CV_DEPTH_rggb_out_ir, XF_CV_DEPTH_3XWIDTH>(img_14bit, rggb_out, img_out_ir, R_IR_C1_wgts,
																						 R_IR_C2_wgts, B_at_R_wgts, IR_at_R_wgts,
																						 IR_at_B_wgts, sub_wgts, height, width);

					function_aec<XF_SRC_T, XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_aecin, XF_CV_DEPTH_aec_out>(
						rggb_out, aec_out, height, width, aec_thresh, aec_hist0, aec_hist1);

					xf::cv::blackLevelCorrection<XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, 16, 15, 1, XF_CV_DEPTH_aec_out,
												 XF_CV_DEPTH_blc_out>(aec_out, blc_out, blc_config_2, blc_config_1);
					xf::cv::badpixelcorrection<XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, 0, 0, XF_CV_DEPTH_blc_out, XF_CV_DEPTH_bpc_out>(
						blc_out, bpc_out);

					function_degamma<XF_SRC_T, XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_bpc_out, XF_CV_DEPTH_dgamma_out,
									 DEGAMMA_KP>(bpc_out, dgamma_out, params_degamma, bayerp, height, width);
					xf::cv::Lscdistancebased<XF_SRC_T, XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_dgamma_out,
											 XF_CV_DEPTH_lsc_out>(dgamma_out, LscOut);

					xf::cv::gaincontrol<XF_BAYER_PATTERN, XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_lsc_out,
										XF_CV_DEPTH_gain_out>(LscOut, gain_out, rgain, bgain);

					xf::cv::demosaicing<XF_BAYER_PATTERN, XF_SRC_T, XF_DST_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, 0, XF_CV_DEPTH_gain_out,
										XF_CV_DEPTH_demosaic_out>(gain_out, demosaic_out);
					if (XF_DST_T == XF_8UC3) {
						fifo_copy<XF_DST_T, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_demosaic_out, XF_CV_DEPTH_ltm_out>(
							demosaic_out, ltm_out, height, width);
					} else {
						function_tm<XF_DST_T, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_demosaic_out, XF_CV_DEPTH_ltm_out>(
							demosaic_out, ltm_out, omin_r, omax_r, omin_w, omax_w, blk_height, blk_width, mean1, mean2, L_max1, L_max2,
							L_min1, L_min2, c1, c2, height, width);
					}
					xf::cv::duplicateMat<XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_ltm_out, XF_CV_DEPTH_awbin,
										 XF_CV_DEPTH_awbin>(ltm_out, awb_in1, awb_in2);

					function_awb<XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_awbin, XF_CV_DEPTH_awb_out>(
						awb_in1, awb_out, awb_hist0, awb_hist1, gain0, gain1, height, width, awb_thresh);
					xf::cv::ispStats<MAX_ZONES, STATS_SIZE_AWB, FINAL_BINS_NUM, MERGE_BINS, XF_GTM_T, NUM_OUT_CH, XF_HEIGHT, XF_WIDTH,
									 XF_NPPC, XF_CV_DEPTH_awbin>(awb_in2, awb_stats, awb_max_bins, roi_tlx, roi_tly, roi_brx, roi_bry,
																 zone_col_num, zone_row_num, inputmin, inputmax1, outputmin,
																 outputmax1);
					xf::cv::colorcorrectionmatrix<XF_CCM_TYPE, XF_GTM_T, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_awb_out,
												  XF_CV_DEPTH_ccm>(awb_out, ccm_out);
					xf::cv::gammacorrection<XF_GTM_T, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_ccm, XF_CV_DEPTH_dst>(
						ccm_out, gamma_out, gamma_lut);

					function_3dlut_fifo<XF_GTM_T, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_dst, XF_CV_DEPTH_lut_out,
										XF_CV_DEPTH_3dlut>(gamma_out, lut_out, lut, lutDim, height, width);
					function_csc_or_mat_array<XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_lut_out>(lut_out, img_out, height,
																										   width);
			}

The ISP 24bit Pipeline design is validated on zcu102 board at 150 MHz frequency. 

.. table:: Table: Resource Utilization Summary for a 1920x1080 Image

    +----------------+---------------------+----------------+-------------+-----------+--------+------+
    | Operating Mode | Operating Frequency |              Utilization Estimate                        |
    |                |                     |                                                          |
    |                | (MHz)               |                                                          |
    +                +                     +----------------+-------------+-----------+--------+------+
    |                |                     | LUT            | FF          | BRAM_18k  | DSP    | URAM |
    +================+=====================+================+=============+===========+========+======+
    | 1 Pixel        | 150                 | 35734          | 38747       | 60        | 275    | 0    |
    +----------------+---------------------+----------------+-------------+-----------+--------+------+




.. table:: Table: Performance Estimate Summary for a 1920x1080 Image

    +----------------+---------------------+------------------+
    | Operating Mode | Operating Frequency | Latency Estimate |
    |                |                     |                  |
    |                | (MHz)               |                  |
    +                +                     +------------------+
    |                |                     | Max (ms)         |
    +================+=====================+==================+
    | 1 pixel        | 150                 | 15.1ms           |
    +----------------+---------------------+------------------+







