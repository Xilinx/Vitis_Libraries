/*
 * Copyright 2022 Xilinx, Inc.
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

#ifndef _XF_ROTATE_HPP_
#define _XF_ROTATE_HPP_

#include "hls_stream.h"
#include "common/xf_common.hpp"
#include <iostream>

using namespace std;

namespace xf {
namespace cv {

template <int PXL_WIDTH, int TYPE, int NPC, int TILE_SZ, int NPC_TILESZ>
void ProcessTile(ap_uint<PXL_WIDTH> Src_Mat[TILE_SZ * NPC_TILESZ],
                 ap_uint<PXL_WIDTH> Dst_Mat[TILE_SZ * NPC_TILESZ],
                 uint16_t Rows,
                 uint16_t Cols,
                 int Row_diff,
                 int Col_diff,
                 int row_idx,
                 int col_idx,
                 int direction) {
// clang-format off
			#pragma HLS INLINE OFF
// clang-format on				
		
		int wr_ptr=0, rd_ptr=0;
		int n = 0, m = 1;
		//ap_uint<2> n;
		int Tile_rows, Tile_cols;
		
		const int XF_PWIDTH = XF_PIXELWIDTH(TYPE, NPC);
		
		Tile_rows = ((Rows - row_idx) == Row_diff) ? Row_diff : TILE_SZ;
		Tile_cols = ((Cols - col_idx) == Col_diff) ? Col_diff : TILE_SZ;
		
		for(int r = 0; r < Tile_rows; r++) {
// clang-format off
			#pragma HLS LOOP_FLATTEN off
			#pragma HLS LOOP_TRIPCOUNT min=TILE_SZ max=TILE_SZ
        // clang-format on
        if (NPC > 1) {
            if (direction == 0) n = n ^ 1;
            if (direction == 2) m = m ^ 1;
        } else
            m = 0;
        for (int c = 0; c < (Tile_cols / NPC); c++) {
// clang-format off
				#pragma HLS LOOP_TRIPCOUNT min=NPC_TILESZ max=NPC_TILESZ
				#pragma HLS PIPELINE
            // clang-format on

            for (int k = 0; k < NPC; k++) {
// clang-format off
					#pragma HLS UNROLL
				// clang-format on	
					
					if(direction == 0){
						
						Dst_Mat[(c*NPC + k) * Tile_rows/NPC + (Tile_rows-1-r)/NPC].range((n + 1) * XF_PWIDTH - 1, n * XF_PWIDTH) =
					
						Src_Mat[rd_ptr].range((k + 1) * XF_PWIDTH - 1, k * XF_PWIDTH);
					}
					else if(direction == 1){
						
						Dst_Mat[(Tile_rows - 1 - r) * Tile_cols/NPC + (Tile_cols)/NPC - c - 1].range((NPC - k) * XF_PWIDTH - 1, (NPC - k - 1) * XF_PWIDTH) =
					
						Src_Mat[rd_ptr].range((k + 1) * XF_PWIDTH - 1, k * XF_PWIDTH);
					}
					else{
						Dst_Mat[(Tile_cols-1-c*NPC-k) * Tile_rows/NPC + r/NPC].range((m + 1) * XF_PWIDTH - 1, m * XF_PWIDTH) =
					
						Src_Mat[rd_ptr].range((k + 1) * XF_PWIDTH - 1, k * XF_PWIDTH);
					}
				}
			rd_ptr++;
			}
		}
		
		return;
	}

	template <int INPUT_PTR_WIDTH, int PXL_WIDTH, int TYPE, int TILE_SZ, int NPC_TILESZ, int ROWS, int COLS, int NPC>
	void _Axi2Mat(ap_uint<INPUT_PTR_WIDTH>* src_ptr,
					ap_uint<PXL_WIDTH> Src_Mat[TILE_SZ*NPC_TILESZ],
					uint16_t Rows,
					uint16_t Cols,
					int Row_diff,
					int Col_diff,
					int row_idx,
					int col_idx) {
// clang-format off
			#pragma HLS INLINE OFF
// clang-format on				
			
		int Tile_rows, Tile_cols;
		
		uint16_t NPC_Cols = Cols >> XF_BITSHIFT(NPC);
		
		Tile_rows = ((Rows - row_idx) == Row_diff) ? Row_diff : TILE_SZ;
		Tile_cols = ((Cols - col_idx) == Col_diff) ? Col_diff : TILE_SZ;
		
		uint64_t OffsetSrc;
		OffsetSrc = ((row_idx*Cols + col_idx)* PXL_WIDTH/NPC + INPUT_PTR_WIDTH - 1)/INPUT_PTR_WIDTH;
		
		MMIterIn<INPUT_PTR_WIDTH, TYPE, TILE_SZ, COLS, NPC, -1>::Array2xfMat(src_ptr+OffsetSrc, Src_Mat, Tile_rows, Tile_cols, Cols);

		return;
	}

	template <int OUTPUT_PTR_WIDTH, int PXL_WIDTH, int TYPE, int TILE_SZ, int NPC_TILESZ, int ROWS, int COLS, int NPC>
	void _Mat2Axi(ap_uint<PXL_WIDTH> Dst_Mat[TILE_SZ*NPC_TILESZ],
						ap_uint<OUTPUT_PTR_WIDTH>*dst_ptr,
						uint16_t Rows,
						uint16_t Cols,
						int Row_diff,
						int Col_diff,
						int row_idx,
						int col_idx,
						int direction) {
// clang-format off
		#pragma HLS INLINE OFF
// clang-format on				

		int Tile_rows, Tile_cols, _cols, _rows, stride;
		
		uint16_t NPC_Cols = Cols >> XF_BITSHIFT(NPC);
		
		Tile_rows = ((Rows - row_idx) == Row_diff) ? Row_diff : TILE_SZ;
		Tile_cols = ((Cols - col_idx) == Col_diff) ? Col_diff : TILE_SZ;
		
		uint64_t OffsetDst;
		
		if(direction == 0) {
			OffsetDst = (((col_idx)* Rows + (Rows - Tile_rows - row_idx))*PXL_WIDTH/NPC + OUTPUT_PTR_WIDTH-1)/OUTPUT_PTR_WIDTH;
			_cols = Tile_rows;
			_rows = Tile_cols;
			stride = Rows;
		}
		else if(direction == 1) {
			OffsetDst = (((Rows - Tile_rows - row_idx) * Cols + (Cols - Tile_cols - col_idx))*PXL_WIDTH/NPC + OUTPUT_PTR_WIDTH-1)/OUTPUT_PTR_WIDTH;
			_cols = Tile_cols;
			_rows = Tile_rows;
			stride = Cols;
		}
		else {
			OffsetDst = (((Cols - Tile_cols - col_idx) * Rows + row_idx)*PXL_WIDTH/NPC + OUTPUT_PTR_WIDTH-1)/OUTPUT_PTR_WIDTH;
			_cols = Tile_rows;
			_rows = Tile_cols;
			stride = Rows;
		}
		MMIterOut<OUTPUT_PTR_WIDTH, TYPE, TILE_SZ, COLS, NPC, 0, -1>::xfMat2Array(Dst_Mat, dst_ptr+OffsetDst, _rows, _cols, stride);
		
		return;
	}

	template <int INPUT_PTR_WIDTH, int OUTPUT_PTR_WIDTH, int PXL_WIDTH, int TYPE, int TILE_SZ, int ROWS, int COLS, int NPC, int COLS_TC>
	void rotate_wrap(ap_uint<INPUT_PTR_WIDTH>* src_ptr,
						  ap_uint<OUTPUT_PTR_WIDTH>* dst_ptr,
						  int Rows,
						  int Cols,
						  int row_idx,
						  int Col_Tiles,
						  int direction) {
	// clang-format off
		#pragma HLS INLINE OFF
    // clang-format on

    uint16_t NPC_Cols = Cols >> XF_BITSHIFT(NPC);
    const uint16_t NPC_TILESZ = TILE_SZ >> XF_BITSHIFT(NPC);

    for (int c = 0; c < Col_Tiles; c++) {
// clang-format off
			#pragma HLS LOOP_TRIPCOUNT min=COLS_TC max=COLS_TC
			#pragma HLS DATAFLOW
        // clang-format on

        ap_uint<PXL_WIDTH> Src_Mat[TILE_SZ * NPC_TILESZ];
        ap_uint<PXL_WIDTH> Dst_Mat[TILE_SZ * NPC_TILESZ];

// clang-format off
#pragma HLS BIND_STORAGE variable = Src_Mat type=ram_s2p impl=bram
#pragma HLS BIND_STORAGE variable = Dst_Mat type=ram_s2p impl=bram
        // clang-format on

        int Row_diff = Rows - (Rows / TILE_SZ) * TILE_SZ;
        int Col_diff = Cols - (Cols / TILE_SZ) * TILE_SZ;

        _Axi2Mat<INPUT_PTR_WIDTH, PXL_WIDTH, TYPE, TILE_SZ, NPC_TILESZ, ROWS, COLS, NPC>(
            src_ptr, Src_Mat, Rows, Cols, Row_diff, Col_diff, row_idx, c * TILE_SZ);

        ProcessTile<PXL_WIDTH, TYPE, NPC, TILE_SZ, NPC_TILESZ>(Src_Mat, Dst_Mat, Rows, Cols, Row_diff, Col_diff,
                                                               row_idx, c * TILE_SZ, direction);

        _Mat2Axi<OUTPUT_PTR_WIDTH, PXL_WIDTH, TYPE, TILE_SZ, NPC_TILESZ, ROWS, COLS, NPC>(
            Dst_Mat, dst_ptr, Rows, Cols, Row_diff, Col_diff, row_idx, c * TILE_SZ, direction);
    }

    return;
}

template <int INPUT_PTR_WIDTH, int OUTPUT_PTR_WIDTH, int TYPE, int TILE_SZ, int ROWS, int COLS, int NPC>
void rotate(ap_uint<INPUT_PTR_WIDTH>* src_ptr, ap_uint<OUTPUT_PTR_WIDTH>* dst_ptr, int rows, int cols, int direction) {
// clang-format off
		#pragma HLS INLINE OFF
// clang-format on

#ifndef __SYNTHESIS__
    assert(((TYPE == XF_8UC1) || (TYPE == XF_8UC3)) &&
           "Input and Output TYPE must be XF_8UC1 for 1-channel, XF_8UC3 for 3-channel");
    assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC2)) && "NPC must be XF_NPPC1, XF_NPPC2 ");
    assert((rows <= ROWS) && (cols <= COLS) && "ROWS and COLS should be greater or equal to input image size ");
#endif

    const uint16_t PXL_WIDTH = XF_WORDDEPTH(XF_WORDWIDTH(TYPE, NPC));

    const uint16_t ROWS_TC = (ROWS + TILE_SZ - 1) / TILE_SZ;
    const uint16_t COLS_TC = (COLS + TILE_SZ - 1) / TILE_SZ;

    uint16_t Row_Tiles = (rows + TILE_SZ - 1) / TILE_SZ;
    uint16_t Col_Tiles = (cols + TILE_SZ - 1) / TILE_SZ;

    for (int r = 0; r < Row_Tiles; r++) {
// clang-format off
			//#pragma HLS LOOP_FLATTEN off
			#pragma HLS LOOP_TRIPCOUNT min=ROWS_TC max=ROWS_TC
        // clang-format on
        rotate_wrap<INPUT_PTR_WIDTH, OUTPUT_PTR_WIDTH, PXL_WIDTH, TYPE, TILE_SZ, ROWS, COLS, NPC, COLS_TC>(
            src_ptr, dst_ptr, rows, cols, r * TILE_SZ, Col_Tiles, direction);
    }

    return;
}
} // namespace cv
} // namespace xf
#endif //_XF_ROTATE_HPP_