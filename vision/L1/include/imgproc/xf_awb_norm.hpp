//#include <iostream>
//#include <stdio.h>
namespace xf {
namespace cv {

template <int SRC_T, int NPC, int HISTSIZE>
void awbPreNorm(short int height,
                short int width,
                float p,
                float inputMin,
                float inputMax,
                uint32_t hist[3][HISTSIZE],
                float min_Value[3],
                float max_Value[3]) {
    constexpr int STEP = XF_DTPIXELDEPTH(SRC_T, NPC);
    ap_uint<STEP + 1> bins = HISTSIZE; // number of bins at each histogram level

    ap_uint<STEP + 1> nElements = HISTSIZE; // int(pow((float)bins, (float)depth));

    int total = width * height;
    ap_fixed<STEP + 8, STEP + 2> min_vals = inputMin - 0.5f;
    ap_fixed<STEP + 8, STEP + 2> max_vals = inputMax + 0.5f;
    ap_fixed<STEP + 8, STEP + 2> minValue[3] = {min_vals, min_vals, min_vals}; //{-0.5, -0.5, -0.5};
    ap_fixed<STEP + 8, STEP + 2> maxValue[3] = {max_vals, max_vals, max_vals}; //{12287.5, 16383.5, 12287.5};
    ap_fixed<32, 16> p_float = p;
    ap_fixed<STEP + 8, 8> s1 = ap_fixed<STEP + 8, 8>(p_float);
    ap_fixed<STEP + 8, 8> s2 = ap_fixed<STEP + 8, 8>(p_float);

    int rval = s1 * total / 100;
    int rval1 = (100 - s2) * total / 100;

    for (int j = 0; j < 3; ++j)
    // searching for s1 and s2
    {
        ap_uint<STEP + 2> p1 = 0;
        ap_uint<STEP + 2> p2 = bins - 1;
        ap_uint<32> n1 = 0;
        ap_uint<32> n2 = total;

        ap_fixed<STEP + 8, STEP + 2> interval = (max_vals - min_vals) / bins;

        for (int k = 0; k < 1; ++k)
        // searching for s1 and s2
        {
            int value = hist[j][p1];
            int value1 = hist[j][p2];

            while (n1 + hist[j][p1] < rval && p1 < HISTSIZE) {
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min = 255 max = 255
#pragma HLS DEPENDENCE variable = hist array intra false
#pragma HLS DEPENDENCE variable = minValue intra false
                n1 += hist[j][p1++];
                minValue[j] += interval;
            }

            while (n2 - hist[j][p2] > rval1 && p2 != 0) {
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min = 255 max = 255
#pragma HLS DEPENDENCE variable = hist array intra false
#pragma HLS DEPENDENCE variable = maxValue intra false
                n2 -= hist[j][p2--];
                maxValue[j] -= interval;
            }
        }
    }

    for (short l = 0; l < 3; l++) {
        min_Value[l] = minValue[l];
        max_Value[l] = maxValue[l];
    }
}

template <int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int NPC = 1,
          int XFCVDEPTH_IN_1 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT_1 = _XFCVDEPTH_DEFAULT,
          int HISTSIZE>
void AWBNormalizationkernel_post(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1>& src,
                                 xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT_1>& dst,
                                 float inputMax,
                                 float min_Value[3],
                                 float max_Value[3]
                                 /* ,
                            uint32_t hist[3][HISTSIZE],
                            float p,
                            float inputMin,
                            float inputMax,
                            float outputMin,
                            float outputMax */) {
// clang-format off
#pragma HLS INLINE OFF

    short width = dst.cols >> XF_BITSHIFT(NPC);
    short height = dst.rows;
    const int STEP = XF_DTPIXELDEPTH(SRC_T, NPC);
	XF_TNAME(SRC_T, NPC) in_buf_n, in_buf_n1, out_buf_n;
	
    ap_fixed<STEP + 8, STEP + 2> minValue[3];
    ap_fixed<STEP + 8, STEP + 2> maxValue[3];
	
	for(short l=0; l<3; l++){
		minValue[l] = min_Value[l];
		maxValue[l] = max_Value[l];
	}

    ap_fixed<STEP + 8, STEP + 2> maxmin_diff[3];
// clang-format off
#pragma HLS ARRAY_PARTITION variable = maxmin_diff complete dim = 0
    // clang-format on

    maxmin_diff[0] = maxValue[0] - minValue[0];
    maxmin_diff[1] = maxValue[1] - minValue[1];
    maxmin_diff[2] = maxValue[2] - minValue[2];

    printf("valuesmin max :%f %f %f %f %f %f\n", (float)maxValue[0], (float)maxValue[1], (float)maxValue[2],
           (float)minValue[0], (float)minValue[1], (float)minValue[2]);
    int pval = 0, read_index = 0, write_index = 0;
    ap_uint<13> row, col;

    ap_fixed<STEP + 16, 2> inv_val[3];

    if (maxmin_diff[0] != 0) inv_val[0] = ((ap_fixed<STEP + 16, 2>)1 / maxmin_diff[0]);
    if (maxmin_diff[1] != 0) inv_val[1] = ((ap_fixed<STEP + 16, 2>)1 / maxmin_diff[1]);
    if (maxmin_diff[2] != 0) inv_val[2] = ((ap_fixed<STEP + 16, 2>)1 / maxmin_diff[2]);

    ap_fixed<STEP + 8, STEP + 2> newmax = inputMax;
    ap_fixed<STEP + 8, STEP + 2> newmin = 0.0f;
    ap_fixed<STEP + 8, STEP + 2> newdiff = newmax - newmin;

Row_Loop1:
    for (int row = 0; row < height; row++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
    // clang-format on

    Col_Loop1:
        for (int col = 0; col < width; col++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=COLS/NPC max=COLS/NPC
#pragma HLS pipeline II=1
#pragma HLS LOOP_FLATTEN OFF
            // clang-format on
            in_buf_n = src.read(read_index++);

            ap_fixed<STEP + 8, STEP + 2> value = 0;
            ap_fixed<STEP + STEP + 16, STEP + 8> divval = 0;
            ap_fixed<STEP + 16, STEP + 16> finalmul = 0;
            ap_int<32> dstval;

            for (int p = 0, bit = 0; p < XF_NPIXPERCYCLE(NPC) * XF_CHANNELS(SRC_T, NPC); p++, bit = p % 3) {
// clang-format off
#pragma HLS unroll
                // clang-format on
                XF_CTUNAME(SRC_T, NPC)
                val = in_buf_n.range(p * STEP + STEP - 1, p * STEP);
                value = val - minValue[bit];
                divval = value * inv_val[p % 3];
                finalmul = divval * newdiff;
                dstval = (int)(finalmul + newmin);
                if (dstval.range(31, 31) == 1) {
                    dstval = 0;
                }
                out_buf_n.range(p * STEP + STEP - 1, p * STEP) = xf_satcast_awb<XF_CTUNAME(SRC_T, NPC)>(dstval);
            }

            dst.write(row * width + col, out_buf_n);
        }
    }
}

template <int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int NPC = 1,
          int XFCVDEPTH_IN_1 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT_1 = _XFCVDEPTH_DEFAULT>
void bgra2bgr(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1>& src,
              xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT_1>& dst) {
    short width = src.cols >> XF_BITSHIFT(NPC);
    short height = src.rows;
    const short bitdepth = XF_PIXELDEPTH(SRC_T);
    const short src_pixelwidth = XF_PIXELWIDTH(SRC_T, NPC);
    const short dst_pixelwidth = XF_PIXELWIDTH(DST_T, NPC);
Row_Loop:
    for (int row = 0; row < height; row++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
    // clang-format on

    Col_Loop:
        for (int col = 0; col < width; col++) {
#pragma HLS LOOP_TRIPCOUNT min = COLS / NPC max = COLS / NPC
            XF_TNAME(SRC_T, NPC) in_val = src.read(row * width + col);
            XF_TNAME(DST_T, NPC) out_val = 0;
            for (int nppc = 0; nppc < NPC; nppc++) {
                ap_uint<dst_pixelwidth> val1 =
                    in_val.range((nppc * src_pixelwidth) + dst_pixelwidth - 1, (nppc * src_pixelwidth));
                out_val.range((nppc * dst_pixelwidth) + dst_pixelwidth - 1, (nppc * dst_pixelwidth)) = val1;
            }
            dst.write((row * width + col), out_val);
        }
    }
}
}
}
