/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
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
/*
QRD Product reference model
*/

#include "qrd_hh_ref.hpp"
#include "qrd_hh_ref_utils.hpp"

#include "fir_ref_utils.hpp"
#include "aie_api/aie_adf.hpp"
#include <adf.h>

//#define _SOLVERLIB_QRD_REF_DEBUG_

namespace xf {
namespace solver {
namespace aie {
namespace qrd_hh {

// QRD - default/base 'specialization'
template <typename TT_DATA,
          size_t TP_DIM_ROWS,
          size_t TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN>
void qrd_hh_ref<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN>::qrd_hh_main(

    input_buffer<TT_DATA>& inWindowA,
    output_buffer<TT_DATA>& outWindowQ,
    output_buffer<TT_DATA>& outWindowR) {

    using array_col = ::std::array<TT_DATA, TP_DIM_COLS>;
    using array_row = ::std::array<TT_DATA, TP_DIM_ROWS>;

    array_operations<TT_DATA, TT_DATA, TP_DIM_ROWS> arrOps_row; //vector_operations object

    array_row x_vect, r_fullsize, mid_prod_vect;
    array_col beta_vect;
    
    array_row q_mat [TP_DIM_COLS];
    array_row r_mat [TP_DIM_COLS];
    array_row v_col_mat [TP_DIM_COLS];

    TT_DATA norm_val_x_vect, alpha, phase, beta, mid_prod_val;
    TT_DATA beta_pp;
    TT_DATA dA_in, norm_val;

    TT_DATA* inPtrA  = (TT_DATA*)inWindowA.data();
    TT_DATA* outPtrQ = (TT_DATA*)outWindowQ.data();
    TT_DATA* outPtrR = (TT_DATA*)outWindowR.data();

    for (int frame=0; frame<TP_NUM_FRAMES; frame++) {
        // Initialize Q matrix as Identity matrix
       for (int row = 0; row < TP_DIM_ROWS; row++) {
            for (int col = 0; col < TP_DIM_COLS; col++) {
                if (col == row)
                    q_mat[col][row] = m_kONE;
                else
                    q_mat[col][row] = m_kZERO;
                }
        }// end of row and col loops to init Q matrix

        for (int col=0; col<TP_DIM_COLS; col++){
            for (int row=0; row<TP_DIM_ROWS; row++) {
                dA_in = *inPtrA++;
                r_mat[col][row]= dA_in; //get the input matrix column-major
            }
        } // end of col and row loops to read input matrix

        for (int col=0; col<TP_DIM_COLS; col++){
            arrOps_row.all_zero_init(x_vect);//initialize x_vect with zeros
            // Copy column 'col' from r_mat to x_vect
            for (int row = col; row < TP_DIM_ROWS; row++) {
                x_vect[row] = r_mat[col][row];
            }
            for (int row = 0; row < TP_DIM_ROWS; row++) {
                v_col_mat[col][row] = x_vect[row];
            }  

            // Compute v vector
            norm_val_x_vect = arrOps_row.calc_norm_val(x_vect);
            phase = calc_phase(x_vect[col]);  
            alpha = - phase * norm_val_x_vect;
            v_col_mat[col][col]= v_col_mat[col][col] - alpha;
            beta_pp = arrOps_row.calc_dot_product(v_col_mat[col], v_col_mat[col]);
            beta_vect[col] = divbyfloat(m_kTWO, beta_pp);

            for (int i=col; i<TP_DIM_COLS; i++){
                // To compute the dot product as v_col * r_mat[i]
                mid_prod_val = arrOps_row.calc_dot_product(v_col_mat[col], r_mat[i]);
                mid_prod_val = mid_prod_val * beta_vect[col];
                mid_prod_vect = arrOps_row.mul_const_arr(mid_prod_val, v_col_mat[col]);
                r_fullsize = arrOps_row.sub_arrays(r_mat[i], mid_prod_vect);
                for (int j = col; j<TP_DIM_ROWS; j++) {
                    r_mat[i][j] = r_fullsize[j]; //keep the upper part unchanged
                }
            }// end of i loop

            for (int row=0; row<TP_DIM_COLS; row++){ //only output the colxcol
                *outPtrR++ = r_mat[col][row];
            }

        } // end of col loop

        for (int col_rev=TP_DIM_COLS-1; col_rev >= 0; col_rev--){ 
            for (int col=0; col<TP_DIM_COLS; col++) {
                mid_prod_val = arrOps_row.calc_dot_product(v_col_mat[col_rev], q_mat[col]);
                mid_prod_vect = arrOps_row.mul_const_arr(mid_prod_val, v_col_mat[col_rev]);
                mid_prod_vect = arrOps_row.mul_const_arr(beta_vect[col_rev], mid_prod_vect);
                q_mat[col] = arrOps_row.sub_arrays(q_mat[col], mid_prod_vect);
            } // end of col loop
        }// end of col_rev loop

        for (int col=0; col<TP_DIM_COLS; col++) {
            for (int row=0; row<TP_DIM_ROWS; row++) {
                *outPtrQ++ = q_mat[col][row];
                }  
        }
    } // end of frame loop
}; // end of qrd_hh_main function


} //namespace qrd_hh
} //namespace aie
} //namespace solver
} //namespace xf
