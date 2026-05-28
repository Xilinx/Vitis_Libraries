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
qrd_hh kernel code.
This file captures the body of run-time code for the kernel class.

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#pragma once
#include <adf.h>
#include <cstring>

#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"
#include "kernel_api_utils.hpp"
#include "qrd_hh.hpp"
#include "qrd_hh_utils.hpp"

using namespace ::xf::dsp::aie;
//#define _SOLVERLIB_QRD_HH_HPP_DEBUG_

namespace xf {
namespace solver {
namespace aie {
namespace qrd_hh {

template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POS,
          bool TP_ROUT_EN,
          bool TP_STREAM_EN>
INLINE_DECL void
qrd_hh_kernel<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, TP_KERNEL_POS, TP_ROUT_EN, TP_STREAM_EN>::qrd_hh_r_func(
                        T_inputIF<TP_STREAM_EN, TT_DATA>& inInterface,
                        T_outputIF<TP_STREAM_EN, TP_ROUT_EN, TT_DATA>& outInterface) {

    using acc_t = accTypeMult_t<TT_DATA, TT_DATA>;
    using vect_t = ::aie::vector<TT_DATA, vecSampleNum>;
    using acc_vect_t = ::aie::accum<acc_t, vecSampleNum>;

    acc_vect_t acc_x_norm, acc_mid_prod;

    vect_t* inAPtr = (vect_t*)inInterface.inWindowA.data();
    vect_t* outRPtr = nullptr;
    if constexpr (TP_ROUT_EN) {
        outRPtr = (vect_t*)outInterface.outWindowR.data();
    }
    vect_t* outQPtr = (vect_t*)outInterface.outWindowQ.data();

    ::aie::mask<vecSampleNum> x_mask; 
    vect_t* vvectBuffPtr = (vect_t*)&vvectBuff[0];
    vect_t* betavectBuffPtr = (vect_t*)&betavectBuff[0];

    vect_t r_vect, v_vect, x_vect;
    vect_t mid_prod_vect;
    vect_t beta_vect;
    vect_t q_vect;
    vect_t blank_vect = ::aie::zeros<TT_DATA, vecSampleNum>();;


    TT_DATA phase, alpha;
    TT_DATA x_1, x_1_abs, x_1_abs_inv, x_acc_val, x_norm_val;
    TT_DATA x_col_sqr, v_col_sqr, v_acc_val, v_acc_val_inv;
    TT_DATA mid_prod_val;
    TT_DATA beta_val;

    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) chess_prepare_for_pipelining chess_loop_range(TP_NUM_FRAMES, ){
        unsigned int frame_offset_q = frame * m_kRowChunkNum * TP_DIM_COLS;
        outRPtr = (vect_t*)outInterface.outWindowR.data() + frame * m_kColChunkNum * TP_DIM_COLS;
        outQPtr = (vect_t*)outInterface.outWindowQ.data() + frame_offset_q;
        betavectBuffPtr = (vect_t*)&betavectBuff[0]; // point to the begining of the beta buffer for the current frame

        for (int col_chunk = 0; col_chunk < m_kColChunkNum; col_chunk++)chess_prepare_for_pipelining chess_loop_range(m_kColChunkNum, ){
            x_mask = ::aie::mask<vecSampleNum>::from_uint32(0x00000000);

            for (int col = 0; col < vecSampleNum; col++) chess_prepare_for_pipelining chess_loop_range(vecSampleNum, ){
                //offset calculation for the row chunk that will be processed
                unsigned int col_idx = col_chunk * vecSampleNum + col;
                unsigned int offset = frame_offset_q + col_idx * m_kRowChunkNum + col_chunk;

                inAPtr = (vect_t*)inInterface.inWindowA.data() + offset; //move to the current column, regarding row chunk
                vvectBuffPtr = (vect_t*)&vvectBuff[0] + col_chunk + 1;
            
                v_vect = ::aie::select(*inAPtr++, m_kZERO, x_mask); // x_vect is stored in the v_vect variable to prevent re-reading from memory -for the first chunk
                mul_vectors(acc_x_norm, v_vect, v_vect);//initial condition for acc_x_norm
                x_mask.set(col); //shift the mask down by 1 position and fill the first position with 0s 

                //alpha is to be calculated first chunk, first element-in-process 
                //get x_1 so following loop can start whilst scalar harware is calculating phase
                x_1 = v_vect[col]; 
                calc_phase(x_1, phase);
                //this scalar opertion is to help v_norm calculation without going through the whole vvector
                mul_scalars(x_1, x_1, x_col_sqr);

                for (int row_chunk = col_chunk+1; row_chunk < m_kRowChunkNum; row_chunk++){ //remainder of the row chunks             
                    x_vect = *inAPtr++;//reading current A vector
                    mac_vectors(acc_x_norm, x_vect, x_vect);
                    *vvectBuffPtr++ = x_vect; //save the xvector, vvector is the same for the rest of the chunks
                }

                x_acc_val = ::aie::reduce_add(acc_x_norm.template to_vector<TT_DATA>(0));
                calc_sqrt(x_acc_val, x_norm_val); // calculate the norm of x_vect 
                alpha = - phase * x_norm_val;
                v_vect[col] = x_1 - alpha; // calculating the first element of the x_vect, v_vect is actually generated here
                *(vvectBuffPtr - m_kRowChunkNum + col_chunk) = v_vect; // save the vvector back to the cache's first chunk address

                //beta calculation
                mul_scalars(v_vect.get(col),v_vect.get(col), v_col_sqr);
                v_acc_val = v_col_sqr - x_col_sqr; //which way is faster here? vector wise calculation?
                v_acc_val = x_acc_val + v_acc_val; // calculating the norm square of the v_vect

                if constexpr (std::is_same<TT_DATA, cfloat>::value) {
                    v_acc_val_inv.real = inv_float(v_acc_val.real);
                    beta_val.real = (2.0f)*v_acc_val_inv.real;
                    beta_val.imag = 0.0f;
                } else {
                    v_acc_val_inv = inv_float(v_acc_val);
                    beta_val = ::aie::mul(m_kTWO, v_acc_val_inv);
                }
                
                beta_vect[col] = beta_val;

                
                for (int i = col + col_chunk * vecSampleNum ; i<TP_DIM_COLS; i++)chess_prepare_for_pipelining chess_loop_range(0, TP_DIM_COLS){
                    //  To compute the dot product as v_col * r_mat[i]
                    vect_t* vvectPtrBase = (vect_t*) &vvectBuff[0] + col_chunk;
                    vect_t* inAPtrBase = (vect_t*) inInterface.inWindowA.data() + frame_offset_q + i * m_kRowChunkNum + col_chunk;//move to the current V column

                    vvectBuffPtr = vvectPtrBase;
                    inAPtr = inAPtrBase;
                
                    mul_vectors(acc_mid_prod, *vvectBuffPtr++, *inAPtr++);//initial condition for acc_mid_prod
                    for (int row_chunk = col_chunk+1; row_chunk < m_kRowChunkNum; row_chunk++){ 
                        mac_vectors(acc_mid_prod, *vvectBuffPtr++, *inAPtr++);  
                    }
                    mid_prod_val = ::aie::reduce_add(acc_mid_prod.template to_vector<TT_DATA>(0));
                    mid_prod_val = mid_prod_val * beta_val;

                    // Now update the r_mat
                    vvectBuffPtr = vvectPtrBase;
                    inAPtr = inAPtrBase;  
                    for (int row_chunk = col_chunk; row_chunk < m_kRowChunkNum; row_chunk++){ 
                        mid_prod_vect = ::aie::mul(mid_prod_val, *vvectBuffPtr++);   
                        r_vect = ::aie::sub(*inAPtr, mid_prod_vect); 
                        *inAPtr++ = r_vect; // save the updated r_vect back to the input buffer  

                    }
                }
                //update R
                // ensure reflection writes to A complete before R read
                inAPtr = (vect_t*)inInterface.inWindowA.data() + frame_offset_q + col_idx * m_kRowChunkNum; // transfer the R values for the current column
                for (int i = 0; i < m_kColChunkNum; i++){
                    *outRPtr++ = *inAPtr++;
                }
                chess_memory_fence(); // ensure R read completes before V-save overwrites A

                //save V vector to the completed input vector buffer, also set the initial value for the output Q buffer for the current column
                inAPtr = (vect_t*)inInterface.inWindowA.data() + frame_offset_q + col_idx * m_kRowChunkNum ; //move to the current column, regarding row chunk
                vvectBuffPtr = (vect_t*) &vvectBuff[0] + col_chunk;

                for (int row = 0; row < col_chunk; row++){
                    *inAPtr++ = blank_vect; 
                    *outQPtr++ = blank_vect;
                }
                //col in process
                q_vect = blank_vect;
                q_vect[col] = m_kONE;
                *outQPtr++ = q_vect; //filling the current col, current chunk
                *inAPtr++ = *vvectBuffPtr++;

                for (int row_chunk = col_chunk+1; row_chunk < m_kRowChunkNum; row_chunk++){ //save the vvector to the output Q buffer (the necessary chunks)
                    *inAPtr++ = *vvectBuffPtr++;
                    *outQPtr++ = blank_vect;
                }

            } // end of col
            *betavectBuffPtr++ = beta_vect;
        }// end of col_chunk
        
        this->qrd_hh_q_func(inInterface, outInterface, frame); // call the qrd_hh_q function to start the reverse sweep for Q generation
    }
}; // end of qrd_hh function

template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POS,
          bool TP_ROUT_EN,
          bool TP_STREAM_EN>
INLINE_DECL void
qrd_hh_kernel<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, TP_KERNEL_POS, TP_ROUT_EN, TP_STREAM_EN>::qrd_hh_q_func(
                        T_inputIF<TP_STREAM_EN, TT_DATA>& inInterface,
                        T_outputIF<TP_STREAM_EN, TP_ROUT_EN, TT_DATA>& outInterface,
                        int& frame_id
                    ) {

    using acc_t = accTypeMult_t<TT_DATA, TT_DATA>;
    using vect_t = ::aie::vector<TT_DATA, vecSampleNum>;
    using acc_vect_t = ::aie::accum<acc_t, vecSampleNum>;

    vect_t v_vect;
    vect_t q_vect, mid_prod_vect;
    vect_t blank_vect = ::aie::zeros<TT_DATA, vecSampleNum>();;
    vect_t beta_vect;

    vect_t* betaBuffPtr;
    vect_t* outQPtr = (vect_t*) outInterface.outWindowQ.data();
    vect_t* betavectBuffPtr = (vect_t*)&betavectBuff[0] + m_kColChunkNum - 1; // point to the last chunk of beta vector for the reverse sweep 
    vect_t* inAPtr = (vect_t*)inInterface.inWindowA.data();

    acc_vect_t acc_mid_prod;
    TT_DATA beta_val;
    TT_DATA* betaScalarPtr;
    TT_DATA mid_prod_val;

    unsigned int frame_offset_q = frame_id * m_kRowChunkNum * TP_DIM_COLS;    
    for (int col_chunk = m_kColChunkNum - 1 ; col_chunk >= 0; col_chunk--) chess_prepare_for_pipelining chess_loop_range(m_kColChunkNum, ){//reverse sweep for v capture
        beta_vect = *betavectBuffPtr--; //get the beta vector for the current chunk
        for (int col = vecSampleNum - 1 ; col >= 0; col--) chess_prepare_for_pipelining chess_loop_range(vecSampleNum, ){
            //offset calculation for the row chunk that will be processed
            unsigned int col_idx = col_chunk * vecSampleNum + col;
            unsigned int offset = frame_offset_q + col_idx * m_kRowChunkNum + col_chunk;
            beta_val = beta_vect[col]; //get the beta value

            for (int i = 0 ; i < TP_DIM_COLS; i++) chess_prepare_for_pipelining chess_loop_range(TP_DIM_COLS, ){ //forward sweep for Q accumulation
                vect_t* QPtrBase =(vect_t*)outInterface.outWindowQ.data() + frame_offset_q + i*m_kRowChunkNum + col_chunk; // set to the begining of the current vector
                vect_t* inAPtrBase = (vect_t*)inInterface.inWindowA.data() + offset;//move to the current V column

                outQPtr = QPtrBase;
                inAPtr = inAPtrBase;

                mul_vectors(acc_mid_prod, *inAPtr++, *outQPtr++);//initial condition for acc_x_norm
                for (int row_chunk = col_chunk+1; row_chunk < m_kRowChunkNum; row_chunk++){ 
                    mac_vectors(acc_mid_prod, *inAPtr++, *outQPtr++);    
                }
                mid_prod_val = ::aie::reduce_add(acc_mid_prod.template to_vector<TT_DATA>(0));
                mid_prod_val = mid_prod_val * beta_val;

                outQPtr = QPtrBase;
                inAPtr = inAPtrBase;

                for (int row_chunk = col_chunk; row_chunk < m_kRowChunkNum; row_chunk++){
                    mid_prod_vect = ::aie::mul(mid_prod_val, *inAPtr++);   
                    q_vect = ::aie::sub(*outQPtr, mid_prod_vect); 
                    *outQPtr++ = q_vect; // save the updated q_vect back to the input buffer
                }
            } // end of i loop
        } // end of col loop
    } // end of col_chunk loop

}; // end of qrd_hh_q function

template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POS,
          bool TP_ROUT_EN,
          bool TP_STREAM_EN>
INLINE_DECL void
qrd_hh_kernel<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, TP_KERNEL_POS, TP_ROUT_EN, TP_STREAM_EN>::qrd_hh_r_casc_func(
                        T_inputIF<TP_STREAM_EN, TT_DATA>& inInterface,
                        T_outputIF<TP_STREAM_EN, TP_ROUT_EN, TT_DATA>& outInterface
        ) {

    using acc_t = accTypeMult_t<TT_DATA, TT_DATA>;
    using vect_t = ::aie::vector<TT_DATA, vecSampleNum>;
    using acc_vect_t = ::aie::accum<acc_t, vecSampleNum>;

    acc_vect_t acc_x_norm, acc_mid_prod;

    vect_t* inAPtr = (vect_t*)inInterface.inWindowA.data();
    vect_t* outRPtr = nullptr;
    if constexpr (TP_ROUT_EN) {
        outRPtr = (vect_t*)outInterface.outWindowR.data();
    }
    vect_t* outQPtr = (vect_t*)outInterface.outWindowQ.data();

    ::aie::mask<vecSampleNum> x_mask; 
    vect_t* vvectBuffPtr = (vect_t*)&vvectBuff[0];
    vect_t* betavectBuffPtr = (vect_t*)&betavectBuff[0];

    vect_t r_vect, v_vect, x_vect;
    vect_t mid_prod_vect;
    vect_t beta_vect;
    vect_t q_vect;
    vect_t blank_vect = ::aie::zeros<TT_DATA, vecSampleNum>();;

    TT_DATA phase, alpha;
    TT_DATA x_1, x_1_abs, x_1_abs_inv, x_acc_val, x_acc_val_stream, x_norm_val;
    TT_DATA x_col_sqr, v_col_sqr, v_acc_val, v_acc_val_inv;
    TT_DATA mid_prod_val, mid_prod_val_pp, mid_prod_val_stream;
    TT_DATA beta_val;
    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) chess_prepare_for_pipelining chess_loop_range(TP_NUM_FRAMES, ) {
        unsigned int frame_offset_q = frame * m_kRowChunkNum * TP_DIM_COLS;
        unsigned int frame_offset_r = frame * m_kRcolChunk * TP_DIM_COLS;
        if constexpr (TP_ROUT_EN) {
        outRPtr = (vect_t*)outInterface.outWindowR.data() + frame_offset_r;
        }

        betavectBuffPtr = (vect_t*)&betavectBuff[0]; // point to the begining of the beta buffer for the current frame
        
        for (int col_chunk = 0; col_chunk < m_kColChunkNum; col_chunk++){
            
            int col_chunk_in_calc;
            int kernel_role; // 0 not leading, 1 next leading, 2 leading, 3 idle

            col_chunk_in_calc = m_kKernelRolesR.col_chunk_in_calc_arr[col_chunk];
            kernel_role = m_kKernelRolesR.kernel_role_arr[col_chunk];

            if( kernel_role < 3) {
                x_mask = ::aie::mask<vecSampleNum>::from_uint32(0x00000000);
            }

            for (int col = 0; col < vecSampleNum; col++) chess_prepare_for_pipelining chess_loop_range(vecSampleNum, ) {
                //offset calculation for the row chunk that will be processed
                unsigned int col_idx = col_chunk * vecSampleNum + col;
                unsigned int offset = frame_offset_q + col_idx * m_kRowChunkNum + col_chunk_in_calc;
                if (kernel_role < 3){ // kernel is not idle
                    inAPtr = (vect_t*)inInterface.inWindowA.data() + offset; //move to the current column, regarding row chunk            
                    vvectBuffPtr = (vect_t*)&vvectBuff[0] + col_chunk_in_calc;
                    
                    x_vect = *inAPtr++;//reading current A vector
                    v_vect = ::aie::select(x_vect, m_kZERO, x_mask); // x_vect is stored in the v_vect variable to prevent re-reading from memory -for the first chunk
                    *vvectBuffPtr++ = x_vect;
                }

                if (kernel_role == 1) { //leading kernel, needs to calculate the alpha, phase and the first element of v_vect
                    x_mask.set(col); //shift the mask down by 1 position and fill the first position with 0s 
                    //alpha is to be calculated first chunk, first element-in-process 
                    //get x_1 so following loop can start whilst scalar harware is calculating phase
                    x_1 = v_vect[col]; 
                    calc_phase(x_1, phase);
                    //this scalar opertion is to help v_norm calculation without going through the whole vvector
                    mul_scalars(x_1, x_1, x_col_sqr);
                }

                if (kernel_role < 3){

                    mul_vectors(acc_x_norm, v_vect, v_vect);//initial condition for acc_x_norm
                    for (int row_chunk = col_chunk_in_calc+1; row_chunk < m_kRowChunkNum; row_chunk++){ //remainder of the row chunks             
                        x_vect = *inAPtr++;//reading current A vector
                        mac_vectors(acc_x_norm, x_vect, x_vect);
                        *vvectBuffPtr++ = x_vect; //save the xvector, vvector is the same for the rest of the chunks
                    }
                    x_acc_val = ::aie::reduce_add(acc_x_norm.template to_vector<TT_DATA>(0));

                    if ((TP_KERNEL_POS != TP_CASC_LEN - 1) &&  (kernel_role < 3)) { // if not the last kernel read the val and update your calc
                        chess_separator_scheduler();            
                        chess_memory_fence();   
                        x_acc_val_stream = readincr<aie_stream_resource_in::a>(inInterface.inStream);
                        x_acc_val = x_acc_val + x_acc_val_stream; // accumulate the x_acc_val from different kernels
                    }

                    if (kernel_role !=1) { // if not the leading kernel, need to send the x_acc_val to the next kernel through the cascade stream
                        writeincr<aie_stream_resource_out::a, TT_DATA>(outInterface.outStream, x_acc_val); //send the xacc
                    }
                }
            
                if (kernel_role == 1) {
                    calc_sqrt(x_acc_val, x_norm_val); // calculate the norm of x_vect  
                    alpha = - phase * x_norm_val;
                    v_vect[col] = x_1 - alpha; // calculating the first element of the x_vect, v_vect is actually generated here 
                    *(vvectBuffPtr - m_kRowChunkNum + col_chunk_in_calc) = v_vect; // save the vvector back to the cache's first chunk address

                    //beta calculation
                    mul_scalars(v_vect.get(col),v_vect.get(col), v_col_sqr);
                    v_acc_val = v_col_sqr - x_col_sqr; //which way is faster here? vector wise calculation?
                    v_acc_val = x_acc_val + v_acc_val; // calculating the norm square of the v_vect
                    
                    if constexpr (std::is_same<TT_DATA, cfloat>::value) {
                        v_acc_val_inv.real = inv_float(v_acc_val.real);
                        beta_val.real = (2.0f)*v_acc_val_inv.real;
                        beta_val.imag = 0.0f;
                    } else {
                        v_acc_val_inv = inv_float(v_acc_val);
                        beta_val = ::aie::mul(m_kTWO, v_acc_val_inv);
                    }

                    writeincr<aie_stream_resource_out::a, TT_DATA>(outInterface.outStream, beta_val); //send the beta
                } 
                else{
                    chess_separator_scheduler();            
                    chess_memory_fence();   
                    beta_val = readincr<aie_stream_resource_in::a>(inInterface.inStream); //if not produced wait for the beta value
                    if ((kernel_role != 2)  && ((kernel_role != 3)  || (TP_KERNEL_POS !=0) )){ //stop transfer
                        writeincr<aie_stream_resource_out::a, TT_DATA>(outInterface.outStream, beta_val); //send the beta
                    }
                }
                beta_vect[col] = beta_val;
                for (int i = col + col_chunk * vecSampleNum ; i<TP_DIM_COLS; i++) chess_prepare_for_pipelining chess_loop_range(0, TP_DIM_COLS) {
                    //  To compute the dot product as v_col * r_mat[i]
                    vect_t* vvectPtrBase = (vect_t*) &vvectBuff[0] + col_chunk_in_calc;
                    vect_t* inAPtrBase = (vect_t*) inInterface.inWindowA.data() + frame_offset_q + i * m_kRowChunkNum + col_chunk_in_calc;

                    vvectBuffPtr = vvectPtrBase;
                    inAPtr = inAPtrBase;

                    if (kernel_role < 3){ // if not idle, calculate the mid_prod_val and update the r_mat
                        mul_vectors(acc_mid_prod, *vvectBuffPtr++, *inAPtr++);//initial condition for acc_x_norm                    
                        for (int row_chunk = col_chunk_in_calc+1; row_chunk < m_kRowChunkNum; row_chunk++){ 
                            mac_vectors(acc_mid_prod, *vvectBuffPtr++, *inAPtr++);  
                        }
                        mid_prod_val = ::aie::reduce_add(acc_mid_prod.template to_vector<TT_DATA>(0));        

                        if (TP_KERNEL_POS != TP_CASC_LEN - 1) { // if not the last kernel, need to accumulate the x_acc_val from the previous kernels through the cascade stream
                            chess_separator_scheduler();            
                            chess_memory_fence();   
                            mid_prod_val_stream = readincr<aie_stream_resource_in::a>(inInterface.inStream);
                            mid_prod_val = mid_prod_val + mid_prod_val_stream; // accumulate the x_acc_val from different kernels
                        }

                        if (kernel_role != 1) { //if it is down the leading kernel
                            writeincr<aie_stream_resource_out::a, TT_DATA>(outInterface.outStream, mid_prod_val); //send the mid_prod_val
                        } 
                    }

                    if (kernel_role == 1) { //leading kernel calculates the pp
                        mid_prod_val = mid_prod_val * beta_val;
                    }else{
                        chess_separator_scheduler();            
                        chess_memory_fence();        
                        mid_prod_val = readincr<aie_stream_resource_in::a>(inInterface.inStream); //if not produced wait for the mid_prod_val
                    }

                    if ((kernel_role != 2)  && ((kernel_role != 3)  || (TP_KERNEL_POS !=0) )){ //stop transfer
                        writeincr<aie_stream_resource_out::a, TT_DATA>(outInterface.outStream, mid_prod_val);
                    }

                    if (kernel_role < 3){ // if not idle
                        // Now update the r_mat
                        vvectBuffPtr = vvectPtrBase;
                        inAPtr = inAPtrBase;

                        for (int row_chunk = col_chunk_in_calc; row_chunk < m_kRowChunkNum; row_chunk++){ 
                            mid_prod_vect = ::aie::mul(mid_prod_val, *vvectBuffPtr++);   
                            r_vect = ::aie::sub(*inAPtr, mid_prod_vect); 
                            *inAPtr++ = r_vect; // save the updated r_vect back to the input buffer
                        }
                    }
                }

                //update R
                if constexpr (TP_ROUT_EN){
                    inAPtr = (vect_t*)inInterface.inWindowA.data() + frame_offset_q + col_idx * m_kRowChunkNum; // transfer the R values for the current column
                    for (int i = 0; i < m_kRcolChunk; i++){
                        *outRPtr++ = *inAPtr++;
                    }
                }
                if (kernel_role < 3){ //idle kernel, just pass the vvector
                    //save V vector to the completed input vector buffer, also set the initial value for the output Q buffer for the current column
                    inAPtr = (vect_t*)inInterface.inWindowA.data() + frame_offset_q + col_idx * m_kRowChunkNum ; //move to the current column, regarding row chunk
                    vvectBuffPtr = (vect_t*) &vvectBuff[0] + col_chunk_in_calc;
                    for (int row = 0; row < col_chunk_in_calc; row++){
                        *inAPtr++ = blank_vect; 
                        }
                    *inAPtr++ = *vvectBuffPtr++;

                    for (int row_chunk = col_chunk_in_calc+1; row_chunk < m_kRowChunkNum; row_chunk++){ //save the vvector to the output Q buffer (the necessary chunks)
                        *inAPtr++ = *vvectBuffPtr++;
                    }
                } 

                outQPtr = (vect_t*)outInterface.outWindowQ.data() + frame_offset_q + col_idx * m_kRowChunkNum; // move to the current column for Q output
                for (int row = 0; row < col_chunk_in_calc; row++){
                    *outQPtr++ = blank_vect;
                    }
                //col in process — skipped for idle kernels (col_chunk_in_calc == m_kRowChunkNum)
                if (col_chunk_in_calc < m_kRowChunkNum) {
                    q_vect = blank_vect;
                    if (kernel_role == 1){ //leading kernel is setting diagonal elements
                        q_vect[col] = m_kONE;}
                    *outQPtr++ = q_vect; //filling the current col, current chunk
                    for (int row_chunk = col_chunk_in_calc+1; row_chunk < m_kRowChunkNum; row_chunk++){ //save the vvector to the output Q buffer (the necessary chunks)
                        *outQPtr++ = blank_vect;
                    }
                }
                
            } // end of col
            *betavectBuffPtr++ = beta_vect;
        } // end of col_chunk

        qrd_hh_q_casc_func(inInterface, outInterface, frame); // call the qrd_hh_q function to start the reverse sweep for Q generation
    }
    };


template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POS,
          bool TP_ROUT_EN,
          bool TP_STREAM_EN>
INLINE_DECL void
qrd_hh_kernel<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, TP_KERNEL_POS, TP_ROUT_EN, TP_STREAM_EN>::qrd_hh_q_casc_func(
                        T_inputIF<TP_STREAM_EN, TT_DATA>& inInterface,
                        T_outputIF<TP_STREAM_EN, TP_ROUT_EN, TT_DATA>& outInterface,
                        int& frame_id) 
                        {

    using acc_t = accTypeMult_t<TT_DATA, TT_DATA>;
    using vect_t = ::aie::vector<TT_DATA, vecSampleNum>;
    using acc_vect_t = ::aie::accum<acc_t, vecSampleNum>;

    vect_t v_vect;
    vect_t q_vect, mid_prod_vect;
    vect_t blank_vect = ::aie::zeros<TT_DATA, vecSampleNum>();;
    vect_t beta_vect;

    vect_t* betaBuffPtr;
    vect_t* outQPtr = (vect_t*) outInterface.outWindowQ.data();
    vect_t* betavectBuffPtr = (vect_t*)&betavectBuff[0] + m_kColChunkNum - 1; // point to the last chunk of beta vector for the reverse sweep 
    vect_t* inAPtr = (vect_t*)inInterface.inWindowA.data();

    acc_vect_t acc_mid_prod;
    TT_DATA beta_val;
    TT_DATA* betaScalarPtr;
    TT_DATA mid_prod_val, mid_prod_val_stream;

    unsigned int frame_offset_q = frame_id * m_kRowChunkNum * TP_DIM_COLS;
    for (int col_chunk = m_kColChunkNum - 1 ; col_chunk >= 0; col_chunk--) chess_prepare_for_pipelining chess_loop_range(m_kColChunkNum, ){//reverse sweep for v capture
        int col_chunk_in_calc = m_kKernelRolesQ.col_chunk_in_calc_arr[col_chunk];
        int kernel_role = m_kKernelRolesQ.kernel_role_arr[col_chunk];; // 0 not leading, 1 next leading, 2 leading, 3 idle
        beta_vect = *betavectBuffPtr--; //get the beta vector for the current chunk
        for (int col = vecSampleNum - 1 ; col >= 0; col--)  chess_prepare_for_pipelining chess_loop_range(vecSampleNum, ) {
            //offset calculation for the row chunk that will be processed
            unsigned int col_idx = col_chunk * vecSampleNum + col;
            unsigned int offset = frame_offset_q + col_idx * m_kRowChunkNum + col_chunk_in_calc;
            beta_val = beta_vect[col]; //get the beta value

            for (int i = 0 ; i < TP_DIM_COLS; i++){ //forward sweep for Q accumulation
                vect_t* QPtrBase = (vect_t*)outInterface.outWindowQ.data() + frame_offset_q + i*m_kRowChunkNum + col_chunk_in_calc;
                vect_t* inAPtrBase = (vect_t*)inInterface.inWindowA.data() + offset;//move to the current V column    

                outQPtr = QPtrBase;
                inAPtr = inAPtrBase;    
                if (kernel_role < 3){ // kernel is not idle, needs to calculate the mid_prod_val and update the q_mat

                    mul_vectors(acc_mid_prod, *inAPtr++, *outQPtr++);//initial condition for acc_x_norm
                    for (int row_chunk = col_chunk_in_calc+1; row_chunk < m_kRowChunkNum; row_chunk++){ 
                        mac_vectors(acc_mid_prod, *inAPtr++, *outQPtr++);    
                    }      
                    mid_prod_val = ::aie::reduce_add(acc_mid_prod.template to_vector<TT_DATA>(0));    

                    if (TP_KERNEL_POS != TP_CASC_LEN-1) {
                        chess_separator_scheduler();            
                        chess_memory_fence();   
                        mid_prod_val_stream = readincr<aie_stream_resource_in::a>(inInterface.inStream);
                        mid_prod_val = mid_prod_val + mid_prod_val_stream;
                    }
                    if (kernel_role != 1) { //if it is down the leading kernel
                        writeincr<aie_stream_resource_out::a, TT_DATA>(outInterface.outStream, mid_prod_val); //send the mid_prod_val
                    }
                }// if not idle

                if (kernel_role == 1) { //leading kernel calculates the pp
                    mid_prod_val = mid_prod_val * beta_val;
                } else{
                    chess_separator_scheduler();        
                    chess_memory_fence();            
                    mid_prod_val = readincr<aie_stream_resource_in::a>(inInterface.inStream);
                }

                if ((kernel_role != 2)  && ((kernel_role != 3)  || (TP_KERNEL_POS !=0) )){ //stop transfer
                    writeincr<aie_stream_resource_out::a, TT_DATA>(outInterface.outStream, mid_prod_val); //send the mid_prod_val to the previous kernel for the update
                }

                outQPtr = QPtrBase;
                inAPtr = inAPtrBase;  
                for (int row_chunk = col_chunk_in_calc; row_chunk < m_kRowChunkNum; row_chunk++){
                    mid_prod_vect = ::aie::mul(mid_prod_val, *inAPtr++);   
                    q_vect = ::aie::sub(*outQPtr, mid_prod_vect); 
                    *outQPtr++ = q_vect; // save the updated q_vect back to the input buffer
                }

            } // end of i loop
        } // end of col loop
    } // end of col_chunk loop

}; // end of qrd_hh_q function



// Base specialization, used for static size window API configurations
template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POS,
          bool TP_ROUT_EN,
          bool TP_STREAM_EN>
NOINLINE_DECL void
qrd_hh<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, TP_KERNEL_POS, TP_ROUT_EN, TP_STREAM_EN>::qrd_hh_main(
                        input_buffer<TT_DATA>& __restrict inWindowA,  
                       output_buffer<TT_DATA>& __restrict outWindowQ,
                       output_buffer<TT_DATA>& __restrict outWindowR) {
    T_inputIF<TP_STREAM_EN, TT_DATA> inInterface(inWindowA);
    T_outputIF<TP_STREAM_EN, TP_ROUT_EN, TT_DATA> outInterface(outWindowQ, outWindowR);                  
    this->qrd_hh_r_func(inInterface, outInterface);
                        }


template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POS>
INLINE_DECL void
qrd_hh<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, TP_KERNEL_POS, false, true>::qrd_hh_main(
                       input_buffer<TT_DATA>& __restrict inWindowA,  
                       input_stream<TT_DATA>* __restrict inStream,
                       output_buffer<TT_DATA>& __restrict outWindowQ,
                       output_stream<TT_DATA>* __restrict outStream ) {
    T_inputIF<true, TT_DATA> inInterface(inWindowA);
    T_outputIF<true, false, TT_DATA> outInterface(outWindowQ);    
    inInterface.inStream = inStream;
    outInterface.outStream = outStream;
    this->qrd_hh_r_casc_func(inInterface, outInterface);

        };

        
template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POS>
INLINE_DECL void
qrd_hh<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, TP_KERNEL_POS, true, true>::qrd_hh_main(
                       input_buffer<TT_DATA>& __restrict inWindowA,  
                       input_stream<TT_DATA>* __restrict inStream,
                       output_buffer<TT_DATA>& __restrict outWindowQ,
                       output_buffer<TT_DATA>& __restrict outWindowR,
                       output_stream<TT_DATA>* __restrict outStream ) {
    T_inputIF<true, TT_DATA> inInterface(inWindowA);
    T_outputIF<true, true, TT_DATA> outInterface(outWindowQ, outWindowR);    
    inInterface.inStream = inStream;
    outInterface.outStream = outStream;
    this->qrd_hh_r_casc_func(inInterface, outInterface);

        };

} // namespace qrd_hh
} // namespace aie 
} // namespace solver
}  // namespace xf