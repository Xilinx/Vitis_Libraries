#
# Copyright 2022 Xilinx, Inc.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
proc get_param_map {libElement} {
    set param_map {}
    if {$libElement eq "fft_window"} { #forgive the fork, but TT_COEF is fir speak and TT_COEFF is fft speak.
        set param_map {
            "DATA_TYPE" "TT_DATA"
            "COEFF_TYPE" "TT_COEFF"
            "POINT_SIZE" "TP_POINT_SIZE"
            "WINDOW_VSIZE" "TP_WINDOW_VSIZE"
            "SHIFT" "TP_SHIFT"
            "API_IO" "TP_API"
            "UUT_SSR" "TP_SSR"
            "DYN_PT_SIZE" "TP_DYN_PT_SIZE"
        }
    } else {
        # Everything else can be shoved in one big map
        set param_map {
            "DATA_TYPE" "TT_DATA"
            "COEFF_TYPE" "TT_COEF"
            "FIR_LEN" "TP_FIR_LEN"
            "SHIFT" "TP_SHIFT"
            "ROUND_MODE" "TP_RND"
            "INPUT_WINDOW_VSIZE" "TP_INPUT_WINDOW_VSIZE"
            "CASC_LEN" "TP_CASC_LEN"
            "INTERPOLATE_FACTOR" "TP_INTERPOLATE_FACTOR"
            "DECIMATE_FACTOR" "TP_DECIMATE_FACTOR"
            "SYMMETRY_FACTOR" ""
            "DUAL_IP" "TP_DUAL_IP"
            "USE_COEFF_RELOAD" "TP_USE_COEF_RELOAD"
            "NUM_OUTPUTS" "TP_NUM_OUTPUTS"
            "UUT_SSR" "TP_SSR"
            "UUT_PARA_DECI_POLY" "TP_PARA_DECI_POLY"
            "UUT_PARA_INTERP_POLY" "TP_PARA_INTERP_POLY"
            "PORT_API" "TP_API"
            "UPSHIFT_CT" "TP_UPSHIFT_CT"
            "TWIDDLE_TYPE" "TT_TWIDDLE"
            "POINT_SIZE" "TP_POINT_SIZE"
            "FFT_NIFFT" "TP_FFT_NIFFT"
            "DYN_PT_SIZE" "TP_DYN_PT_SIZE"
            "WINDOW_VSIZE" "TP_WINDOW_VSIZE"
            "PARALLEL_POWER" "TP_PARALLEL_POWER"
            "API_IO" "TP_API"
            "T_DATA_A" "TT_DATA_A"
            "T_DATA_B" "TT_DATA_B"
            "P_DIM_A" "TP_DIM_A"
            "P_DIM_AB" "TP_DIM_AB"
            "P_DIM_B" "TP_DIM_B"
            "P_SHIFT" "TP_SHIFT"
            "P_ROUND_MODE" "TP_RND"
            "P_DIM_A_LEADING" "TP_DIM_A_LEADING"
            "P_DIM_B_LEADING" "TP_DIM_B_LEADING"
            "P_DIM_OUT_LEADING" "TP_DIM_OUT_LEADING"
            "P_ADD_TILING_A" "TP_ADD_TILING_A"
            "P_ADD_TILING_B" "TP_ADD_TILING_B"
            "P_ADD_DETILING_OUT" "TP_ADD_DETILING_OUT"
            "P_INPUT_WINDOW_VSIZE_A" "TP_INPUT_WINDOW_VSIZE_A"
            "P_INPUT_WINDOW_VSIZE_B" "TP_INPUT_WINDOW_VSIZE_B"
            "P_CASC_LEN" "TP_CASC_LEN"
            "MIXER_MODE" "TP_MIXER_MODE"
            "P_API" "TP_API"
            "DDS_PHASE_INC" "phaseInc"
            "INITIAL_DDS_OFFSET" "initialPhaseOffset"
            "UUT_WINDOW_VSIZE" "TP_INPUT_WINDOW_VSIZE"
        }
    }
    return $param_map
}