#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
#
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
# shortcut
isValid = {"is_valid": True}
def isError(msg) :
  return {"is_valid" : False, "err_message" : msg}

dictlist = ["fir_tdm","dds_mixer","dds_mixer_lut","dft","fft_window","fft_ifft_dit_1ch","fir_decimate_asym","fir_decimate_hb","fir_decimate_sym",
"fir_interpolate_asym","fir_interpolate_hb","fir_resampler","fir_sr_sym","fir_sr_asym","mixed_radix_fft","sample_delay"]

dds_mixer = {
"ERROR: ":"",
"TT_DATA":"data_type",
"TT_OUT_DATA":"data_out_type",
"TT_COEFF": "coeff_type",
"TT_TWIDDLE":"twiddle_type",
"TP_SSR":"ssr",
"phaseInc":"phase_increment",
"initialPhaseOffset":"initial_phase_offset",
"TP_INPUT_WINDOW_VSIZE":"Input window size",
"TP_MIXER_MODE":"mixer_mode",
"TP_API":"TP_API",
"TP_POINT_SIZE":"point_size",
"TP_NUM_FRAMES":"num_frames",
"TP_CASC_LEN":"casc_length",
"TP_SHIFT":"shift_val",
"TP_WINDOW_VSIZE":"Input size",
"TP_RND":"rnd_mode (Rounding mode)",
"TP_SAT":"sat_mode (Saturation mode)",
"TP_UPSHIFT_CT":"Upshift",
"MIXER_MODE_0":"DDS block",
"TP_DECIMATE_FACTOR":"decimate_factor",
"TP_NUM_OUTPUTS":"num_outputs",
"TP_DUAL_IP":"dual_ip",
"TP_PARA_INTERP_POLY":"interp_poly (Interpolation poly phase)",
"TP_FIR_LEN":"fir_length (Filter length)",
"TP_INTERPOLATE_FACTOR":"interpolate_factor"
}


dds_mixer_lut = {
"ERROR: ":"",
"TT_DATA":"data_type",
"TT_OUT_DATA":"data_out_type",
"TT_COEFF": "coeff_type",
"TT_TWIDDLE":"twiddle_type",
"TP_SSR":"ssr",
"phaseInc":"phase_increment",
"initialPhaseOffset":"initial_phase_offset",
"TP_INPUT_WINDOW_VSIZE":"Input window size",
"TP_MIXER_MODE":"mixer_mode",
"TP_API":"TP_API",
"TP_POINT_SIZE":"point_size",
"TP_NUM_FRAMES":"num_frames",
"TP_CASC_LEN":"casc_length",
"TP_SHIFT":"shift_val",
"TP_WINDOW_VSIZE":"Input size",
"TP_RND":"rnd_mode (Rounding mode)",
"TP_SAT":"sat_mode (Saturation mode)",
"TP_SFDR":"sfdr",
"MIXER_MODE_0":"DDS block",
"TP_DECIMATE_FACTOR":"decimate_factor",
"TP_NUM_OUTPUTS":"num_outputs",
"TP_DUAL_IP":"dual_ip",
"TP_PARA_INTERP_POLY":"interp_poly (Interpolation poly phase)",
"TP_FIR_LEN":"fir_length (Filter length)",
"TP_INTERPOLATE_FACTOR":"interpolate_factor"
}

fir_tdm = {
"ERROR: ":"",
"TT_DATA":"data_type",
"TT_OUT_DATA":"data_out_type",
"TT_COEFF": "coeff_type",
"TT_TWIDDLE":"twiddle_type",
"TP_SSR":"ssr",
"phaseInc":"phase_increment",
"initialPhaseOffset":"initial_phase_offset",
"TP_INPUT_WINDOW_VSIZE":"Input window size",
"TP_MIXER_MODE":"mixer_mode",
"TP_API":"TP_API",
"TP_POINT_SIZE":"point_size",
"TP_NUM_FRAMES":"num_frames",
"TP_CASC_LEN":"casc_length",
"TP_SHIFT":"shift_val",
"TP_WINDOW_VSIZE":"Input size",
"TP_RND":"rnd_mode (Rounding mode)",
"TP_SAT":"sat_mode (Saturation mode)",
"TP_UPSHIFT_CT":"Upshift",
"TP_PARA_DECI_POLY":"deci_poly (Decimation poly phase)",
"TP_TDM_CHANNELS":"tdm_channels",
"TP_NUM_OUTPUTS":"num_outputs",
"TP_DUAL_IP":"dual_ip",
"TP_PARA_INTERP_POLY":"interp_poly (Interpolation poly phase)",
"TP_FIR_LEN":"fir_length (Filter length)",
"TP_INTERPOLATE_FACTOR":"interpolate_factor"
}

