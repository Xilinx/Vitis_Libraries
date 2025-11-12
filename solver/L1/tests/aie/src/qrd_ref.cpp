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

#include "qrd_ref.hpp"
#include "qrd_ref_utils.hpp"

#include "fir_ref_utils.hpp"
// #include "solver_ref_utils.hpp"
#include "aie_api/aie_adf.hpp"
#include <adf.h>

//#define _SOLVERLIB_QRD_REF_DEBUG_

namespace xf {
namespace solver {
namespace aie {
namespace qrd {

// QRD - default/base 'specialization'
template <typename TT_DATA,
          size_t TP_DIM_ROWS,
          size_t TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN>
void qrd_ref<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN>::qrd_main(

    input_buffer<TT_DATA>& inWindowA,
    output_buffer<TT_DATA>& outWindowQ,
    output_buffer<TT_DATA>& outWindowR) {
    

    // unsigned int vcSize = CEIL(TP_DIM_ROWS, kSamplesInVectOutData); //vector size  

    using array_col = ::std::array<TT_DATA, TP_DIM_COLS>;
    using array_row = ::std::array<TT_DATA, TP_DIM_ROWS>;

    array_row original_vector; 
    array_row project_vector; 
    
    array_row a_mat [TP_DIM_COLS];
    array_row v_mat [TP_DIM_COLS];
    array_row q_mat [TP_DIM_COLS];
    array_col r_mat [TP_DIM_COLS];

    TT_DATA dot_product_val, q_val, r_val;
    TT_DATA dA_in;
    
    TT_DATA norm_val, dot_product;

    array_operations<TT_DATA, TT_DATA, TP_DIM_ROWS> arrOps_row; //vector_operations object
    array_operations<TT_DATA, TT_DATA, TP_DIM_COLS> arrOps_col; //vector_operations object

    TT_DATA* inPtrA  = (TT_DATA*)inWindowA.data();
    TT_DATA* outPtrQ = (TT_DATA*)outWindowQ.data();
    TT_DATA* outPtrR = (TT_DATA*)outWindowR.data();


    for (int frame=0; frame<TP_NUM_FRAMES; frame++) {
        for (unsigned int col=0; col<TP_DIM_COLS; col++){
            arrOps_col.all_zero_init(r_mat[col]);//initialize the r_matrix with zeros

            for (unsigned int row=0; row<TP_DIM_ROWS; row++) {
                dA_in = *inPtrA++;
                original_vector[row] = dA_in;
            }
            a_mat[col] = original_vector;
            v_mat[col] = original_vector;

            for (unsigned int col_pre=0; col_pre<col; col_pre++) {
                dot_product_val = arrOps_row.calc_dot_product(q_mat[col_pre], a_mat[col]); 
                r_mat[col][col_pre]  =  dot_product_val;
                project_vector = arrOps_row.mul_const_arr(r_mat[col][col_pre], q_mat[col_pre]);
                v_mat[col] = arrOps_row.sub_arrays(v_mat[col], project_vector);
            }

            norm_val = arrOps_row.calc_norm_val(v_mat[col]); //assign the norm val of the corresponding vector to the diagonal elements of R
            r_mat[col][col] = norm_val;
            q_mat[col] = arrOps_row.calc_normalized_vector(v_mat[col]);

            for (unsigned int row=0; row<TP_DIM_ROWS; row++) {
                q_val=q_mat[col][row];
                *outPtrQ++ = q_val;
                }            
            
            for (unsigned int col_vec=0; col_vec<TP_DIM_COLS; col_vec++) {
                r_val=r_mat[col][col_vec];
                *outPtrR++ = r_val;
                }    
            
        } 

    }
};


}
}
}
}
