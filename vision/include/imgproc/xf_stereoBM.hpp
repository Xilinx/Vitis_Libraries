/***************************************************************************
Copyright (c) 2019, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ***************************************************************************/

#ifndef _XF_STEREO_LBM_HPP_
#define _XF_STEREO_LBM_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"
#include "imgproc/xf_sobel.hpp"

#define __XF_MIN(a,b) a<b?a:b

namespace xf{

template  < unsigned int _Num, unsigned int _I=_Num/2>
class xFBitWidth
{
public:
    static const unsigned int Value = 1 + xFBitWidth<_Num,_I/2>::Value;
};

template <unsigned int _Num>
class xFBitWidth<_Num, 0>
{
public:
    static const unsigned int Value = 1;
};

template<typename T>
T xFabsdiff2(T a, T b)
{
#pragma HLS INLINE
  int x = a-b;
#pragma HLS RESOURCE variable=x core=AddSubnS
  T r;
  if (x > 0)
  {
    r = x;
  }
  else
  {
    r = -x;
  }
  return r;
}

template<int SIZE>
class xFMinSAD
{
public:
  template <typename T, typename T_idx>
  static void find(T a[SIZE], T_idx &loc, T &val)
  {
#pragma HLS INLINE
#pragma HLS array_partition variable=a complete dim=0

    T a1[SIZE/2];
    T a2[SIZE-SIZE/2];

    for(int i = 0; i < SIZE/2; i++)
    {
#pragma HLS UNROLL
      a1[i] = a[i];
    }
    for(int i = 0; i < SIZE-SIZE/2; i++)
    {
#pragma HLS UNROLL
      a2[i] = a[i+SIZE/2];
    }

    T_idx l1,l2;
    T v1,v2;
    xFMinSAD<SIZE/2>::find(a1,l1,v1);
    xFMinSAD<SIZE-SIZE/2>::find(a2,l2,v2);

    if(v2 <= v1)
    {
      val = v2;
      loc = l2+SIZE/2;
    }
    else
    {
      val = v1;
      loc = l1;
    }
  }
};

template<>
class xFMinSAD<1>
{
public:
  template <typename T, typename T_idx>
  static void find(T a[1], T_idx &loc, T &val)
  {
#pragma HLS INLINE

    loc = 0;
    val = a[0];
  }
};

template<>
class xFMinSAD<2>
{
public:
  template <typename T, typename T_idx>
  static void find(T a[2], T_idx &loc, T &val)
  {
#pragma HLS INLINE
#pragma HLS array_partition variable=a complete dim=0

    T_idx l1=0, l2=1;
    T v1=a[0], v2=a[1];
    if(v2 <= v1)
    {
      val = v2;
      loc = l2;
    }
    else
    {
      val = v1;
      loc = l1;
    }
  }
};

/* TEXTURE THRESHOLD computation */
template<int WSIZE,int NPC,int L_WIN_COLS,int WORDWIDTH>
void xFUpdateTextureSum(unsigned char window[WSIZE][L_WIN_COLS],unsigned char l_tmp[WSIZE],int row,int col,int cap,int text_sum[WSIZE])
{
#pragma HLS INLINE

  int abs_diff[WSIZE];
  int col_sums = 0;

  text_sum_loop1:
  for (int i = 0; i < WSIZE; i++)
  {
#pragma HLS UNROLL
    col_sums += (i > row? 0 : xFabsdiff2((int)(l_tmp[i]), cap)) - (((col < WSIZE) || (i > row) ) ? 0 : xFabsdiff2((int)window[i][WSIZE-1], cap));
  }

  int tmp_prev[2];
  int tmp_int_sums;

  tmp_prev[0] = col>0 ? (int)text_sum[0]:(int)0;
  tmp_prev[1] = col_sums;

  //shift right
  for(int j = WSIZE-1; j >= 1; j--)
  {
#pragma HLS UNROLL
    text_sum[j] = text_sum[j-1];
  }

  //  shift_right<ap_uint<32>, NDISP_UNITS,SAD_COL_SIZE,NPC>(text_sum);
  tmp_int_sums = tmp_prev[0] + tmp_prev[1];
  text_sum[0] = tmp_int_sums;
}

template<typename T,int ROWS,int COLS>
void xFShiftRight(T buf[ROWS][COLS])
{
#pragma HLS INLINE

  shift_right_loop2:
  for(unsigned char j = COLS-1; j >= 1; j--)
  {
#pragma HLS UNROLL
    shift_right_loop1:
    for(unsigned char i = 0; i < ROWS; i++)
    {
#pragma HLS UNROLL
      buf[i][j] = buf[i][j-1];
    }
  }
}

template<int ROWS,int COLS,typename T>
void xFInsertLeft(T buf[ROWS][COLS],T tmp[ROWS])
{
#pragma HLS INLINE
  insert_right_loop1:
  for(unsigned char i = 0; i < ROWS; i++)
  {
#pragma HLS UNROLL
    buf[i][0] = tmp[i];
  }
}

template<int WSIZE, int L_WIN_COLS, int R_WIN_COLS, typename T>
short int xFSADComputeInc(
    T l_win[WSIZE][L_WIN_COLS],
    T r_win_s[WSIZE][R_WIN_COLS],
    unsigned char d,
    unsigned short col,
    short int sad_cols_d[WSIZE])
{
#pragma HLS inline
  short int a_sum = 0, b_sum = 0;
  // compute new column sads;
  for (unsigned char i = 0; i < WSIZE; i++) {
    b_sum += __ABS((unsigned char)l_win[i][0] - (unsigned char)r_win_s[i][d]);
  }
  // valid guard;
  if (col < d) b_sum = 0;
  // get previous sad_cols value;
  a_sum = sad_cols_d[WSIZE-1];
  // shift sad_cols[d];
  for (unsigned char j = WSIZE-1; j > 0; j--) {
    sad_cols_d[j] = sad_cols_d[j-1];
  }
  // fill in sad_cols with newly computed values;
  sad_cols_d[0] = b_sum;

  return (-a_sum+b_sum);
}



template<int ROWS,int COLS,int WORDWIDTH_SRC,int WORDWIDTH_DST,
int WSIZE,int NDISP,int NDISP_UNIT, int SWEEP_FACT, int ROW_TC,
int COL_TC,int BUF_SIZE,
int LWINWIDTH,int RWINWIDTH,int DISPWORDWIDTH,int SADWORDWIDTH,bool USE_URAM>
void xFSADBlockMatching(
    hls::stream<XF_TNAME(WORDWIDTH_SRC,1)> &left,
    hls::stream<XF_TNAME(WORDWIDTH_SRC,1)> &right,
    hls::stream<XF_TNAME(WORDWIDTH_DST,1)>& out,
    xf::xFSBMState<WSIZE, NDISP, NDISP_UNIT>& state,
    short int height, short int width)
{
  //create the left and right line buffers.
  XF_TNAME(WORDWIDTH_SRC,1) left_line_buf[WSIZE][BUF_SIZE];

  if(USE_URAM)
  {
#pragma HLS RESOURCE variable=left_line_buf core=RAM_2P_URAM
#pragma HLS ARRAY_RESHAPE variable=left_line_buf cyclic factor=WSIZE dim=1
  }else{
#pragma HLS ARRAY_PARTITION variable=left_line_buf complete dim=1
  }

  XF_TNAME(WORDWIDTH_SRC,1) right_line_buf[WSIZE][BUF_SIZE];

  if(USE_URAM)
   {
#pragma HLS RESOURCE variable=right_line_buf core=RAM_2P_URAM
#pragma HLS ARRAY_RESHAPE variable=right_line_buf cyclic factor=WSIZE dim=1
   }else{

#pragma HLS ARRAY_PARTITION variable=right_line_buf complete dim=1
   }


  //create the left and right window buffers.
  unsigned char l_window[WSIZE][LWINWIDTH];
#pragma HLS ARRAY_PARTITION variable=l_window complete dim=2
#pragma HLS ARRAY_PARTITION variable=l_window complete dim=1
  unsigned char r_window[WSIZE][RWINWIDTH];
#pragma HLS ARRAY_PARTITION variable=r_window complete dim=2
#pragma HLS ARRAY_PARTITION variable=r_window complete dim=1

  int TMP_INT_MAX_PACK;
  TMP_INT_MAX_PACK = 2147483647;

  short int FILTERED = 0;//((state.minDisparity - 1) << 4);
  unsigned char cap = state.preFilterCap;
  unsigned char l_tmp[WSIZE];
#pragma HLS array_partition variable=l_tmp complete dim=0
  unsigned char r_tmp[WSIZE];
#pragma HLS array_partition variable=r_tmp complete dim=0
  int text_sum[WSIZE];
#pragma HLS ARRAY_PARTITION variable=text_sum complete dim=0
  int sad[NDISP_UNIT];
#pragma HLS array_partition variable=sad complete dim=0

  short int sad_cols[NDISP_UNIT][WSIZE];
#pragma HLS array_partition variable=sad_cols complete dim=0


  int minsad[COLS+WSIZE-1];
  if (USE_URAM){
#pragma HLS RESOURCE variable=minsad core=RAM_S2P_URAM
  }
  XF_TNAME(WORDWIDTH_DST,1) mind[BUF_SIZE];
  if (USE_URAM){
#pragma HLS RESOURCE variable=mind core=RAM_S2P_URAM
  }
  bool skip[BUF_SIZE];
  if (USE_URAM){
#pragma HLS RESOURCE variable=skip core=RAM_S2P_URAM
  }

  loop_row:
  for (unsigned short row = 0; row < height+WSIZE-1; row++) {
#pragma HLS LOOP_TRIPCOUNT min=ROW_TC max=ROW_TC

    loop_mux:
    for (unsigned char sweep = 0; sweep < state.sweepFactor; sweep++) {
#pragma HLS LOOP_TRIPCOUNT min=SWEEP_FACT max=SWEEP_FACT

      loop_sad_init:
      for (unsigned char d = 0; d < NDISP_UNIT; d++) {
#pragma HLS unroll
        sad[d] = 0;
        for (unsigned char i = 0; i < WSIZE; i++) {
#pragma HLS unroll
          sad_cols[d][i] = 0;
        }
      }
      loop_col:
      for (unsigned short col = 0; col < width+WSIZE-1; col++) {
#pragma HLS LOOP_TRIPCOUNT min=COL_TC max=COL_TC

#pragma HLS loop_flatten
#pragma HLS pipeline II=1

        unsigned char tmp_l = cap,tmp_r=cap;

        if (sweep == 0) {
          // load and shifting buffs
          // shift down
          for(unsigned char sd = WSIZE-1; sd > 0; sd--) {
#pragma HLS unroll
            left_line_buf[sd][col] = left_line_buf[sd-1][col];
          }

          for(unsigned char sd = WSIZE-1; sd > 0; sd--) {
#pragma HLS unroll
            right_line_buf[sd][col] = right_line_buf[sd-1][col];
          }

          if (!(row < (WSIZE-1)/2 || row >= height+(WSIZE-1)/2 || col < (WSIZE-1)/2 || col >= width+(WSIZE-1)/2)) {
            tmp_l = left.read();
            tmp_r = right.read();
          }
          // insert bottom
          left_line_buf[0][col] = tmp_l;
          right_line_buf[0][col] = tmp_r;
          loop_get_data_from_linebuff:
          for (unsigned char i = 0; i < WSIZE; i++) {
            l_tmp[i] = left_line_buf[i][col];
            r_tmp[i] = right_line_buf[i][col];
          }
        } else {
          unsigned short offset = sweep * NDISP_UNIT;
          loop_get_data_from_linebuff_with_offset:
          for (unsigned char i = 0; i < WSIZE; i++) {
            l_tmp[i] = left_line_buf[i][col];
            r_tmp[i] = right_line_buf[i][col-offset < 0 ? 0 : col-offset];
          }
        }

        xFUpdateTextureSum<WSIZE,0,LWINWIDTH,WORDWIDTH_SRC>(l_window,l_tmp,row,col,state.preFilterCap,text_sum);

        xFShiftRight<unsigned char,WSIZE,LWINWIDTH>(l_window);
        xFShiftRight<unsigned char,WSIZE,RWINWIDTH>(r_window);
        xFInsertLeft<WSIZE,LWINWIDTH,unsigned char>(l_window,l_tmp);
        xFInsertLeft<WSIZE,RWINWIDTH,unsigned char>(r_window,r_tmp);

        loop_sad_compute:
        for (unsigned char d = 0; d < NDISP_UNIT; d++) {
          sad[d] += (int)xFSADComputeInc<WSIZE, LWINWIDTH, RWINWIDTH, unsigned char>(l_window, r_window, d, col, sad_cols[d]);
        }

        int skip_val[BUF_SIZE];
        if (USE_URAM){
#pragma HLS RESOURCE variable=skip_val core=RAM_S2P_URAM
        }
        int edge_neighbor[BUF_SIZE];
        if (USE_URAM){
#pragma HLS RESOURCE variable=edge_neighbor core=RAM_S2P_URAM
        }
        int edge[BUF_SIZE];
        if (USE_URAM){
#pragma HLS RESOURCE variable=edge core=RAM_S2P_URAM
        }
        int minsad_p[BUF_SIZE];
        if (USE_URAM){
#pragma HLS RESOURCE variable=minsad_p core=RAM_S2P_URAM
        }
        int minsad_n[BUF_SIZE];
        if (USE_URAM){
#pragma HLS RESOURCE variable=minsad_n core=RAM_S2P_URAM
        }

        // SAD computing and store output
        if (row >= WSIZE-1 && col >= WSIZE-1) {
          int skip_flag = 0;
          if (text_sum[0] < state.textureThreshold) skip_flag = 1; // texture threshold check
          if ((row - WSIZE+1) < (WSIZE-1)/2 || (row - WSIZE+1) >= height - (WSIZE-1)/2) skip_flag = 1;  // border skip horizontal
          if ((col - WSIZE+1) < NDISP-1 + (WSIZE-1)/2 || (col - WSIZE+1) >= width - (WSIZE-1)/2) skip_flag = 1; // border skip vertical

          int gminsad = TMP_INT_MAX_PACK;
          XF_TNAME(WORDWIDTH_DST,1) gmind = 0;
          bool gskip = 0;
          int gskip_val = TMP_INT_MAX_PACK;
          int gedge_neighbor = TMP_INT_MAX_PACK;  // for uniqueness check
          int gedge=0; // for subpixel interpolation
          if (NDISP_UNIT != 1)
            gedge = sad[1];

          int lminsad = TMP_INT_MAX_PACK;
          XF_TNAME(WORDWIDTH_DST,1) lmind = 0;
          int gminsad_p = TMP_INT_MAX_PACK;
          int gminsad_n = TMP_INT_MAX_PACK;

          if (sweep > 0) {
            gminsad = minsad[col];
            gmind   = mind[col];
            gskip = skip[col];
            gskip_val = skip_val[col];
            gedge_neighbor = edge_neighbor[col];
            if (sweep == 1 && NDISP_UNIT == 1)
              gedge_neighbor = TMP_INT_MAX_PACK;
            gedge = edge[col];
            gminsad_p = minsad_p[col];
            gminsad_n = (gmind == sweep*NDISP_UNIT-1 ? sad[0] : minsad_n[col]);
          }

          xFMinSAD<NDISP_UNIT>::find(sad, lmind, lminsad);

          if (lminsad <= gminsad) {
            gskip = 0;
            if (state.uniquenessRatio > 0) {
              int thresh = lminsad + (lminsad * state.uniquenessRatio / 100);
              if (gminsad <= thresh && lmind+sweep*NDISP_UNIT > gmind+1) {
                gskip = 1;
                gskip_val = gminsad;
              } else if (gminsad <= thresh && lmind+sweep*NDISP_UNIT == gmind+1 && gskip_val <= thresh) {
                gskip = 1;
                // gskip_val unchanged;
              } else if (gminsad <= thresh && lmind+sweep*NDISP_UNIT == gmind+1 && gedge_neighbor <= thresh) {
                gskip = 1;
                gskip_val = gedge_neighbor;
              }
              loop_unique_search_0:
              for (unsigned char d = 0; d < NDISP_UNIT; d++) {
                if (sad[d] <= thresh && sad[d] < gskip_val && (d < lmind-1 || d > lmind+1)) {
                  gskip = 1;
                  gskip_val = sad[d];
                }
              }
            }
            // update global values;
            gminsad_p = (lmind == 0 ? gedge : sad[lmind-1]);
            if (NDISP_UNIT == 1)
              gminsad_n = sad[lmind == NDISP_UNIT-1 ? 0 : (int)(lmind+1)];
            else
              gminsad_n = sad[lmind == NDISP_UNIT-1 ? lmind-1 : lmind+1];
            gminsad = lminsad;
            gmind = lmind + sweep*NDISP_UNIT;
          } else {
            if (state.uniquenessRatio > 0) {
              int thresh = gminsad + (gminsad * state.uniquenessRatio / 100);
              loop_unique_search_1:
              for (unsigned char d = 0; d < NDISP_UNIT; d++) {
                if (sad[d] <= thresh && sad[d] < gskip_val && ((gmind == (sweep*NDISP_UNIT-1)) ? ((sweep*NDISP_UNIT+d) > (gmind+1)) : 1)) {
                  gskip = 1;
                  gskip_val = sad[d];
                }
              }
            }
          }
          minsad[col] = gminsad;
          mind[col] = gmind;
          skip[col] = gskip;
          skip_val[col] = gskip_val;
          if (NDISP_UNIT == 1)
            edge_neighbor[col] = edge[col];
          else
            edge_neighbor[col] = sad[NDISP_UNIT-2];
          edge[col] = sad[NDISP_UNIT-1];
          minsad_p[col] = gminsad_p;
          minsad_n[col] = gminsad_n;

          if (sweep == state.sweepFactor-1) {
            ap_int<xFBitWidth<255*WSIZE*WSIZE>::Value> p = gmind==0?gminsad_n:gminsad_p;
            ap_int<xFBitWidth<255*WSIZE*WSIZE>::Value> n = gmind==NDISP-1?gminsad_p:gminsad_n;
            ap_int<xFBitWidth<255*WSIZE*WSIZE>::Value> k = p + n - 2*gminsad + __ABS((int)p - (int)n);

            ap_int<xFBitWidth<255*WSIZE*WSIZE>::Value+8> num = p - n;
            num = num << 8;
            ap_int<10> delta = 0;
            if (k != 0) delta = num/k;
            XF_TNAME(WORDWIDTH_DST,1) out_disp = ((gmind*256 + delta + 15) >> 4);

            skip_flag |= gskip;
            if (skip_flag) out_disp = FILTERED;
            out.write(out_disp);
          }
        }
      }
    }
  }
}


/* Support function for the Image Clip function */
template <int NPC>
void xFImageClipUtility(int i, int j, int k, int height, int width, int *pix)
{
#pragma HLS INLINE OFF
  if (i<1 || i > height-2 || (j*(1<<XF_BITSHIFT(NPC))+k < 1) || (j*(1<<XF_BITSHIFT(NPC))+k) > width-2)
    *pix = 0;
}


/* Clips the Output from the Sobel function based on the Cap value input */
template<int ROWS, int COLS, int NPC,int DEPTH_SRC, int DEPTH_DST, int SRC_T, int DST_T,int COLS_TC>
void xFImageClip(
    xf::Mat<SRC_T,ROWS,COLS,1>& src,
    hls::stream<XF_TNAME(DST_T,1)>& dst,
    int cap, short int height, short int width)
{
  loop_row_clip:
  for (short i = 0; i < height; i++)
  {
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off

    loop_col_clip:
    for (short j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
    {
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_TRIPCOUNT min=COLS_TC max=COLS_TC
      XF_TNAME(SRC_T,1) tmp = src.read(i*(width>>XF_BITSHIFT(NPC))+j);
      XF_TNAME(DST_T,1) tmp_out;
      for (int k = 0; k < (1<<XF_BITSHIFT(NPC)); k++)
      {
#pragma HLS UNROLL
        int pix = (XF_PTNAME(DEPTH_SRC))tmp.range((k+1)*XF_PIXELDEPTH(DEPTH_SRC)-1,k*XF_PIXELDEPTH(DEPTH_SRC));
        xFImageClipUtility<NPC>(i,j,k,height,width,&pix);

        XF_PTNAME(DEPTH_DST) p = (XF_PTNAME(DEPTH_DST))(pix < -cap ? 0 : pix > cap ? cap*2 : pix + cap);
        tmp_out.range((k+1)*XF_PIXELDEPTH(DEPTH_DST)-1,k*XF_PIXELDEPTH(DEPTH_DST)) = (XF_PTNAME(DEPTH_DST))p;
      }
      dst.write(tmp_out);
    }
  }
}


/* For reading the Gradient-Y stream, rather than letting the stream dangling */
template<int ROWS, int COLS, int NPC, int DEPTH_SRC, int DEPTH_DST, int SRC_T, int COLS_TC>
void xFReadOutStream(
    xf::Mat<SRC_T,ROWS,COLS,1>& src,
    short int height,short int width)
{
  loop_row_clip:
  for (short i = 0; i < height; i++)
  {
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off
    loop_col_clip:
    for (short j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
    {
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_TRIPCOUNT min=COLS_TC max=COLS_TC
      XF_TNAME(SRC_T,1) tmp = src.read(i*(width>>XF_BITSHIFT(NPC))+j);
    }
  }
}


/* Sobel gradient and Clipping */
template <int ROWS, int COLS, int SRC_T, int FILTER_T, int DST_T,bool USE_URAM>
void xFStereoPreProcess(xf::Mat<SRC_T, ROWS, COLS, 1> &in_mat, hls::stream<XF_TNAME(DST_T,1)>& clipped_strm, int preFilterType,int preFilterCap, short int height, short int width)
{
#pragma HLS INLINE
    xf::Mat<FILTER_T,ROWS,COLS,1> in_sobel_x(height,width);
#pragma HLS stream variable=in_sobel_x.data depth=2
    xf::Mat<FILTER_T,ROWS,COLS,1> in_sobel_y(height,width);
#pragma HLS stream variable=in_sobel_y.data depth=2
	
    xf::Sobel<XF_BORDER_CONSTANT, XF_FILTER_3X3,SRC_T,FILTER_T,ROWS,COLS,XF_NPPC1,USE_URAM>(in_mat,in_sobel_x ,in_sobel_y);
    xFImageClip<ROWS,COLS,XF_NPPC1,XF_16SP,XF_8UP,FILTER_T,DST_T,COLS>(in_sobel_x,clipped_strm,preFilterCap,height,width);
    xFReadOutStream<ROWS,COLS,XF_NPPC1,XF_16SP,XF_8UP,FILTER_T,COLS>(in_sobel_y,height,width);
}


/* This function performs preprocessing and disparity computation for NO mode */
template <int ROWS, int COLS, int SRC_T, int DST_T, int NPC, int WSIZE, int NDISP, int NDISP_UNIT, int SWEEP_FACT,bool USE_URAM>
void xFFindStereoCorrespondenceLBMNO_pipeline (hls::stream<XF_TNAME(SRC_T,NPC)> &_left_strm,
    hls::stream<XF_TNAME(SRC_T,NPC)> &_right_strm,
    XF_TNAME(DST_T,NPC) *disp_ptr ,
    xf::xFSBMState<WSIZE,NDISP,NDISP_UNIT> &sbmstate,
    short int height, short int width)
{
#pragma HLS INLINE

  xf::Mat<SRC_T,ROWS,COLS,NPC> _left_mat(height,width);
#pragma HLS stream variable=_left_mat.data depth=2
  xf::Mat<SRC_T,ROWS,COLS,NPC> _right_mat(height,width);
#pragma HLS stream variable=_right_mat.data depth=2

  hls::stream< XF_TNAME(SRC_T,NPC) > left_clipped("left_clipped");
  hls::stream< XF_TNAME(SRC_T,NPC) > right_clipped("right_clipped");

  hls::stream< XF_TNAME(DST_T,NPC) > _disp_strm("disparity stream");

#pragma HLS DATAFLOW

  int TC=(ROWS*COLS);
  for (int i = 0; i < height*width; i++)
  {
#pragma HLS pipeline ii=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=TC
    _left_mat.write(i,_left_strm.read());
    _right_mat.write(i,_right_strm.read());
  }

  /* Sobel and Clipping */
  xFStereoPreProcess<ROWS,COLS,SRC_T,XF_16SC1,SRC_T>(_left_mat,left_clipped,sbmstate.preFilterType,sbmstate.preFilterCap,height,width);
  xFStereoPreProcess<ROWS,COLS,SRC_T,XF_16SC1,SRC_T>(_right_mat,right_clipped,sbmstate.preFilterType,sbmstate.preFilterCap,height,width);

  /* SAD and disparity computation */
  xFSADBlockMatching<ROWS,COLS,SRC_T,DST_T,WSIZE,NDISP,NDISP_UNIT,SWEEP_FACT,ROWS+WSIZE-1,COLS+WSIZE-1,
  COLS+WSIZE-1,WSIZE,WSIZE+NDISP_UNIT-1,XF_16UW,XF_32UW,USE_URAM>(left_clipped,right_clipped,_disp_strm,sbmstate,height,width);

  for (int i = 0; i < height*width; i++)
  {
#pragma HLS pipeline ii=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=TC
    *(disp_ptr + i) = _disp_strm.read();
  }
}


/* This function performs preprocessing and disparity computation for NO mode */
template <int ROWS, int COLS, int SRC_T, int DST_T, int NPC, int WSIZE, int NDISP, int NDISP_UNIT, int SWEEP_FACT,bool USE_URAM>
void xFFindStereoCorrespondenceLBMNO (xf::Mat<SRC_T, ROWS, COLS, NPC> &_left_mat,
    xf::Mat<SRC_T, ROWS, COLS, NPC> &_right_mat,
    xf::Mat<DST_T, ROWS, COLS, NPC> &_disp_mat,
    xf::xFSBMState<WSIZE,NDISP,NDISP_UNIT> &sbmstate,
    short int height, short int width)
{
  hls::stream< XF_TNAME(SRC_T,NPC) > left_clipped("left_clipped");
  hls::stream< XF_TNAME(SRC_T,NPC) > right_clipped("right_clipped");

  hls::stream< XF_TNAME(DST_T,NPC) > _disp_strm("disparity stream");
#pragma HLS DATAFLOW

  int TC=(ROWS*COLS);

  /* Sobel and Clipping */
  xFStereoPreProcess<ROWS,COLS,SRC_T,XF_16SC1,SRC_T,USE_URAM>(_left_mat,left_clipped,sbmstate.preFilterType,sbmstate.preFilterCap,height,width);
  xFStereoPreProcess<ROWS,COLS,SRC_T,XF_16SC1,SRC_T,USE_URAM>(_right_mat,right_clipped,sbmstate.preFilterType,sbmstate.preFilterCap,height,width);

  /* SAD and disparity computation */
  xFSADBlockMatching<ROWS,COLS,SRC_T,DST_T,WSIZE,NDISP,NDISP_UNIT,SWEEP_FACT,ROWS+WSIZE-1,COLS+WSIZE-1,
  COLS+WSIZE-1,WSIZE,WSIZE+NDISP_UNIT-1,XF_16UW,XF_32UW,USE_URAM>(left_clipped,right_clipped,_disp_strm,sbmstate,height,width);

  for (int i = 0; i < height*width; i++)
  {
#pragma HLS pipeline ii=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=TC
    _disp_mat.write(i,_disp_strm.read());
  }
}


/* Calls the functions based on the PIXEL PARALLELISM configuration */
template<int ROWS, int COLS, int SRC_T, int DST_T, int NPC, int WSIZE, int NDISP, int NDISP_UNIT,bool USE_URAM>
void xFFindStereoCorrespondenceLBM_pipeline(hls::stream<XF_TNAME(SRC_T,NPC)> &_left_strm,
    hls::stream<XF_TNAME(SRC_T,NPC)> &_right_strm,
    XF_TNAME(DST_T,NPC) *out_ptr,
    xf::xFSBMState<WSIZE,NDISP,NDISP_UNIT> &sbmstate,
    short int height,short int width)
{
#pragma HLS INLINE

  assert((SRC_T == XF_8UC1) && " SRC_T must be XF_8UC1 ");
  assert((DST_T == XF_16UC1) && " DST_T must be XF_16UC1 ");
  assert((NPC == XF_NPPC1) && " NPC must be XF_NPPC1 ");
  assert((WSIZE%2 == 1) && (WSIZE < __XF_MIN(height,width) && (WSIZE >= 5)) && " WSIZE must be an odd number, less than minimum of height & width and greater than or equal to '5'  ");
  assert(((NDISP > 1) && (NDISP < width)) && " NDISP must be greater than '1' and less than the image width ");
  assert((NDISP >= NDISP_UNIT) && " NDISP must not be lesser than NDISP_UNIT");
  assert((((NDISP/NDISP_UNIT)*NDISP_UNIT) == NDISP) && " NDISP/NDISP_UNIT must be a non-fractional number ");
  assert(sbmstate.uniquenessRatio >= 0 && "uniqueness ratio must be non-negative");
  assert(sbmstate.preFilterCap >=1 && sbmstate.preFilterCap <= 63 && "preFilterCap must be within 1..63");
  assert(sbmstate.preFilterType == XF_STEREO_PREFILTER_SOBEL_TYPE);

  xFFindStereoCorrespondenceLBMNO_pipeline<ROWS,COLS,SRC_T,DST_T,NPC,WSIZE,NDISP,NDISP_UNIT,(NDISP/NDISP_UNIT)+((NDISP%NDISP_UNIT)!=0),USE_URAM>(_left_strm,_right_strm,out_ptr,sbmstate,height,width);
}

/* Calls the functions based on the PIXEL PARALLELISM configuration */
template<int ROWS, int COLS, int SRC_T, int DST_T, int NPC, int WSIZE, int NDISP, int NDISP_UNIT,bool USE_URAM>
void xFFindStereoCorrespondenceLBM(xf::Mat<SRC_T, ROWS, COLS, NPC> &_left_mat,
    xf::Mat<SRC_T, ROWS, COLS, NPC> &_right_mat,
    xf::Mat<DST_T, ROWS, COLS, NPC> &_disp_mat,
    xf::xFSBMState<WSIZE,NDISP,NDISP_UNIT> &sbmstate,
    short int height,short int width)
{
  assert((SRC_T == XF_8UC1) && " SRC_T must be XF_8UC1 ");
  assert((DST_T == XF_16UC1) && " DST_T must be XF_16UC1 ");
  assert((NPC == XF_NPPC1) && " NPC must be XF_NPPC1 ");
  assert((WSIZE%2 == 1) && (WSIZE < __XF_MIN(height,width) && (WSIZE >= 5)) && " WSIZE must be an odd number, less than minimum of height & width and greater than or equal to '5'  ");
  assert(((NDISP > 1) && (NDISP < width)) && " NDISP must be greater than '1' and less than the image width ");
  assert((NDISP >= NDISP_UNIT) && " NDISP must not be lesser than NDISP_UNIT");
  assert((((NDISP/NDISP_UNIT)*NDISP_UNIT) == NDISP) && " NDISP/NDISP_UNIT must be a non-fractional number ");
  assert(sbmstate.uniquenessRatio >= 0 && "uniqueness ratio must be non-negative");
  assert(sbmstate.preFilterCap >=1 && sbmstate.preFilterCap <= 63 && "preFilterCap must be within 1..63");
  assert(sbmstate.preFilterType == XF_STEREO_PREFILTER_SOBEL_TYPE);

  xFFindStereoCorrespondenceLBMNO<ROWS,COLS,SRC_T,DST_T,NPC,WSIZE,NDISP,NDISP_UNIT,(NDISP/NDISP_UNIT)+((NDISP%NDISP_UNIT)!=0),USE_URAM>(_left_mat,_right_mat,_disp_mat,sbmstate,height,width);
}


//#pragma SDS data data_mover("_left_mat.data":AXIDMA_SIMPLE,"_right_mat.data":AXIDMA_SIMPLE,"_disp_mat.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("_left_mat.data":SEQUENTIAL,"_right_mat.data":SEQUENTIAL,"_disp_mat.data":SEQUENTIAL)
//#pragma SDS data sys_port("_left_mat.data":ps_e_S_AXI_HP3_FPD)
//#pragma SDS data sys_port("_right_mat.data":ps_e_S_AXI_HP1_FPD)
#pragma SDS data copy("_left_mat.data"[0:"_left_mat.size"])
#pragma SDS data copy("_right_mat.data"[0:"_right_mat.size"])
#pragma SDS data copy("_disp_mat.data"[0:"_disp_mat.size"])

template <int WSIZE, int NDISP, int NDISP_UNIT, int SRC_T, int DST_T, int ROWS, int COLS, int NPC,bool USE_URAM=false>
void StereoBM(xf::Mat<SRC_T, ROWS, COLS, NPC> &_left_mat,
    xf::Mat<SRC_T, ROWS, COLS, NPC> &_right_mat,
    xf::Mat<DST_T, ROWS, COLS, NPC> &_disp_mat,
    xf::xFSBMState<WSIZE,NDISP,NDISP_UNIT> &sbmstate)
{
#pragma HLS INLINE OFF

  xFFindStereoCorrespondenceLBM<ROWS,COLS,SRC_T,DST_T,NPC,WSIZE,NDISP,NDISP_UNIT,USE_URAM>(_left_mat,_right_mat,_disp_mat,sbmstate,_left_mat.rows,_left_mat.cols);
}
}

#endif  // _XF_STEREOBM_HPP_

