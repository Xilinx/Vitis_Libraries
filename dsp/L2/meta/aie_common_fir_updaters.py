#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
# Common utility variables and functions used accross FIR elements.

import aie_common as com
from aie_common import *
from aie_common_fir import *
import fir_polyphase_decomposer as poly

TP_DECIMATE_FACTOR_min = 2
TP_DECIMATE_FACTOR_max = 7
TP_INTERPOLATE_FACTOR_min = 1
TP_INTERPOLATE_FACTOR_max_aie1 = 16
TP_INTERPOLATE_FACTOR_max_aie2 = 8
TP_INPUT_WINDOW_VSIZE_min = 4
TP_PARA_DECI_POLY_min = 1
TP_PARA_INTERP_POLY_min = 1
TP_CASC_LEN_min = 1
TP_CASC_LEN_max = 40
TP_FIR_LEN_min = 4
TP_FIR_LEN_max = 8192
TP_SSR_min = 1


def fn_type_support_coeff_update(AIE_VARIANT, TT_DATA, legal_set):
    legal_set_int = legal_set
    if AIE_VARIANT == AIE:
        if TT_DATA == "int16":
            legal_set_int = remove_from_set(["int16"], legal_set_int)
    return legal_set_int


def fn_type_aieml_support_update(AIE_VARIANT, TT_DATA, legal_set):
    legal_set_int = legal_set
    # Not aware of any restrictions yet, default to AIE-ML.
    if AIE_VARIANT == AIE_MLv2:
        if TT_DATA == "cfloat":
            legal_set_int = remove_from_set(["cfloat"], legal_set_int)
    # Cfloat is not supported by API for AIE-ML device.
    if AIE_VARIANT == AIE_ML:
        if TT_DATA == "cfloat":
            legal_set_int = remove_from_set(["cfloat"], legal_set_int)
    # Bfloat16 is not supported by AIE HW.
    if AIE_VARIANT == AIE:
        if TT_DATA == "bfloat16":
            legal_set_int = remove_from_set(["bfloat16"], legal_set_int)
        if TT_DATA == "cbfloat16":
            legal_set_int = remove_from_set(["cbfloat16"], legal_set_int)
    return legal_set_int


def fn_max_fir_len_each_kernel_update(
    TT_DATA, TP_CASC_LEN, TP_USE_COEFF_RELOAD, TP_SSR=1, TP_API=0, symFactor=1
):
    # Coeff array needs storage on heap and unrolled MAC operation inflate Program Memory.
    firLengthMaxCoeffArray = 256 * symFactor
    # Data samples must fit into 1024-bit (128 Byte) vector register
    firLengthMaxDataReg = 128 * symFactor / fn_size_by_byte(TT_DATA)
    # Fir length per kernel in a cascaded design that may also be decomposed into multiple SSR paths.
    if TP_API == API_BUFFER:
        # When buffer IO, check that the coeff array fits into heap
        if TP_USE_COEFF_RELOAD == 1:
            # Coeff array gets divided up in SSR mode, where each SSR phase gets a fraction of the array.
            TP_FIR_LEN_max = int(firLengthMaxCoeffArray * TP_SSR)
        else:
            TP_FIR_LEN_max = int(firLengthMaxCoeffArray * (TP_CASC_LEN * TP_SSR))
    else:
        # When stream IO, check that the data fits into a 1024-bit reg. Coeff Array condition always met.
        TP_FIR_LEN_max = int(firLengthMaxDataReg * (TP_CASC_LEN * TP_SSR))
    return TP_FIR_LEN_max


def fn_update_binary(param_name):
    param_dict = {"name": param_name, "enum": [0, 1]}
    return param_dict


def fn_update_dual_ip_sr_asym(AIE_VARIANT, TP_API):
    legal_set = [0, 1]

    if TP_API == API_STREAM:
        if (
            AIE_VARIANT == AIE_ML or AIE_VARIANT == AIE_MLv2
        ):  # Under this condition 2 outputs will not be supported, hence TP_DUAL_IP should also be 0.
            legal_set = [0]
    else:
        legal_set = [0]
    param_dict = {"name": "TP_DUAL_IP", "enum": legal_set}
    return param_dict


def fn_update_num_outputs(TP_API, AIE_VARIANT, param_name):
    legal_set = [1, 2]
    if TP_API == API_STREAM:
        if AIE_VARIANT == AIE_ML or AIE_VARIANT == AIE_MLv2:
            # HW only supports 1 stream
            legal_set = [1]
    param_dict = {"name": param_name, "enum": legal_set}
    return param_dict


