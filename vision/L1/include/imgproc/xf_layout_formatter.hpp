/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __XF_LAYOUT_FORMATTER_HPP__
#define __XF_LAYOUT_FORMATTER_HPP__

#include "ap_int.h"
#include "common/xf_types.hpp"
#include "hls_stream.h"

namespace xf {
namespace cv {

template <int PTR_WIDTH, int TYPE, int ROWS, int COLS, int NPC>
void intrStrm2OutMat(hls::stream<ap_uint<PTR_WIDTH> >& srcStrm,
                     ap_uint<PTR_WIDTH>* out_mat,
                     int rows,
                     int cols,
                     int out_format_channels,
                     int bits_num) {
    int loop_count = (((rows * cols * out_format_channels * bits_num) + PTR_WIDTH - 1) / PTR_WIDTH);

    for (int i = 0; i < loop_count; i++) {
// clang-format off
            #pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS*COLS/PTR_WIDTH
            #pragma HLS PIPELINE
        // clang-format on
        ap_uint<PTR_WIDTH> val = srcStrm.read();
        out_mat[i] = val;
    }
}

// Multi-stream consumer for NCHW format - reads from 4 streams in parallel
template <int PTR_WIDTH, int TYPE, int ROWS, int COLS, int NPC>
void intrStrm2OutMat_4streams(hls::stream<ap_uint<PTR_WIDTH> >& srcStrm1,
                              hls::stream<ap_uint<PTR_WIDTH> >& srcStrm2,
                              hls::stream<ap_uint<PTR_WIDTH> >& srcStrm3,
                              hls::stream<ap_uint<PTR_WIDTH> >& srcStrm4,
                              ap_uint<PTR_WIDTH>* out_mat1,
                              ap_uint<PTR_WIDTH>* out_mat2,
                              ap_uint<PTR_WIDTH>* out_mat3,
                              ap_uint<PTR_WIDTH>* out_mat4,
                              int rows,
                              int cols,
                              int out_format_channels,
                              int bits_num,
                              int out_pixel_channels) {
    int loop_count = (((rows * cols * out_format_channels * bits_num) + PTR_WIDTH - 1) / PTR_WIDTH);

    // Read from all streams in parallel (interleaved)
    for (int i = 0; i < loop_count; i++) {
// clang-format off
            #pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS*COLS/PTR_WIDTH
            #pragma HLS PIPELINE II=1
        // clang-format on
        ap_uint<PTR_WIDTH> val1 = srcStrm1.read();
        ap_uint<PTR_WIDTH> val2 = srcStrm2.read();
        ap_uint<PTR_WIDTH> val3 = srcStrm3.read();

        out_mat1[i] = val1;
        out_mat2[i] = val2;
        out_mat3[i] = val3;

        if (out_pixel_channels == 4) {
            ap_uint<PTR_WIDTH> val4 = srcStrm4.read();
            out_mat4[i] = val4;
        }
    }
}

template <int OUT_PTR_WIDTH,
          int INTYPE,
          int ROWS,
          int COLS,
          int NPC,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
void format_nhwc(xf::cv::Mat<INTYPE, ROWS, COLS, NPC, XFCVDEPTH_IN>& in_mat,
                 hls::stream<ap_uint<OUT_PTR_WIDTH> >& intr_out_stm,
                 int data_type,
                 int out_channels,
                 int bits_num) {
    const int bitdepth = XF_DTPIXELDEPTH(INTYPE, NPC);
    const int channels = XF_CHANNELS(INTYPE, NPC);
    short int width = in_mat.cols >> XF_BITSHIFT(NPC);
    unsigned char out_channels_r = out_channels;

    // Pre-compute commonly used values
    const unsigned short int_pack_bits = (out_channels_r * bits_num * NPC);
    const unsigned short y_pack_max_bits = int_pack_bits - 1;

    ap_uint<OUT_PTR_WIDTH> y_pack_out = 0;
    unsigned short bitcount = 0;
    ap_uint<OUT_PTR_WIDTH> remainder = 0;
    unsigned short remainder_bits = 0;

ROW_LOOP:
    for (int i = 0; i < in_mat.rows; i++) {
#pragma HLS LOOP_TRIPCOUNT min = 1 max = ROWS
#pragma HLS LOOP_FLATTEN OFF
    COL_LOOP:
        for (int j = 0; j < width; j++) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = 1 max = COLS / NPC

            XF_TNAME(INTYPE, NPC) x_pack = in_mat.read(j + i * width);
            XF_TNAME(INTYPE, NPC) y_pack = 0;

            // Convert input format to output format
            if ((bitdepth == bits_num) && (channels == out_channels_r)) {
                y_pack = x_pack;
            } else {
                // Extract MSB bits from input pack and put in output pack
                for (int n = 0; n < NPC; n++) {
#pragma HLS UNROLL
                    short n_offset_y = n * channels * bits_num;
                    short n_offset_x = n * channels * bitdepth;
                    for (int c = 0; c < channels; c++) {
#pragma HLS UNROLL
                        short y_high_pos = n_offset_y + (c + 1) * bits_num - 1;
                        short y_low_pos = n_offset_y + c * bits_num;
                        short x_high_pos = n_offset_x + (c + 1) * bitdepth - 1;
                        short x_low_pos = n_offset_x + (c + 1) * bitdepth - bits_num;
                        y_pack.range(y_high_pos, y_low_pos) = x_pack.range(x_high_pos, x_low_pos);
                    }
                }
            }

            // Calculate space available
            unsigned short space_available = OUT_PTR_WIDTH - bitcount;
            bool will_overflow = (int_pack_bits > space_available);

            // Pack data - either fits entirely or needs split
            if (!will_overflow) {
                // Entire y_pack fits
                y_pack_out.range(bitcount + int_pack_bits - 1, bitcount) = y_pack.range(y_pack_max_bits, 0);
                bitcount += int_pack_bits;
            } else {
                if (space_available > 0) {
                    y_pack_out.range(OUT_PTR_WIDTH - 1, bitcount) = y_pack.range(space_available - 1, 0);
                }
                remainder_bits = int_pack_bits - space_available;
                remainder.range(remainder_bits - 1, 0) = y_pack.range(y_pack_max_bits, space_available);
                bitcount = OUT_PTR_WIDTH;
            }

            // Write when full (at most once per iteration)
            if (bitcount >= OUT_PTR_WIDTH) {
                intr_out_stm.write(y_pack_out);
                y_pack_out = 0;
                bitcount = 0;

                // Move remainder to new pack
                if (remainder_bits > 0) {
                    y_pack_out.range(remainder_bits - 1, 0) = remainder.range(remainder_bits - 1, 0);
                    bitcount = remainder_bits;
                    remainder_bits = 0;
                }
            }
        } // end-width-loop
    }     // end-row-loop