dft = {
"ERROR: ":"",
"TT_COEFF": "coeff_type",
"TT_OUT_DATA":"data_out_type",
"TT_DATA":"data_type",
"TT_TWIDDLE":"twiddle_type",
"TP_SSR":"ssr",
"TP_INPUT_WINDOW_VSIZE":"Input window size",
"TP_API":"TP_API",
"TP_POINT_SIZE":"point_size",
"TP_NUM_FRAMES":"num_frames",
"TP_CASC_LEN":"casc_length",
"TP_SHIFT":"shift_val",
"TP_WINDOW_VSIZE":"Input size",
"TP_RND":"rnd_mode (Rounding mode)",
"TP_SAT":"sat_mode (Saturation mode)",
"TP_UPSHIFT_CT":"Upshift",
"TP_PARA_DECI_POLY":"deci_poly (Decimation poly phase)",
"TP_DECIMATE_FACTOR":"decimate_factor",
"TP_NUM_OUTPUTS":"num_outputs",
"TP_DUAL_IP":"dual_ip",
"TP_PARA_INTERP_POLY":"interp_poly (Interpolation poly phase)",
"TP_FIR_LEN":"fir_length (Filter length)",
"TP_INTERPOLATE_FACTOR":"interpolate_factor"
}

fft_window = {
"ERROR: ":"",
"TT_COEFF":"coeff_type",
"TP_DYN_PT_SIZE":"Dynamic point_size",
"TT_OUT_DATA":"data_out_type",
"TT_DATA":"data_type",
"TT_TWIDDLE":"twiddle_type",
"TP_SSR":"SSR",
"TP_INPUT_WINDOW_VSIZE":"Input size",
"TP_API":"TP_API",
"TP_POINT_SIZE":"point_size",
"TP_NUM_FRAMES":"num_frames",
"TP_CASC_LEN":"casc_length",
"TP_SHIFT":"shift_val",
"TP_WINDOW_VSIZE":"Input size",
"TP_RND":"rnd_mode (Rounding mode)",
"TP_SAT":"sat_mode (Saturation mode)",
"TP_UPSHIFT_CT":"Upshift",
"TP_PARA_DECI_POLY":"deci_poly (Decimation poly phase)",
"TP_DECIMATE_FACTOR":"decimate_factor",
"TP_NUM_OUTPUTS":"num_outputs",
"TP_DUAL_IP":"dual_ip",
"TP_PARA_INTERP_POLY":"interp_poly (Interpolation poly phase)",
"TP_FIR_LEN":"fir_length (Filter length)",
"TP_INTERPOLATE_FACTOR":"interpolate_factor"
}

fft_ifft_dit_1ch = {
"ERROR: ":"",
"TT_DATA":"data_type",
"TT_COEFF": "coeff_type",
"TT_OUT_DATA":"data_out_type",
"TT_TWIDDLE":"twiddle_type",
"TP_SSR":"ssr",
"TP_INPUT_WINDOW_VSIZE":"Input size",
"TP_API":"TP_API",
"TP_POINT_SIZE":"point_size",
"TP_NUM_FRAMES":"num_frames",
"TP_CASC_LEN":"casc_length",
"TP_SHIFT":"parameter Shift",
"TP_WINDOW_VSIZE":"Input size",
"TP_RND":"rnd_mode (Rounding mode)",
"TP_SAT":"sat_mode (Saturation mode)",
"TP_UPSHIFT_CT":"Upshift",
"TP_PARA_DECI_POLY":"deci_poly (Decimation poly phase)",
"TP_DECIMATE_FACTOR":"decimate_factor",
"TP_NUM_OUTPUTS":"num_outputs",
"TP_DUAL_IP":"dual_ip",
"TP_PARA_INTERP_POLY":"interp_poly (Interpolation poly phase)",
"TP_FIR_LEN":"fir_length (Filter length)",
"TP_INTERPOLATE_FACTOR":"interpolate_factor"
}

