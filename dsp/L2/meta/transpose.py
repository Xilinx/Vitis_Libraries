#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
import aie_common as com
from aie_common import *

#### naming ####
#
# Name functions with prefix
#   validate_ for validators, returning boolean result and error message as a tuple.
#   update_ for updators, returning object with default value and refined candidate constraints.
#   info_ for creating information based on parameters
#   fn_ for internal functions

TP_DIM_ROWS_min = 1
TP_DIM_COLS_min = 1
TP_SSR_min = 1
TP_SSR_max = 40  # sanity cap, mirrors the cascade length limit used by the FIR family
TP_CASC_LEN_min = 1
TP_CASC_LEN_max = 40  # sanity cap, mirrors the cascade length limit used by the FIR family

kSplitModeEqual = 0
kSplitModeAligned = 1


#######################################################
########### AIE_VARIANT Updater and Validator #########
#######################################################
def update_AIE_VARIANT(args):
    return fn_update_AIE_VARIANT()


def fn_update_AIE_VARIANT():
    legal_set_AIE_VARIANT = [com.AIE, com.AIE_ML, com.AIE_MLv2]
    param_dict = {"name": "AIE_VARIANT", "enum": legal_set_AIE_VARIANT}
    return param_dict


def validate_AIE_VARIANT(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    param_dict = fn_update_AIE_VARIANT()
    return validate_legal_set(param_dict["enum"], "AIE_VARIANT", AIE_VARIANT)


#######################################################
############# TT_DATA Updater and Validator ###########
#######################################################
def update_TT_DATA(args):
    return fn_update_TT_DATA()


def fn_update_TT_DATA():
    legal_set_TT_DATA = ["int16", "cint16", "int32", "cint32", "float", "cfloat"]
    param_dict = {"name": "TT_DATA", "enum": legal_set_TT_DATA}
    return param_dict


def validate_TT_DATA(args):
    TT_DATA = args["TT_DATA"]
    param_dict = fn_update_TT_DATA()
    return validate_legal_set(param_dict["enum"], "TT_DATA", TT_DATA)


#######################################################
######### TP_DIM_ROWS Updater and Validator ###########
#######################################################
def update_TP_DIM_ROWS(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    return fn_update_TP_DIM_ROWS(AIE_VARIANT, TT_DATA)


def fn_update_TP_DIM_ROWS(AIE_VARIANT, TT_DATA):
    # Favorable-case upper bound: the largest TP_DIM_ROWS for which *some* TP_SSR (up to its own
    # maximum) could make a lane fit, i.e. the single-lane bound scaled by TP_SSR_max. This is
    # deliberately loose - it only rules out values no TP_SSR could ever rescue. The definitive,
    # split-mode-aware, concrete-TP_SSR check happens in validate_TP_SSR.
    mem_width = fnVecSampleNumMax(AIE_VARIANT, TT_DATA)
    pad_rows_max = FLOOR(fn_tpose_single_kernel_dim_max(AIE_VARIANT, TT_DATA) * TP_SSR_max, mem_width)
    param_dict = {"name": "TP_DIM_ROWS", "minimum": TP_DIM_ROWS_min, "maximum": int(pad_rows_max)}
    return param_dict


def validate_TP_DIM_ROWS(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM_ROWS = args["TP_DIM_ROWS"]
    param_dict = fn_update_TP_DIM_ROWS(AIE_VARIANT, TT_DATA)
    return validate_range([param_dict["minimum"], param_dict["maximum"]], "TP_DIM_ROWS", TP_DIM_ROWS)


#######################################################
######### TP_DIM_COLS Updater and Validator ###########
#######################################################
def update_TP_DIM_COLS(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    return fn_update_TP_DIM_COLS(AIE_VARIANT, TT_DATA)


def fn_update_TP_DIM_COLS(AIE_VARIANT, TT_DATA):
    # Favorable-case upper bound, symmetric with TP_DIM_ROWS: the single-kernel bound scaled by
    # TP_CASC_LEN_max. Raise TP_CASC_LEN to split a larger TP_DIM_COLS across more,
    # smaller-buffered kernels; the definitive check happens in validate_TP_CASC_LEN.
    mem_width = fnVecSampleNumMax(AIE_VARIANT, TT_DATA)
    pad_cols_max = FLOOR(fn_tpose_single_kernel_dim_max(AIE_VARIANT, TT_DATA) * TP_CASC_LEN_max, mem_width)
    param_dict = {"name": "TP_DIM_COLS", "minimum": TP_DIM_COLS_min, "maximum": int(pad_cols_max)}
    return param_dict


def validate_TP_DIM_COLS(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM_COLS = args["TP_DIM_COLS"]
    param_dict = fn_update_TP_DIM_COLS(AIE_VARIANT, TT_DATA)
    return validate_range([param_dict["minimum"], param_dict["maximum"]], "TP_DIM_COLS", TP_DIM_COLS)


#######################################################
########### TP_SPLIT_MODE Updater and Validator #######
#######################################################
def update_TP_SPLIT_MODE(args):
    return fn_update_TP_SPLIT_MODE()


def fn_update_TP_SPLIT_MODE():
    param_dict = {"name": "TP_SPLIT_MODE", "enum": [kSplitModeEqual, kSplitModeAligned]}
    return param_dict


def validate_TP_SPLIT_MODE(args):
    TP_SPLIT_MODE = args["TP_SPLIT_MODE"]
    param_dict = fn_update_TP_SPLIT_MODE()
    return validate_legal_set(param_dict["enum"], "TP_SPLIT_MODE", TP_SPLIT_MODE)


#######################################################
############# TP_SSR Updater and Validator ############
#######################################################
def update_TP_SSR(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM_ROWS = args["TP_DIM_ROWS"]
    return fn_update_TP_SSR(AIE_VARIANT, TT_DATA, TP_DIM_ROWS)


def fn_update_TP_SSR(AIE_VARIANT, TT_DATA, TP_DIM_ROWS):
    param_dict = {
        "name": "TP_SSR",
        "minimum": TP_SSR_min,
        "maximum": int(min(TP_SSR_max, TP_DIM_ROWS)),
    }
    return param_dict


def validate_TP_SSR(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM_ROWS = args["TP_DIM_ROWS"]
    TP_DIM_COLS = args["TP_DIM_COLS"]
    TP_SSR = args["TP_SSR"]
    TP_SPLIT_MODE = args["TP_SPLIT_MODE"]

    param_dict = fn_update_TP_SSR(AIE_VARIANT, TT_DATA, TP_DIM_ROWS)
    range_check = validate_range([param_dict["minimum"], param_dict["maximum"]], "TP_SSR", TP_SSR)
    if range_check["is_valid"] == False:
        return range_check

    # Every lane must own at least one row, and all lanes together must cover exactly
    # TP_DIM_ROWS (see the equivalent TP_CASC_LEN coverage check for why this is an exact-sum
    # check rather than just "last lane's count is positive").
    rowsPerLane = [
        fn_tpose_cols_per_kernel(TP_DIM_ROWS, TP_SSR, sp, AIE_VARIANT, TT_DATA, TP_SPLIT_MODE)
        for sp in range(TP_SSR)
    ]
    if sum(rowsPerLane) != TP_DIM_ROWS or min(rowsPerLane) < 1:
        return isError(
            f"TP_SPLIT_MODE={TP_SPLIT_MODE} cannot divide TP_DIM_ROWS={TP_DIM_ROWS} across "
            f"TP_SSR={TP_SSR} lanes while giving every lane at least one row "
            f"(got per-lane counts {rowsPerLane}). Reduce TP_SSR or use TP_SPLIT_MODE=0."
        )

    # Favorable-case tile-memory check: TP_CASC_LEN hasn't been fixed yet at this point in
    # validation (mirrors matrix_mult.py's update_TP_SSR reading a conservative "TP_CASC_LEN at
    # its own maximum" assumption rather than a concrete value). If even the smallest possible
    # per-kernel column count (TP_CASC_LEN at its max) can't make lane 0 fit, no TP_CASC_LEN
    # value will rescue this TP_SSR - so reject it here. The definitive, concrete-TP_SSR-aware
    # check happens in validate_TP_CASC_LEN once TP_CASC_LEN is itself being chosen.
    effectiveMaxCasc = min(TP_CASC_LEN_max, TP_DIM_COLS)
    bestCaseColsPerKernel = (TP_DIM_COLS + effectiveMaxCasc - 1) // effectiveMaxCasc
    bufBytes = fn_tpose_buf_bytes(rowsPerLane[0], bestCaseColsPerKernel, AIE_VARIANT, TT_DATA)
    memLimit = k_data_memory_bytes[AIE_VARIANT]
    if bufBytes > memLimit:
        return isError(
            f"With TT_DATA={TT_DATA}, TP_DIM_ROWS={TP_DIM_ROWS}, TP_SPLIT_MODE={TP_SPLIT_MODE} and "
            f"TP_SSR={TP_SSR}, lane 0 would need at least a {bufBytes}-byte input+output buffer "
            f"even with TP_CASC_LEN at its maximum ({effectiveMaxCasc}), exceeding this AIE "
            f"variant's {memLimit}-byte tile data memory. Reduce TP_DIM_ROWS or increase TP_SSR."
        )
    return isValid


#######################################################
########### TP_CASC_LEN Updater and Validator #########
#######################################################
def update_TP_CASC_LEN(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM_COLS = args["TP_DIM_COLS"]
    return fn_update_TP_CASC_LEN(AIE_VARIANT, TT_DATA, TP_DIM_COLS)


def fn_update_TP_CASC_LEN(AIE_VARIANT, TT_DATA, TP_DIM_COLS):
    param_dict = {
        "name": "TP_CASC_LEN",
        "minimum": TP_CASC_LEN_min,
        "maximum": int(min(TP_CASC_LEN_max, TP_DIM_COLS)),
    }
    return param_dict


def validate_TP_CASC_LEN(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM_ROWS = args["TP_DIM_ROWS"]
    TP_DIM_COLS = args["TP_DIM_COLS"]
    TP_SSR = args["TP_SSR"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    TP_SPLIT_MODE = args["TP_SPLIT_MODE"]

    param_dict = fn_update_TP_CASC_LEN(AIE_VARIANT, TT_DATA, TP_DIM_COLS)
    range_check = validate_range(
        [param_dict["minimum"], param_dict["maximum"]], "TP_CASC_LEN", TP_CASC_LEN
    )
    if range_check["is_valid"] == False:
        return range_check

    # Every kernel must own at least one column. Under TP_SPLIT_MODE=1 (aligned), rounding each
    # non-last kernel up to the memory access width can consume the whole of TP_DIM_COLS before
    # the last kernel is reached, leaving it with zero (or, in the unsigned C++ arithmetic, a
    # huge wrapped-around) columns - so this is checked as an exact coverage invariant rather
    # than just "last kernel's count is positive".
    colsPerKernel = [
        fn_tpose_cols_per_kernel(TP_DIM_COLS, TP_CASC_LEN, kp, AIE_VARIANT, TT_DATA, TP_SPLIT_MODE)
        for kp in range(TP_CASC_LEN)
    ]
    if sum(colsPerKernel) != TP_DIM_COLS or min(colsPerKernel) < 1:
        return isError(
            f"TP_SPLIT_MODE={TP_SPLIT_MODE} cannot divide TP_DIM_COLS={TP_DIM_COLS} across "
            f"TP_CASC_LEN={TP_CASC_LEN} kernels while giving every kernel at least one column "
            f"(got per-kernel counts {colsPerKernel}). Reduce TP_CASC_LEN or use TP_SPLIT_MODE=0."
        )

    # Definitive, split-mode-aware, per-kernel tile-memory-fit check, now using the concrete,
    # already-fixed TP_SSR to compute lane 0's actual row count (mirrors matrix_mult.py's
    # update_TP_CASC_LEN reading args["TP_SSR"] directly). Index 0 is always the largest (or
    # tied-largest) lane/kernel under both split modes, so (lane 0, kernel 0) alone bounds the
    # worst-case per-kernel buffer.
    rowsThisLane = fn_tpose_cols_per_kernel(TP_DIM_ROWS, TP_SSR, 0, AIE_VARIANT, TT_DATA, TP_SPLIT_MODE)
    bufBytes = fn_tpose_buf_bytes(rowsThisLane, colsPerKernel[0], AIE_VARIANT, TT_DATA)
    memLimit = k_data_memory_bytes[AIE_VARIANT]
    if bufBytes > memLimit:
        return isError(
            f"With TT_DATA={TT_DATA}, TP_DIM_ROWS={TP_DIM_ROWS}, TP_DIM_COLS={TP_DIM_COLS}, "
            f"TP_SPLIT_MODE={TP_SPLIT_MODE}, TP_SSR={TP_SSR} and TP_CASC_LEN={TP_CASC_LEN}, "
            f"kernel (0,0) would need a {bufBytes}-byte input+output buffer, exceeding this AIE "
            f"variant's {memLimit}-byte tile data memory. Increase TP_CASC_LEN and/or TP_SSR."
        )
    return isValid


#######################################################
########### TP_CONJUGATE Updater and Validator ########
#######################################################
def update_TP_CONJUGATE(args):
    return fn_update_TP_CONJUGATE()


def fn_update_TP_CONJUGATE():
    param_dict = {"name": "TP_CONJUGATE", "enum": [0, 1]}
    return param_dict


def validate_TP_CONJUGATE(args):
    TT_DATA = args["TT_DATA"]
    TP_CONJUGATE = args["TP_CONJUGATE"]
    param_dict = fn_update_TP_CONJUGATE()
    legalCheck = validate_legal_set(param_dict["enum"], "TP_CONJUGATE", TP_CONJUGATE)
    if legalCheck["is_valid"] == False:
        return legalCheck
    if TP_CONJUGATE == 1 and not fn_is_complex(TT_DATA):
        return isError(f"TP_CONJUGATE requires a complex TT_DATA type. Got TT_DATA={TT_DATA}.")
    return isValid


#######################################################
####################### helpers #######################
#######################################################
def fn_tpose_single_kernel_dim_max(AIE_VARIANT, TT_DATA):
    # Largest square (rows==cols) single-lane/single-kernel buffer that fits the tile, before
    # padding.
    memLimit = k_data_memory_bytes[AIE_VARIANT]
    maxSamplesTotal = memLimit / fn_size_by_byte(TT_DATA) / 2  # in + out buffers
    return int(maxSamplesTotal ** 0.5)


def fn_tpose_cols_per_kernel(dimSize, splitLen, pos, AIE_VARIANT, TT_DATA, TP_SPLIT_MODE):
    # Dimension-agnostic split math - called with (TP_DIM_ROWS, TP_SSR, ssrPos, ...) for the row
    # axis and (TP_DIM_COLS, TP_CASC_LEN, cascPos, ...) for the column axis. Mirrors
    # fnTposeColsPerKernel()/fnTposeRangeEqual()/fnTposeRangeAligned() in transpose_utils.hpp.
    if TP_SPLIT_MODE == kSplitModeEqual:
        base = dimSize // splitLen
        rem = dimSize - base * splitLen
        return base + (1 if pos < rem else 0)
    else:
        memWidth = fnVecSampleNumMax(AIE_VARIANT, TT_DATA)
        avgSize = (dimSize + splitLen - 1) // splitLen
        sizePerLane = CEIL(avgSize, memWidth)
        if pos < splitLen - 1:
            return sizePerLane
        else:
            return dimSize - sizePerLane * (splitLen - 1)


def fn_tpose_buf_bytes(rowsThisKernel, colsThisKernel, AIE_VARIANT, TT_DATA):
    memWidth = fnVecSampleNumMax(AIE_VARIANT, TT_DATA)
    padRows = CEIL(rowsThisKernel, memWidth)
    padCols = CEIL(colsThisKernel, memWidth)
    return (padRows * padCols + padCols * padRows) * fn_size_by_byte(TT_DATA)


#### port ####
def info_ports(args):
    """Standard function creating a static dictionary of information for upper software to
    correctly connect the IP. transpose has a configurable number of ports (TP_SSR*TP_CASC_LEN),
    so port information is implemented as a function."""
    TT_DATA = args["TT_DATA"]
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_DIM_ROWS = args["TP_DIM_ROWS"]
    TP_DIM_COLS = args["TP_DIM_COLS"]
    TP_SSR = args["TP_SSR"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    TP_SPLIT_MODE = args["TP_SPLIT_MODE"]

    memWidth = fnVecSampleNumMax(AIE_VARIANT, TT_DATA)
    rowsPerLane = [
        fn_tpose_cols_per_kernel(TP_DIM_ROWS, TP_SSR, sp, AIE_VARIANT, TT_DATA, TP_SPLIT_MODE)
        for sp in range(TP_SSR)
    ]
    colsPerKernel = [
        fn_tpose_cols_per_kernel(TP_DIM_COLS, TP_CASC_LEN, kp, AIE_VARIANT, TT_DATA, TP_SPLIT_MODE)
        for kp in range(TP_CASC_LEN)
    ]

    portsIn = []
    portsOut = []
    for ssrIdx in range(TP_SSR):
        for cascIdx in range(TP_CASC_LEN):
            kp = ssrIdx * TP_CASC_LEN + cascIdx
            padRows = CEIL(rowsPerLane[ssrIdx], memWidth)
            padCols = CEIL(colsPerKernel[cascIdx], memWidth)
            portsIn.append(
                {
                    "name": f"in[{kp}]",
                    "type": "window",
                    "direction": "in",
                    "data_type": TT_DATA,
                    "fn_is_complex": fn_is_complex(TT_DATA),
                    "window_size": (padRows * padCols) * fn_size_by_byte(TT_DATA),
                    "margin_size": 0,
                }
            )
            portsOut.append(
                {
                    "name": f"out[{kp}]",
                    "type": "window",
                    "direction": "out",
                    "data_type": TT_DATA,
                    "fn_is_complex": fn_is_complex(TT_DATA),
                    "window_size": (padCols * padRows) * fn_size_by_byte(TT_DATA),
                    "margin_size": 0,
                }
            )
    return portsIn + portsOut


#### graph generator ####
def generate_graph(graphname, args):
    if graphname == "":
        graphname = "default_graphname"

    TT_DATA = args["TT_DATA"]
    TP_DIM_ROWS = args["TP_DIM_ROWS"]
    TP_DIM_COLS = args["TP_DIM_COLS"]
    TP_SSR = args["TP_SSR"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    TP_SPLIT_MODE = args["TP_SPLIT_MODE"]
    TP_CONJUGATE = args["TP_CONJUGATE"]

    code = f"""
class {graphname} : public adf::graph {{
public:
  static constexpr unsigned int TP_SSR = {TP_SSR};
  static constexpr unsigned int TP_CASC_LEN = {TP_CASC_LEN};
  static constexpr unsigned int kNumKernels = TP_SSR * TP_CASC_LEN;
  template <typename dir>
  using tpose_port_array = std::array<adf::port<dir>, kNumKernels>;

  tpose_port_array<input> in;
  tpose_port_array<output> out;

  xf::dsp::aie::transpose::transpose_graph<
    {TT_DATA}, //TT_DATA
    {TP_DIM_ROWS}, //TP_DIM_ROWS
    {TP_DIM_COLS}, //TP_DIM_COLS
    {TP_SSR}, //TP_SSR
    {TP_CASC_LEN}, //TP_CASC_LEN
    {TP_SPLIT_MODE}, //TP_SPLIT_MODE
    {TP_CONJUGATE} //TP_CONJUGATE
  > transposeGraph;

  {graphname}() : transposeGraph() {{
    for (unsigned int i=0; i < kNumKernels; i++) {{
      adf::connect<> net_in(in[i], transposeGraph.in[i]);
      adf::connect<> net_out(transposeGraph.out[i], out[i]);
    }}
  }}

}};
"""
    out = {}
    out["graph"] = code
    out["port_info"] = info_ports(args)
    out["headerfile"] = "transpose_graph.hpp"
    out["searchpaths"] = [
        "L2/include/aie",
        "L2/tests/aie/common/inc",
        "L1/include/aie",
        "L1/src/aie",
        "L1/tests/aie/inc",
        "L1/tests/aie/src",
    ]

    return out
