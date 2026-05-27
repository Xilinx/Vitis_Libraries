
#include "xf_isp_types.h"

static bool flag = 0;

static uint32_t hist0_awb[3][HIST_SIZE] = {0};
static uint32_t hist1_awb[3][HIST_SIZE] = {0};

template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC = 1, int XFCVDEPTH_IN_1 = 2>
void function_awb(xf::cv::Mat<SRC_T, ROWS, COLS, NPC>& bgr_out,
                  uint32_t hist0[3][HIST_SIZE],
                  uint32_t hist1[3][HIST_SIZE],
                  unsigned short height,
                  unsigned short width,
                  float thresh,
                  float min_Value[3],
                  float max_Value[3]) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on	
	//xf::cv::Mat<XF_DST_T, XF_HEIGHT, XF_WIDTH, XF_NPPC> imgop(height, width);

	float inputMin = 0.0f;
    float inputMax = (1 << (XF_DTPIXELDEPTH(XF_SRC_T, NPC))) - 1; // 65535.0f;
    float outputMin = 0.0f;
    float outputMax = (1 << (XF_DTPIXELDEPTH(XF_SRC_T, NPC))) - 1; // 65535.0f;
	
	// clang-format off
//#pragma HLS DATAFLOW
    // clang-format on
    xf::cv::AWBhistogram<SRC_T, SRC_T, ROWS, COLS, NPC, XF_USE_URAM, WB_TYPE, HIST_SIZE, XFCVDEPTH_IN_1,
                         XFCVDEPTH_IN_1>(bgr_out, hist0, hist1, thresh, inputMin, inputMax, outputMin, outputMax,
                                         min_Value, max_Value);
}

void AWB(ap_uint<INPUT_PTR_WIDTH>* img_inp,
         unsigned short height,
         unsigned short width,
         float thresh,
         uint32_t hist0[3][HIST_SIZE],
         uint32_t hist1[3][HIST_SIZE],
         float min_Value[3],
         float max_Value[3]) {
    xf::cv::Mat<XF_8UC4, XF_HEIGHT, XF_WIDTH, XF_NPPC> demosaic_out(height, width);
    xf::cv::Mat<XF_DST_T, XF_HEIGHT, XF_WIDTH, XF_NPPC> bgr_out(height, width);

#pragma HLS DATAFLOW
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, XF_8UC4, XF_HEIGHT, XF_WIDTH, XF_NPPC>(img_inp, demosaic_out);
    xf::cv::bgra2bgr<XF_8UC4, XF_DST_T, XF_HEIGHT, XF_WIDTH, XF_NPPC>(demosaic_out, bgr_out);
    function_awb<XF_DST_T, XF_DST_T, XF_HEIGHT, XF_WIDTH, XF_NPPC>(bgr_out, hist0, hist1, height, width, thresh,
                                                                   min_Value, max_Value);
}

extern "C" {
void AWB_accel(
    ap_uint<INPUT_PTR_WIDTH>* img_inp, int height, int width, float thresh, float* min_Value, float* max_Value) {
// clang-format off
#pragma HLS INTERFACE m_axi     port=img_inp  offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi     port=min_Value  offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi     port=max_Value  offset=slave bundle=gmem2
// clang-format on

// clang-format off
#pragma HLS ARRAY_PARTITION variable=hist0_awb complete dim=1
#pragma HLS ARRAY_PARTITION variable=hist1_awb complete dim=1

    // clang-format on

    if (!flag) {
        AWB(img_inp, height, width, thresh, hist0_awb, hist1_awb, min_Value, max_Value);
        flag = 1;

    } else {
        AWB(img_inp, height, width, thresh, hist1_awb, hist0_awb, min_Value, max_Value);
        flag = 0;
    }
}
}