fir_decimate_asym = {
"ERROR: ":"",
"TT_DATA":"data_type",
"TT_COEFF": "coeff_type",
"TT_TWIDDLE":"twiddle_type",
"TP_SSR":"ssr",
"TP_INPUT_WINDOW_VSIZE":"Input window size",
"TP_API":"TP_API",
"TP_POINT_SIZE":"point_size",
"TP_NUM_FRAMES":"num_frames",
"TP_CASC_LEN":"casc_length",
"TP_SHIFT":"shift_val",
"TP_WINDOW_VSIZE":"Input size",
"TP_RND":"rnd_mode (Rounding mode)",
"TP_SAT":"sat_mode (Saturation mode)",
"TP_UPSHIFT_CT":"Upshift",
"TP_PARA_DECI_POLY":"deci_poly (Decimation poly phase)",
"TP_DECIMATE_FACTOR":"decimate_factor",
"TP_NUM_OUTPUTS":"num_outputs",
"TP_DUAL_IP":"dual_ip",
"TP_PARA_INTERP_POLY":"interp_poly (Interpolation poly phase)",
"TP_FIR_LEN":"fir_length (Filter length)",
"TP_INTERPOLATE_FACTOR":"interpolate_factor"
}

fir_decimate_hb = {
"ERROR: ":"",
"TT_DATA":"data_type",
"TT_COEFF": "coeff_type",
"TT_TWIDDLE":"twiddle_type",
"TP_SSR":"ssr",
"TP_INPUT_WINDOW_VSIZE":"Input window size",
"TP_API":"TP_API",
"TP_POINT_SIZE":"point_size",
"TP_NUM_FRAMES":"num_frames",
"TP_CASC_LEN":"casc_length",
"TP_SHIFT":"shift_val",
"TP_WINDOW_VSIZE":"Input size",
"TP_RND":"rnd_mode (Rounding mode)",
"TP_SAT":"sat_mode (Saturation mode)",
"TP_UPSHIFT_CT":"Upshift",
"TP_PARA_DECI_POLY":"deci_poly (Decimation poly phase)",
"TP_DECIMATE_FACTOR":"decimate_factor",
"TP_NUM_OUTPUTS":"num_outputs",
"TP_DUAL_IP":"dual_ip",
"TP_PARA_INTERP_POLY":"interp_poly (Interpolation poly phase)",
"TP_FIR_LEN":"fir_length (Filter length)",
"TP_INTERPOLATE_FACTOR":"interpolate_factor"
}

fir_decimate_sym = {
"ERROR: ":"",
"TT_DATA":"data_type",
"TT_COEFF": "coeff_type",
"TT_TWIDDLE":"twiddle_type",
"TP_SSR":"ssr",
"TP_INPUT_WINDOW_VSIZE":"Input window size",
"TP_API":"TP_API",
"TP_POINT_SIZE":"point_size",
"TP_NUM_FRAMES":"num_frames",
"TP_CASC_LEN":"casc_length",
"TP_SHIFT":"shift_val",
"TP_WINDOW_VSIZE":"Input size",
"TP_RND":"rnd_mode (Rounding mode)",
"TP_SAT":"sat_mode (Saturation mode)",
"TP_UPSHIFT_CT":"Upshift",
"TP_PARA_DECI_POLY":"deci_poly (Decimation poly phase)",
"TP_DECIMATE_FACTOR":"decimate_factor",
"TP_NUM_OUTPUTS":"num_outputs",
"TP_DUAL_IP":"dual_ip",
"TP_PARA_INTERP_POLY":"interp_poly (Interpolation poly phase)",
"TP_FIR_LEN":"fir_length (Filter length)",
"TP_INTERPOLATE_FACTOR":"interpolate_factor"
}

