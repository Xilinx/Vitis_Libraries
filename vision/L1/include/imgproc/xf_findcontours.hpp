/* Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*/

#include "ap_int.h"
#include "hls_stream.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"

namespace xf {
namespace cv {
// Output packing: y(15:0) | x(15:0)
static inline ap_uint<32> pack_xy(ap_uint<16> x, ap_uint<16> y) {
#pragma HLS INLINE
    return (ap_uint<32>)(((ap_uint<32>)y << 16) | (ap_uint<32>)x);
}

// Constants/flags from main_1.cpp logic
const ap_int<4> MASK8_RIGHT = (ap_int<4>)(-8); // 0x80; // 1000
const ap_int<4> MASK8_NEW = (ap_int<4>)(2);    // 0x02; //  0010
const ap_int<4> MASK8_FLAGS = (ap_int<4>)(-2); // 0xFE; // 1110
const ap_int<4> MASK8_BLACK = (ap_int<4>)(1);  // 0x01; //  0001

// Chain code 8-neighborhood deltas (dx,dy)
const int CC_DX[8] = {1, 1, 0, -1, -1, -1, 0, 1};
const int CC_DY[8] = {0, -1, -1, -1, 0, 1, 1, 1};

static inline int clamp_direction(int dir) {
#pragma HLS INLINE
    return (dir < 15) ? dir : 15;
}
static inline int getDelta(int s, int step) {
#pragma HLS INLINE
    s &= 7;
    return CC_DX[s] + CC_DY[s] * step;
}
static inline bool inBounds(int x, int y, int cols, int rows) {
#pragma HLS INLINE
    return (unsigned)x < (unsigned)cols && (unsigned)y < (unsigned)rows;
}

// Helpers that write flags into the working image (signed char buffer)
static inline void setRightFlag(ap_int<4>* elem, ap_int<4> nbd) {
#pragma HLS INLINE
    *elem = (ap_int<4>)(nbd | MASK8_RIGHT);
}
static inline void setNewFlag(ap_int<4>* elem, ap_int<4> nbd) {
#pragma HLS INLINE
    *elem = nbd;
}


static int findNextX_row(int x, int y, ap_int<4> prev, ap_int<4>& p, ap_int<8>* im, int cols, int new_cols) {
#pragma HLS INLINE off
    int width = cols - 1;
    if (x >= width) {
        p = 0;
        return width;
    }

    int base_idx = y * new_cols;
    int words_per_row = new_cols;
    int max_idx = base_idx + words_per_row;
    int widx = base_idx + (x >> 1);
    bool high_nibble = (x & 1);
    int curr_x = x;

    ap_int<8> word = im[widx];
    ap_int<8> next_word = 0;
    bool have_next = false;
    int next_idx = widx + 1;
    if (next_idx < max_idx) {
        next_word = im[next_idx];
        have_next = true;
    }

Scan:
    while (true) {
#pragma HLS PIPELINE II = 1
        ap_int<4> curr = high_nibble ? word.range(7, 4) : word.range(3, 0);
        p = curr;
        if (curr != prev) return curr_x;
        curr_x++;
        if (curr_x >= width) return width;

        high_nibble = !high_nibble;
        if (!high_nibble) {
            ++widx;
            if (have_next) {
                word = next_word;
            } else if (widx < max_idx) {
                word = im[widx];
            } else {
                return width;
            }
            int prefetch_idx = widx + 1;
            if (prefetch_idx < max_idx) {
                next_word = im[prefetch_idx];
                have_next = true;
            } else {
                have_next = false;
            }
        }
    }
}
//Helper for packed 2‑pixels/byte storage (low nibble = even x, high nibble = odd x)
static inline ap_int<4> pix_read(const ap_int<8>* base, int words_per_row, int x, int y) {
#pragma HLS INLINE
    int widx = y * words_per_row + (x >> 1);
    ap_int<8> w = base[widx];
    return (x & 1) ? (ap_int<4>)w.range(7, 4) : (ap_int<4>)w.range(3, 0);
}
static inline void pix_write(ap_int<8>* base, int words_per_row, int x, int y, ap_int<4> v) {
#pragma HLS INLINE
    int widx = y * words_per_row + (x >> 1);
    ap_int<8> w = base[widx];
    if (x & 1)
        w.range(7, 4) = v;
    else
        w.range(3, 0) = v;
    base[widx] = w;
}

static int fetchContour_single(ap_int<8>* im,
                               int rows,
                               int cols,
                               ap_int<4> nbd,
                               bool is_hole,
                               int seed_x,
                               int seed_y,
                               ap_uint<32>* out_pts,
                               int out_base,
                               int out_cap) {
#pragma HLS INLINE off
    if (out_cap <= 0) return 0;

    const int words_per_row = (cols + 1) >> 1;

    int start_x = seed_x - (is_hole ? 1 : 0);
    int start_y = seed_y;
    int cur_x = start_x - 1;
    int cur_y = start_y - 1;

    int s_end = is_hole ? 0 : 4;
    int s = s_end;

    // Find first non-zero neighbor around (start_x,start_y)
    int i1_x = 0, i1_y = 0;
    do {
        s = (s - 1) & 7;
        i1_x = start_x + CC_DX[s];
        i1_y = start_y + CC_DY[s];
    } while ((i1_x >= 0 && i1_x < cols && i1_y >= 0 && i1_y < rows && pix_read(im, words_per_row, i1_x, i1_y) == 0) &&
             s != s_end);

    int out_len = 0;

    if (s == s_end) {
        // single pixel
        ap_int<4> v = pix_read(im, words_per_row, start_x, start_y);
        v = (ap_int<4>)(nbd | MASK8_RIGHT);
        pix_write(im, words_per_row, start_x, start_y, v);
        out_pts[out_base] = pack_xy((ap_uint<16>)cur_x, (ap_uint<16>)cur_y);
        return 1;
    }

    int prev_s = s ^ 4;
    int i3_x = start_x, i3_y = start_y;

    while (true) {
#pragma HLS PIPELINE II = 1
        s_end = s;
        // search next non-zero
        int tmp_s = s;
        do {
            tmp_s = (tmp_s + 1);
            int cand = tmp_s & 7;
            int nx = i3_x + CC_DX[cand];
            int ny = i3_y + CC_DY[cand];
            if (nx >= 0 && nx < cols && ny >= 0 && ny < rows && pix_read(im, words_per_row, nx, ny) != 0) {
                s = cand;
                break;
            }
        } while (tmp_s < s + 8);

        // Flag update
        ap_int<4> v3 = pix_read(im, words_per_row, i3_x, i3_y);
        if ((unsigned)(s - 1) < (unsigned)s_end) {
            v3 = (ap_int<4>)(nbd | MASK8_RIGHT);
        } else if (v3 == MASK8_BLACK) {
            v3 = nbd;
        }
        pix_write(im, words_per_row, i3_x, i3_y, v3);

        // Emit vertex on direction change
        if (s != prev_s) {
            if (out_len < out_cap) {
                out_pts[out_base + out_len] = pack_xy((ap_uint<16>)cur_x, (ap_uint<16>)cur_y);
                out_len++;
            }
        }
        prev_s = s;

        // Advance along chain
        cur_x += CC_DX[s];
        cur_y += CC_DY[s];
        int i4_x = i3_x + CC_DX[s];
        int i4_y = i3_y + CC_DY[s];

        // Termination: back to start & first neighbor
        if (i4_x == start_x && i4_y == start_y && i3_x == i1_x && i3_y == i1_y) break;

        i3_x = i4_x;
        i3_y = i4_y;
        s = (s + 4) & 7;
    }
    return out_len;
}

// Contour scan at (x,y); updates outputs if a contour is found.
static bool contourScan_noVec(ap_int<4> prev,
                              ap_int<4>& p,
                              int& last_pos_x,
                              int x,
                              int y,
                              ap_int<8>* im,
                              int rows,
                              int cols,
                              int new_cols,
                              // outputs
                              ap_uint<32>* points_packed,
                              int& write_ptr,
                              int max_total_points,
                              ap_uint<32>* contour_offsets,
                              ap_uint<32>& num_contours,
                              ap_uint<32> max_contours,
                              ap_int<4>& nbd) {
#pragma HLS INLINE off
    bool is_hole = false;
    if (!(prev == 0 && p == 1)) {
        if (p != 0 || prev < 1) return false;
        if ((prev & MASK8_FLAGS) != 0) last_pos_x = x - 1;
        is_hole = true;
    }
    // if is_hole or last_pos is already > 0, skip
    ap_int<8> last_pos_in_data = (ap_int<8>)(im[y * new_cols + (last_pos_x >> 1)]);
    if ((last_pos_x & 1) == 0) {
        if (is_hole || (ap_int<4>)(last_pos_in_data.range(3, 0)) > 0) return false;
    } else {
        if (is_hole || (ap_int<4>)(last_pos_in_data.range(7, 4)) > 0) return false;
    }

    last_pos_x = x - (is_hole ? 1 : 0);
    ap_int<4> nbd_local = nbd;

    if (num_contours >= max_contours) return false;
    int remaining = max_total_points - write_ptr;
    if (remaining <= 0) return false;

    int len = fetchContour_single(im, rows, cols, nbd_local, is_hole, x, y, points_packed, write_ptr, remaining);
    if (len > 0) {
        write_ptr += len;
        contour_offsets[++num_contours] = (ap_uint<32>)write_ptr;
    }

    // Update scan state
    nbd = nbd_local;
    return len > 0;
}

// Top-level HLS kernel:
// img: binary image (0 => background, non-zero => foreground). Should include a 1-pixel zero border like main_1.cpp.
// points_packed: flat output of all contours, y(15:0)|x(15:0)
// contour_offsets: offsets[0]=0; offsets[c] = end index for contour c-1
template <int SRC_T,
          int INPUT_PTR_WIDTH,
          int OUTPUT_PTR_WIDTH,
          int MAX_H,
          int MAX_W,
          int MAX_TOTAL_POINTS,
          int MAX_CONTOURS,
          int NPPCX = 1,
          int XF_CV_DEPTH_IN_MAT = 2>
void findcontours(xf::cv::Mat<SRC_T, MAX_H, MAX_W, NPPCX, XF_CV_DEPTH_IN_MAT>& _src,
                  int rows,
                  int cols,
                  ap_uint<OUTPUT_PTR_WIDTH>* points_packed,
                  ap_uint<OUTPUT_PTR_WIDTH>* contour_offsets,
                  ap_uint<OUTPUT_PTR_WIDTH>& num_contours) {
    // Working image buffer (signed char) to store flags in-place
    ap_int<8> im[((MAX_H + 2) * (MAX_W + 2)) / 2];
#pragma HLS BIND_STORAGE variable = im type = ram_t2p impl = bram

    int rows_padded = rows + 2;
    int cols_padded = cols + 2;
    int packed_cols_pad = (cols_padded + 1) / 2;

    LoadImage:
    for (int i = 0; i < rows; ++i) {
        int dy = i + 1;
        int row_off = dy * packed_cols_pad;
        ap_int<4> even_bit = 0;
        bool even_valid = false;
    
        for (int j = 0; j < cols; ++j) {
    #pragma HLS PIPELINE II = 1
            ap_uint<8> vin = _src.read(i * cols + j);
            ap_int<4> bit = (vin != 0) ? (ap_int<4>)1 : (ap_int<4>)0;
            int dx = j + 1;
            int widx = row_off + (dx >> 1);
    
            if ((dx & 1) == 0) {
                even_bit = bit;
                even_valid = true;
                if (dx == cols) {
                    ap_int<8> word = 0;
                    word.range(3, 0) = even_bit;
                    im[widx] = word;
                    even_valid = false;
                }
            } else {
                ap_int<8> word = 0;
                word.range(3, 0) = even_valid ? even_bit : (ap_int<4>)0;
                word.range(7, 4) = bit;
                im[widx] = word;
                even_valid = false;
            }
        }
    }

    // Initialize scan state
    int lnbd_x = 0; // lnbd = Point(0,1), but we keep only x since row y is loop variable
    int pt_x = 1;   // pt = Point(1,1)
    int pt_y = 1;
    ap_int<4> nbd = ap_int<4>(2);

    // Outputs
    ap_uint<32> writePtr = 0;
    num_contours = 0;
    ap_uint<32> numC = 0;
    contour_offsets[0] = 0;

    int width = cols_padded - 1;
    int height = rows_padded - 1;
    int last_pos_x = lnbd_x; // lnbd = (0,y) in original; x set below after row end

    int i = pt_y;
RowScan:
    for (; i < height; ++i) {
        int last_pos_x = lnbd_x; // lnbd = (0,y) in original; x set below after row end
        int j = pt_x;
        ap_int<4> prev;
        prev = im[i * packed_cols_pad].range(3, 0);
        ap_int<4> p = 0;
    ColScan:
        for (; j < width;) {
#pragma HLS PIPELINE II = 1
            j = findNextX_row(j, i, prev, p, im, cols_padded, packed_cols_pad);

            if (j >= width) break;

            bool found = contourScan_noVec(prev, p, last_pos_x, j, i, im, rows_padded, cols_padded, packed_cols_pad,
                                           points_packed, (int&)writePtr, MAX_TOTAL_POINTS, contour_offsets, numC,
                                           (ap_uint<32>)MAX_CONTOURS, nbd);

            if (!found) {
                prev = p;
                if ((prev & MASK8_FLAGS) != 0) {
                    last_pos_x = j;
                }
                ++j; // ensure progress
            } else {
                // On found, lnbd = last_pos (x updated already)
                // We continue scanning; prev is unchanged; move to next x
                ++j;
            }
        }
        // Prepare next row
        lnbd_x = 0;
        pt_x = 1;
        prev = 0;
    }

// Fill remaining offsets with final writePtr (optional)
TailOffsets:
    for (ap_uint<32> c = numC + 1; c <= MAX_CONTOURS; ++c) {
#pragma HLS PIPELINE II = 1
        contour_offsets[c] = writePtr;
    }
    num_contours = (ap_uint<32>)numC;
}

} // namespace cv
} // namespace xf