def fn_update_num_outputs_sr_asym(TP_API, AIE_VARIANT, TP_SSR, TP_DUAL_IP, param_name):
    param_dict = fn_update_num_outputs(TP_API, AIE_VARIANT, param_name)
    legal_set = param_dict["enum"]
    if TP_API == API_BUFFER:
        # When buffer API, single output or 2 output clones are both valid use cases.
        legal_set = legal_set
    if TP_API == API_STREAM:
        # When stream API, keep IO connections same,
        # i.e. when 1 input stream 1 output stream, when 2 inputs, allow 2 outputs.
        # When 2 inputs, does 1 output make sense? No.
        if TP_DUAL_IP == 0:
            legal_set = remove_from_set([2], legal_set)
        else:
            legal_set = remove_from_set([1], legal_set)

    # Constraint copied from graph. Can this be removed?
    if TP_API == API_BUFFER and TP_SSR > 1:
        legal_set = remove_from_set([2], legal_set)

    param_dict.update({"enum": legal_set})

    return param_dict


def fn_update_interp_dual_ip(AIE_VARIANT, TP_API):
    legal_set_TP_DUAL_IP = [0]
    if TP_API == API_STREAM:
        if AIE_VARIANT == AIE:  # Replace with k_tile_io_streams[AIE_VARIANT] >= 2 ?
            legal_set_TP_DUAL_IP = [0, 1]

    param_dict = {"name": "TP_DUAL_IP", "enum": legal_set_TP_DUAL_IP}
    return param_dict


def fn_update_hb_num_outputs(TP_PARA_POLY, TP_DUAL_IP, param_name):
    legal_set = [1, 2]
    if TP_PARA_POLY == 2:
        legal_set = [TP_DUAL_IP + 1]

    param_dict = {"name": param_name, "enum": legal_set}
    return param_dict


def fn_eliminate_casc_len_min_fir_len_each_kernel(
    legal_set, TP_FIR_LEN, TP_SSR=1, TP_Rnd=1
):
    firLen = TP_FIR_LEN // TP_SSR
    firLengthMin = 1
    vld = True
    legal_set_int = legal_set
    # Check that the last and second last kernel has at least the minimum required taps.
    for casc_len in legal_set.copy():
        if casc_len > 1:
            vld = (
                fnFirRangeRem(firLen, casc_len, casc_len - 1, TP_Rnd) >= firLengthMin
                and fnFirRange(firLen, casc_len, casc_len - 2, TP_Rnd) >= firLengthMin
            )
        else:
            vld = firLen >= firLengthMin

        if not vld:
            remove_from_set([casc_len], legal_set_int)

    return legal_set_int


def fn_eliminate_casc_len_max_fir_len_each_kernel(
    TT_DATA,
    TP_FIR_LEN,
    TP_USE_COEFF_RELOAD,
    TP_SSR=1,
    TP_API=0,
    symFactor=1,
    legal_set_casc=[],
):
    # Coeff array needs storage on heap and unrolled MAC operation inflate Program Memory.
    firLengthMaxCoeffArray = 256 * symFactor
    # Data samples must fit into 1024-bit (128 Byte) vector register
    firLengthMaxDataReg = 128 * symFactor / fn_size_by_byte(TT_DATA)
    # Fir length per kernel in a cascaded design that may also be decomposed into multiple SSR paths.
    legal_set_casc_int = legal_set_casc
    for casc_len in legal_set_casc.copy():
        firLengthPerKernel = TP_FIR_LEN / (casc_len * TP_SSR)

        if TP_API == API_BUFFER:
            # When buffer IO, check that the coeff array fits into heap
            if TP_USE_COEFF_RELOAD == 1:
                # Coeff array gets divided up in SSR mode, where each SSR phase gets a fraction of the array.
                if TP_FIR_LEN / (TP_SSR) <= firLengthMaxCoeffArray:
                    vld = True
                else:
                    vld = False
            else:
                if firLengthPerKernel <= firLengthMaxCoeffArray:
                    vld = True
                else:
                    vld = False
        else:
            # When stream IO, check that the data fits into a 1024-bit reg. Coeff Array condition always met.
            if firLengthPerKernel <= firLengthMaxDataReg:
                vld = True
            else:
                vld = False

        if not vld:
            legal_set_casc_int = remove_from_set([casc_len], legal_set_casc_int)

    return legal_set_casc_int


def fn_max_windowsize_for_buffer_update(
    TT_DATA,
    TP_FIR_LEN,
    TP_SSR=1,
    TP_INTERPOLATE_FACTOR=1,
    TP_DECIMATE_FACTOR=1,
    AIE_VARIANT=1,
):
    kMemoryModuleSize = k_data_memory_bytes[AIE_VARIANT]
    TP_FIR_LEN = TP_FIR_LEN // TP_SSR  # FIR Length gets reduced by SSR factor
    # max_in_window_size=Margin + requested window size in bytes
    TP_INPUT_WINDOW_VSIZE_inmax = (
        kMemoryModuleSize / fn_size_by_byte(TT_DATA)
    ) - TP_FIR_LEN
    # no margin
    # max_out_window_size=rate change is applied to buffer size
    TP_INPUT_WINDOW_VSIZE_outmax = (
        (kMemoryModuleSize / fn_size_by_byte(TT_DATA)) * TP_DECIMATE_FACTOR
    ) // TP_INTERPOLATE_FACTOR
    # the smallest value of in-max and out-max determines the max windowsize limit
    TP_INPUT_WINDOW_VSIZE_max = min(
        TP_INPUT_WINDOW_VSIZE_inmax, TP_INPUT_WINDOW_VSIZE_outmax
    )

    return int(TP_INPUT_WINDOW_VSIZE_max)