fir_interpolate_asym = {
"ERROR: ":"",
"TT_DATA":"data_type",
"TT_COEFF": "coeff_type",
"TT_TWIDDLE":"twiddle_type",
"TP_SSR":"ssr",
"TP_INPUT_WINDOW_VSIZE":"Input size",
"TP_API":"TP_API",
"TP_POINT_SIZE":"point_size",
"TP_NUM_FRAMES":"num_frames",
"TP_CASC_LEN":"casc_length",
"TP_SHIFT":"shift_val",
"TP_WINDOW_VSIZE":"Input size",
"TP_RND":"rnd_mode (Rounding mode)",
"TP_SAT":"sat_mode (Saturation mode)",
"TP_UPSHIFT_CT":"Upshift",
"TP_PARA_DECI_POLY":"deci_poly (Decimation poly phase)",
"TP_DECIMATE_FACTOR":"decimate_factor",
"TP_NUM_OUTPUTS":"num_outputs",
"TP_DUAL_IP":"dual_ip",
"TP_PARA_INTERP_POLY":"interp_poly (Interpolation poly phase)",
"TP_FIR_LEN":"fir_length (Filter length)",
"TP_INTERPOLATE_FACTOR":"interpolate_factor"
}

fir_interpolate_hb = {
"ERROR: ":"",
"TT_DATA":"data_type",
"TT_COEFF": "coeff_type",
"TT_TWIDDLE":"twiddle_type",
"TP_SSR":"ssr",
"TP_INPUT_WINDOW_VSIZE":"Input size",
"TP_API":"TP_API",
"TP_POINT_SIZE":"point_size",
"TP_NUM_FRAMES":"num_frames",
"TP_CASC_LEN":"casc_length",
"TP_SHIFT":"shift_val",
"TP_WINDOW_VSIZE":"Input size",
"TP_RND":"rnd_mode (Rounding mode)",
"TP_SAT":"sat_mode (Saturation mode)",
"TP_UPSHIFT_CT":"Upshift",
"TP_PARA_DECI_POLY":"deci_poly (Decimation poly phase)",
"TP_DECIMATE_FACTOR":"decimate_factor",
"TP_NUM_OUTPUTS":"num_outputs",
"TP_DUAL_IP":"dual_ip",
"TP_PARA_INTERP_POLY":"interp_poly (Interpolation poly phase)",
"TP_FIR_LEN":"fir_length (Filter length)",
"TP_INTERPOLATE_FACTOR":"interpolate_factor"
}

fir_resampler = {
"ERROR: ":"",
"TT_DATA":"data_type",
"TT_COEFF": "coeff_type",
"TT_TWIDDLE":"twiddle_type",
"TP_SSR":"ssr",
"TP_INPUT_WINDOW_VSIZE":"Input size",
"TP_API":"TP_API",
"TP_POINT_SIZE":"point_size",
"TP_NUM_FRAMES":"num_frames",
"TP_CASC_LEN":"casc_length",
"TP_SHIFT":"parameter Shift",
"TP_WINDOW_VSIZE":"Input size",
"TP_RND":"rnd_mode (Rounding mode)",
"TP_SAT":"sat_mode (Saturation mode)",
"TP_UPSHIFT_CT":"Upshift",
"TP_PARA_DECI_POLY":"deci_poly (Decimation poly phase)",
"TP_DECIMATE_FACTOR":"decimate_factor",
"TP_NUM_OUTPUTS":"num_outputs",
"TP_DUAL_IP":"dual_ip",
"TP_PARA_INTERP_POLY":"interp_poly (Interpolation poly phase)",
"TP_FIR_LEN":"fir_length (Filter length)",
"TP_INTERPOLATE_FACTOR":"interpolate_factor"
}

fir_sr_asym = {
"ERROR: ":"",
"TT_DATA":"data_type",
"TT_COEFF": "coeff_type",
"TT_TWIDDLE":"twiddle_type",
"TP_SSR":"ssr",
"TP_INPUT_WINDOW_VSIZE":"Input size",
"TP_API":"TP_API",
"TP_POINT_SIZE":"point_size",
"TP_NUM_FRAMES":"num_frames",
"TP_CASC_LEN":"casc_length",
"TP_SHIFT":"shift_val",
"TP_WINDOW_VSIZE":"Input size",
"TP_RND":"rnd_mode (Rounding mode)",
"TP_SAT":"sat_mode (Saturation mode)",
"TP_UPSHIFT_CT":"Upshift",
"TP_PARA_DECI_POLY":"deci_poly (Decimation poly phase)",
"TP_DECIMATE_FACTOR":"decimate_factor",
"TP_NUM_OUTPUTS":"num_outputs",
"TP_DUAL_IP":"dual_ip",
"TP_PARA_INTERP_POLY":"interp_poly (Interpolation poly phase)",
"TP_FIR_LEN":"fir_length (Filter length)",
"TP_INTERPOLATE_FACTOR":"interpolate_factor"
}

