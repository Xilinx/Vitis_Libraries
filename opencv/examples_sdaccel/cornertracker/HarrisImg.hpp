void HarrisImg(ap_uint<INPUT_PTR_WIDTH> *inHarris,
				   unsigned int *list,				   
				   unsigned int *params, 
				   int harris_rows,int harris_cols,uint16_t Thresh,uint16_t k,uint32_t *nCorners, bool harris_flag)
				   {

const int pROWS = HEIGHT;
  const int pCOLS = WIDTH;

	xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> in_harris_mat(harris_rows,harris_cols);
#pragma HLS stream variable=in_harris_mat.data depth=2
	xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> out_harris_mat(harris_rows,harris_cols);
#pragma HLS stream variable=out_harris_mat.data depth=2


	{
		#pragma HLS DATAFLOW
		 
		xf::Array2xfMat<INPUT_PTR_WIDTH,XF_8UC1,HEIGHT,WIDTH,XF_NPPC1>(inHarris,in_harris_mat);
		xf::cornerHarris<FILTER_WIDTH,BLOCK_WIDTH,NMS_RADIUS,XF_8UC1,HEIGHT,WIDTH,XF_NPPC1>(in_harris_mat, out_harris_mat, Thresh, k);

		xf::cornersImgToList<MAXCORNERS,XF_8UC1,HEIGHT,WIDTH,XF_NPPC1>(out_harris_mat, list, nCorners);
	}
	}