def fn_max_windowsize_for_buffer(
    TT_DATA,
    TP_FIR_LEN,
    TP_INPUT_WINDOW_VSIZE,
    TP_API,
    TP_SSR=1,
    TP_INTERPOLATE_FACTOR=1,
    TP_DECIMATE_FACTOR=1,
    AIE_VARIANT=1,
):
    kMemoryModuleSize = k_data_memory_bytes[AIE_VARIANT]
    TP_FIR_LEN = TP_FIR_LEN // TP_SSR  # FIR Length gets reduced by SSR factor
    # Margin + requested window size in bytes
    inBufferSize = (TP_FIR_LEN + TP_INPUT_WINDOW_VSIZE) * fn_size_by_byte(TT_DATA)
    # no margin
    outBufferSize = (
        (TP_INPUT_WINDOW_VSIZE * TP_INTERPOLATE_FACTOR) // TP_DECIMATE_FACTOR
    ) * fn_size_by_byte(TT_DATA)

    if TP_API == API_BUFFER:
        if inBufferSize > kMemoryModuleSize:
            return isError(
                f"Input buffer size ({inBufferSize}B) exceeds Memory Module size of {k_mem_module_size[AIE_VARIANT]}."
            )
        if outBufferSize > kMemoryModuleSize:
            return isError(
                f"Output buffer size ({outBufferSize}B) exceeds Memory Module size of {k_mem_module_size[AIE_VARIANT]}."
            )

    return isValid


def fn_update_tt_data(AIE_VARIANT):
    legal_set_TT_DATA = [
        "int16",
        "cint16",
        "int32",
        "cint32",
        "float",
        "bfloat16",  # not supported
        # "cbfloat16",  # not supported
        "cfloat",
    ]

    if AIE_VARIANT == AIE_ML or AIE_VARIANT == AIE_MLv2:
        legal_set_TT_DATA = remove_from_set(["cfloat"], legal_set_TT_DATA)
    param_dict = {"name": "TT_DATA", "enum": legal_set_TT_DATA}
    return param_dict


def fn_update_tt_coeff(AIE_VARIANT, TT_DATA):
    legal_set_TT_COEFF = [
        "int16",
        "cint16",
        "int32",
        "cint32",
        "float",
        "bfloat16",  # not supported
        # "cbfloat16",  # not supported
        "cfloat",
    ]
    legal_set_TT_COEFF = fn_coeff_type_update(TT_DATA, legal_set_TT_COEFF)
    legal_set_TT_COEFF = fn_type_aieml_support_update(
        AIE_VARIANT, TT_DATA, legal_set_TT_COEFF
    )
    param_dict = {"name": "TT_COEFF", "enum": legal_set_TT_COEFF}

    return param_dict


def fn_update_interpolate_factor(AIE_VARIANT, param_name):
    TP_INTERPOLATE_FACTOR_max_int = 1  # Default for new variants
    if AIE_VARIANT == AIE:
        TP_INTERPOLATE_FACTOR_max_int = TP_INTERPOLATE_FACTOR_max_aie1
    elif AIE_VARIANT == AIE_ML:
        TP_INTERPOLATE_FACTOR_max_int = TP_INTERPOLATE_FACTOR_max_aie2
    elif AIE_VARIANT == AIE_MLv2:
        TP_INTERPOLATE_FACTOR_max_int = TP_INTERPOLATE_FACTOR_max_aie2

    param_dict = {
        "name": param_name,
        "minimum": TP_INTERPOLATE_FACTOR_min,
        "maximum": TP_INTERPOLATE_FACTOR_max_int,
    }

    return param_dict


def fn_check_factor(param, factor, param_name):
    if param % factor != 0:
        return isError(f"{param_name} should be a multiple of {factor}!")
    else:
        return isValid


def fn_greater_precision_data_in_out_update(TT_DATA, legal_set):
    data_precision = (
        k_base_type_size_map[k_base_type_map[TT_DATA]] * 8
    )  # Byte sized map
    remove_set = []
    for tt_data_out in legal_set.copy():
        tt_data_out_precision = (
            k_base_type_size_map[k_base_type_map[tt_data_out]] * 8
        )  # Byte sized map
        if data_precision > tt_data_out_precision:
            remove_set.append(tt_data_out)
    legal_set_out = remove_from_set(remove_set, legal_set.copy())
    return legal_set_out
