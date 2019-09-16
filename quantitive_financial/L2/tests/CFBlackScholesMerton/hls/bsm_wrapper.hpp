/*
 * Copyright 2019 Xilinx, Inc.
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

/**
 * @file bsm_wrapper.hpp
 * @brief Declaration of wrapper for testbench
 */

#ifndef XF_FINTECH_BSM_WRAPPER_H
#ifdef __cplusplus
extern "C" {
#endif

void bsm_wrapper(float* s_in,
                 float* v_in,
                 float* r_in,
                 float* t_in,
                 float* k_in,
                 float* q_in,
                 bool call,
                 int num,
                 float* price_out,
                 float* delta_out,
                 float* gamma_out,
                 float* vega_out,
                 float* theta_out,
                 float* rho_out);

#ifdef __cplusplus
}
#endif
#endif