    // Flush any remaining bits
    if (bitcount > 0) {
        intr_out_stm.write(y_pack_out);
    }
}

template <int OUT_PTR_WIDTH,
          int INTYPE,
          int OUTTYPE,
          int ROWS,
          int COLS,
          int NPC,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
void format_hcwnc(xf::cv::Mat<INTYPE, ROWS, COLS, NPC, XFCVDEPTH_IN>& in_mat,
                  hls::stream<ap_uint<OUT_PTR_WIDTH> >& intr_out_stm,
                  int data_type,
                  int data_order,
                  int out_pixel_channels,
                  int out_format_channels,
                  int bits_num) {
    int write_index = 0;
    XF_TNAME(INTYPE, NPC) x_pack = 0;
    const int bitdepth = XF_DTPIXELDEPTH(INTYPE, NPC);
    const int channels = XF_CHANNELS(INTYPE, NPC);
    const int out_channels_c = XF_CHANNELS(OUTTYPE, NPC);

    short int width = in_mat.cols >> XF_BITSHIFT(NPC);

    unsigned char iter_index = 0;

    int cnt = 0;
    int new_cnt = 0;

    unsigned char loop_end = (out_channels_c * bitdepth) / (out_format_channels * bits_num);

    if (loop_end < 1) {
        loop_end = 1;
    }
    int stride = 4;
    if (data_order == layout_format::XF_HCWNC8) {
        stride = 8;
    }
ROW_LOOP:
    for (int i = 0; i < in_mat.rows; i++) { // H

#pragma HLS LOOP_TRIPCOUNT min = 1 max = ROWS
#pragma HLS LOOP_FLATTEN OFF
        XF_TNAME(OUTTYPE, NPC) out_pack = 0, y_pack_out = 0;
    COL_LOOP:
        for (int j = 0; j < width; j++) { // W
#pragma HLS LOOP_TRIPCOUNT min = 1 max = COLS / NPC
#pragma HLS PIPELINE II = 1
            x_pack = in_mat.read(j + i * width);

        NPC_LOOP:
            for (int k = 0; k < XF_NPIXPERCYCLE(NPC); k++) { // NPC
#pragma HLS UNROLL
#pragma HLS LOOP_TRIPCOUNT min = 1 max = NPC
                XF_DTUNAME(INTYPE, NPC)
                pix_npc = x_pack.range(((k + 1) * bitdepth * channels) - 1, k * bitdepth * channels);
                XF_DTUNAME(OUTTYPE, NPC) out_pix = 0;
            STRIDE_LOOP:
                for (int l = 1; l <= out_channels_c; l++) { // S
#pragma HLS UNROLL
#pragma HLS LOOP_TRIPCOUNT min = 1 max = out_channels_c
                    if (l <= out_pixel_channels) {
                        out_pix.range((l * bits_num) - 1, (l - 1) * bits_num) =
                            pix_npc.range((l * bitdepth) - 1, (l * bitdepth) - bits_num);
                    } else {
                        out_pix.range((l * bits_num) - 1, (l - 1) * bits_num) = 0;
                    }
                }

                out_pack.range(((k + 1) * bits_num * stride) - 1, k * bits_num * stride) = out_pix;
            }
            y_pack_out.range(((iter_index + 1) * out_format_channels * bits_num * NPC) - 1,
                             iter_index * (out_format_channels * bits_num * NPC)) =
                out_pack.range((out_format_channels * bits_num * NPC) - 1, 0);
            iter_index++;
            cnt++;

            if (cnt == loop_end) { // enough data found to be packed
                intr_out_stm.write(y_pack_out);
                cnt = 0;
                iter_index = 0;
                y_pack_out = 0;
            }
        }
    }
}

template <int OUT_PTR_WIDTH_NCHW,
          int INTYPE,
          int ROWS,
          int COLS,
          int NPC,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
void format_nchw(xf::cv::Mat<INTYPE, ROWS, COLS, NPC, XFCVDEPTH_IN>& in_mat,
                 hls::stream<ap_uint<OUT_PTR_WIDTH_NCHW> >& intr_out_stm1,
                 hls::stream<ap_uint<OUT_PTR_WIDTH_NCHW> >& intr_out_stm2,
                 hls::stream<ap_uint<OUT_PTR_WIDTH_NCHW> >& intr_out_stm3,
                 hls::stream<ap_uint<OUT_PTR_WIDTH_NCHW> >& intr_out_stm4,
                 int data_type,
                 int out_pixel_channels,
                 int bits_num) {
    int write_index = 0;
    const int bitdepth = XF_DTPIXELDEPTH(INTYPE, NPC);
    const int channels = XF_CHANNELS(INTYPE, NPC);

    short int width = in_mat.cols >> XF_BITSHIFT(NPC);
    unsigned char k = 0;
    int cnt = 0;
    int new_cnt = 0;
    ap_uint<bitdepth * NPC> y_pack_out[channels] = {0};

    unsigned char loop_end = bitdepth / bits_num; // As channels are split across multiple ports
                                                  //***************NCHW******************
ROW_LOOP:
    for (int i = 0; i < in_mat.rows; i++) {
// clang-format off
			#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
					COL_LOOP:
					for (int j = 0; j < width; j++) {
						#pragma HLS PIPELINE II=1
						#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
						
						XF_TNAME(INTYPE, NPC) x_pack = in_mat.read(j + i* width);
						ap_uint<bitdepth*NPC> out_pack[channels] = {0};
						NPC_LOOP:
						for (int m = 0; m < XF_NPIXPERCYCLE(NPC); m++) {					
					#pragma HLS UNROLL

							XF_DTUNAME(INTYPE, NPC) pix_npc = x_pack.range( ((m+1)*bitdepth*channels)-1, m*bitdepth*channels );
							CHNL_LOOP:
							for (int l = 1; l <= XF_CHANNELS(INTYPE, NPC); l++) {
				
				#pragma HLS UNROLL
								out_pack[l-1].range( ((m+1)*bits_num)-1, m*bits_num ) = pix_npc.range( (l*bitdepth)-1, (l*bitdepth)-bits_num);	
							}
						}
						PACK_LOOP:
						for(int w=0; w<XF_CHANNELS(INTYPE, NPC); w++){
							y_pack_out[w].range(((k+1)*bits_num*NPC)-1, 
							k*(bits_num*NPC)) = out_pack[w].range((bits_num*NPC)-1, 0);
						}
						k++;
						cnt++;
						
						if(cnt == loop_end){ //enough data found to be packed
							intr_out_stm1.write(y_pack_out[0]);
							intr_out_stm2.write(y_pack_out[1]);
							intr_out_stm3.write(y_pack_out[2]);
							if(out_pixel_channels == 4){
								intr_out_stm4.write(y_pack_out[3]);
							}
							y_pack_out[0] = 0; y_pack_out[1] = 0; y_pack_out[2] = 0;
							if(out_pixel_channels == 4){
								y_pack_out[3] = 0;
							}
							write_index++;
							cnt = 0;
							k = 0;
						}
					}
						
				}
		}


template<int OUTPTR_WIDTH, int INTYPE, int ROWS, int COLS, int NPC, int XFCVDEPTH_IN=2>
void nhwc_formatter(xf::cv::Mat<INTYPE, ROWS, COLS, NPC, XFCVDEPTH_IN>& in_mat,
                    ap_uint<OUTPTR_WIDTH>* out_mat,
					int data_type, int out_pixel_channels, int bits_num){
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW
	hls::stream<ap_uint<OUTPTR_WIDTH> >	intr_out_stm;
	#pragma HLS stream depth=COLS/4 variable=intr_out_stm
	#pragma HLS bind_storage variable=intr_out_stm type=fifo impl=bram

	format_nhwc<OUTPTR_WIDTH, INTYPE>(in_mat, intr_out_stm, data_type, out_pixel_channels, bits_num);
	intrStrm2OutMat<OUTPTR_WIDTH, INTYPE, ROWS, COLS, NPC>(intr_out_stm, out_mat, 
	in_mat.rows, in_mat.cols, out_pixel_channels, bits_num);
}


template<int OUTPTR_WIDTH, int INTYPE, int OUTTYPE, int ROWS, int COLS, int NPC, int XFCVDEPTH_IN=2>
void hcwnc_formatter(xf::cv::Mat<INTYPE, ROWS, COLS, NPC, XFCVDEPTH_IN>& in_mat,
                    ap_uint<OUTPTR_WIDTH>* out_mat,
					int data_type, int data_order, int out_pixel_channels, 
					int out_format_channels, int bits_num){
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW

	hls::stream<ap_uint<OUTPTR_WIDTH> >	intr_out_stm;
	#pragma HLS stream depth=COLS variable=intr_out_stm
	#pragma HLS bind_storage variable=intr_out_stm type=fifo impl=bram

	format_hcwnc<OUTPTR_WIDTH, INTYPE, OUTTYPE>(in_mat, intr_out_stm, data_type, data_order, 
	out_pixel_channels, out_format_channels, bits_num);
	intrStrm2OutMat<OUTPTR_WIDTH, OUTTYPE, ROWS, COLS, NPC>(intr_out_stm, out_mat, 
	in_mat.rows, in_mat.cols, out_format_channels, bits_num);
}

template<int OUTPTR_WIDTH, int INTYPE, int OUTTYPE, int ROWS, int COLS, int NPC, int XFCVDEPTH_IN=2>
void nchw_formatter(xf::cv::Mat<INTYPE, ROWS, COLS, NPC, XFCVDEPTH_IN>& in_mat,
                    ap_uint<OUTPTR_WIDTH>* out_mat1,
					ap_uint<OUTPTR_WIDTH>* out_mat2,
					ap_uint<OUTPTR_WIDTH>* out_mat3,
					ap_uint<OUTPTR_WIDTH>* out_mat4,
					int data_type, int out_pixel_channels,
					int out_format_channels,
					int bits_num){
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW
	hls::stream<ap_uint<OUTPTR_WIDTH> > intr_out_stm1;
	hls::stream<ap_uint<OUTPTR_WIDTH> > intr_out_stm2;
	hls::stream<ap_uint<OUTPTR_WIDTH> > intr_out_stm3;
	hls::stream<ap_uint<OUTPTR_WIDTH> > intr_out_stm4;


	#pragma HLS stream depth=COLS/4 variable=intr_out_stm1
	#pragma HLS stream depth=COLS/4 variable=intr_out_stm2
	#pragma HLS stream depth=COLS/4 variable=intr_out_stm3
	#pragma HLS stream depth=COLS/4 variable=intr_out_stm4

	
	#pragma HLS bind_storage variable=intr_out_stm1 type=fifo impl=bram
	#pragma HLS bind_storage variable=intr_out_stm2 type=fifo impl=bram
	#pragma HLS bind_storage variable=intr_out_stm3 type=fifo impl=bram
	#pragma HLS bind_storage variable=intr_out_stm4 type=fifo impl=bram

	format_nchw<OUTPTR_WIDTH, INTYPE>(in_mat, intr_out_stm1, intr_out_stm2, 
	intr_out_stm3, intr_out_stm4, data_type, out_pixel_channels, bits_num);
	
	// Use multi-stream consumer to read from all streams in parallel
	intrStrm2OutMat_4streams<OUTPTR_WIDTH, OUTTYPE, ROWS, COLS, NPC>(
		intr_out_stm1, intr_out_stm2, intr_out_stm3, intr_out_stm4,
		out_mat1, out_mat2, out_mat3, out_mat4,
		in_mat.rows, in_mat.cols, out_format_channels, bits_num, out_pixel_channels);
}

// NHWC - HCWNC4 - _XF_HCWNC8_ - NCHW
template <int OUT_PTR_WIDTH,
          int OUT_PTR_WIDTH_NCHW,
          int DATAORDER,
          int DATATYPE,
          int INTYPE,
          int OUTTYPE,
          int OUT_TYPE_NCHW,
          int ROWS,
          int COLS,
          int NPC,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT
	> 
	void layout_formatter(xf::cv::Mat<INTYPE, ROWS, COLS, NPC, XFCVDEPTH_IN>& in_mat,
                      ap_uint<OUT_PTR_WIDTH> *out_mat,
					  ap_uint<OUT_PTR_WIDTH_NCHW> *out_mat1,
					  ap_uint<OUT_PTR_WIDTH_NCHW> *out_mat2,
					  ap_uint<OUT_PTR_WIDTH_NCHW> *out_mat3,
					  ap_uint<OUT_PTR_WIDTH_NCHW> *out_mat4,
					  int data_order, int data_type, int out_pixel_channels
			)
		{

			const int in_channels_c = XF_CHANNELS(INTYPE, NPC);

			#ifndef __SYNTHESIS__

				if ((DATAORDER & (1 << data_order)) == 0) {
					assert(false && "Runtime data-order not supported as it was not selected during compile time");
				}
				if ((DATATYPE & (1 << data_type)) == 0) {
					assert(false && "Runtime data-type not supported as it was not selected during compile time");
				}
				if (out_pixel_channels > in_channels_c) {
					assert(false && "Runtime channels cannot be higher than compile time selection");
				}

			#endif

			short int width = in_mat.cols >> XF_BITSHIFT(NPC);
			short int height = in_mat.rows;
			int write_index = 0;
			int bits_num = 32; //Default FP32
			unsigned char out_format_channels = 0;
			int cnt = 0;
			unsigned char k = 0;
			if(data_type == data_types::INT8){
				bits_num = 8;							
			}
			else if((data_type == data_types::BF16) || (data_type == data_types::FP16)){ //FP16 and BF16
				bits_num = 16;
			}

			if(data_order == layout_format::XF_NHWC){
				out_format_channels = out_pixel_channels;
			}
			else if(data_order == layout_format::XF_NCHW){
				out_format_channels = 1;
			}
			else if(data_order == layout_format::XF_HCWNC4){
				out_format_channels = 4;
			}
			else if(data_order == layout_format::XF_HCWNC8){
				out_format_channels = 8;
			}	

	//#pragma HLS DATAFLOW
	//***************NHWC******************		
			if(DATAORDER & 0x0001 ){
				if(data_order == layout_format::XF_NHWC){
					nhwc_formatter<OUT_PTR_WIDTH, INTYPE, ROWS, COLS, NPC>(in_mat, out_mat, 
					data_type, out_pixel_channels, bits_num);
				}
			} //NHWC

	//**************HCWNC4 - _XF_HCWNC8_*****************	
			if(DATAORDER & 0x0002 || DATAORDER & 0x0004){
				if(data_order == layout_format::XF_HCWNC4 || data_order == layout_format::XF_HCWNC8){

					hcwnc_formatter<OUT_PTR_WIDTH, INTYPE, OUTTYPE, ROWS, COLS, NPC>(in_mat, out_mat, data_type, 
					data_order, out_pixel_channels, out_format_channels, bits_num);
				}
			} //HCWNC4 - _XF_HCWNC8_

	//***************NCHW******************
			if(DATAORDER & 0x0008){
				if(data_order == layout_format::XF_NCHW) {
						nchw_formatter<OUT_PTR_WIDTH_NCHW, INTYPE, OUT_TYPE_NCHW, ROWS, COLS, NPC>(in_mat, out_mat1, 
						out_mat2, out_mat3, out_mat4, data_type, out_pixel_channels, out_format_channels, bits_num);
				}
			}
		}
	} //cv
} // xf		

#endif