fir_sr_sym = {
"ERROR: ":"",
"TT_DATA":"data_type",
"TT_COEFF": "coeff_type",
"TT_TWIDDLE":"twiddle_type",
"TP_SSR":"ssr",
"TP_INPUT_WINDOW_VSIZE":"Input size",
"TP_API":"TP_API",
"TP_POINT_SIZE":"point_size",
"TP_NUM_FRAMES":"num_frames",
"TP_CASC_LEN":"casc_length",
"TP_SHIFT":"shift_val",
"TP_WINDOW_VSIZE":"Input size",
"TP_RND":"rnd_mode (Rounding mode)",
"TP_SAT":"sat_mode (Saturation mode)",
"TP_UPSHIFT_CT":"Upshift",
"TP_PARA_DECI_POLY":"deci_poly (Decimation poly phase)",
"TP_DECIMATE_FACTOR":"decimate_factor",
"TP_NUM_OUTPUTS":"num_outputs",
"TP_DUAL_IP":"dual_ip",
"TP_PARA_INTERP_POLY":"interp_poly (Interpolation poly phase)",
"TP_FIR_LEN":"fir_length (Filter length)",
"TP_INTERPOLATE_FACTOR":"interpolate_factor"
}

mixed_radix_fft = {
"ERROR: ":"",
"TT_DATA":"data_type",
"TT_COEFF": "coeff_type",
"TT_TWIDDLE":"twiddle_type",
"TP_SSR":"ssr",
"TP_INPUT_WINDOW_VSIZE":"Input window size",
"TP_API":"TP_API",
"TP_POINT_SIZE":"point_size",
"TP_NUM_FRAMES":"num_frames",
"TP_CASC_LEN":"casc_length",
"TP_SHIFT":"shift_val",
"TP_WINDOW_VSIZE":"Input size",
"TP_RND":"rnd_mode (Rounding mode)",
"TP_SAT":"sat_mode (Saturation mode)",
"TP_UPSHIFT_CT":"Upshift",
"TP_PARA_DECI_POLY":"deci_poly (Decimation poly phase)",
"TP_DECIMATE_FACTOR":"decimate_factor",
"TP_NUM_OUTPUTS":"num_outputs",
"TP_DUAL_IP":"dual_ip",
"TP_PARA_INTERP_POLY":"interp_poly (Interpolation poly phase)",
"TP_FIR_LEN":"fir_length (Filter length)",
"TP_INTERPOLATE_FACTOR":"interpolate_factor"
}

sample_delay = {
"ERROR: ":"",
"TT_DATA":"data_type",
"TT_COEFF": "coeff_type",
"TT_TWIDDLE":"twiddle_type",
"TP_SSR":"ssr",
"TP_INPUT_WINDOW_VSIZE":"Input window size",
"TP_API":"TP_API",
"TP_POINT_SIZE":"point_size",
"TP_NUM_FRAMES":"num_frames",
"TP_CASC_LEN":"casc_length",
"TP_SHIFT":"shift_val",
"TP_WINDOW_VSIZE":"Input size",
"TP_RND":"rnd_mode (Rounding mode)",
"TP_SAT":"sat_mode (Saturation mode)",
"TP_UPSHIFT_CT":"Upshift",
"TP_PARA_DECI_POLY":"deci_poly (Decimation poly phase)",
"TP_DECIMATE_FACTOR":"decimate_factor",
"TP_NUM_OUTPUTS":"num_outputs",
"TP_DUAL_IP":"dual_ip",
"TP_PARA_INTERP_POLY":"interp_poly (Interpolation poly phase)",
"TP_FIR_LEN":"fir_length (Filter length)",
"TP_INTERPOLATE_FACTOR":"interpolate_factor"
}

def ProcesStrFun(input_string, replacement_dict_str):
    if replacement_dict_str in dictlist:
        replacement_dict=eval(replacement_dict_str)
    else:
	    return input_string
    for old_word, new_word in replacement_dict.items():
        input_string = input_string.replace(old_word, new_word)
    return input_string
if __name__ == "__main__":
     print(ProcesStrFun("TP_SSR ","dds_mixer"))

