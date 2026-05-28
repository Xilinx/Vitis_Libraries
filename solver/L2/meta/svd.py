#
# Copyright (C) 2025, Advanced Micro Devices, Inc.
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
import sys, os
import aie_common as com
import aie_common_fir_updaters as com_upd
from aie_common import *

import math as math
from math import floor, ceil, sqrt

script_directory = os.path.dirname(os.path.abspath(__file__))
L2path = script_directory + "/.."
include_path = L2path + "/tests/aie/common/scripts"
sys.path.insert(0, include_path)

#### naming ####
#
# Name functions with prefix
#   validate_ for validators, returning boolean result and error message as a tuple.
#   update_ for updators, returning object with default value and refined candidate constraints.
#   info_ for creating information based on parameters
#   fn_ for internal functions
#
# Name function arguments as template parameters, when possible
# so the code matches easier with API definition.

TP_CASC_LEN_min = 1
TP_CASC_LEN_max = 16

def fn_kernel_vec_size(TT_DATA):
    # The current SVD kernel keeps a fixed logical vector width for metadata and
    # legality checks: 8 lanes for float, 4 lanes for cfloat. This is deliberate.
    # Although AIE-ML/AIE-MLv2 can support wider read/write widths, the current
    # implementation still couples column iteration, output padding, and cascade
    # legality to these lane counts. Metadata therefore models the implementation
    # as-shipped rather than the hardware maximum.
    return 4 if TT_DATA == "cfloat" else 8


def fn_estimated_kernel_data_bytes(TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_CASC_LEN):
    vec_size = fn_kernel_vec_size(TT_DATA)
    cols_sched = TP_DIM_COLS + (TP_DIM_COLS % 2)
    cols_padded = int(com.CEIL(cols_sched, vec_size))
    pairs_per_set = cols_sched // 2

    local_in_bytes = (TP_DIM_ROWS * TP_DIM_COLS // TP_CASC_LEN) * com.fn_size_by_byte(TT_DATA)
    local_u_bytes = (TP_DIM_ROWS * cols_padded // TP_CASC_LEN) * com.fn_size_by_byte(TT_DATA)
    local_s_bytes = cols_padded * 4
    local_v_bytes = cols_padded * cols_padded * com.fn_size_by_byte(TT_DATA)
    local_scratch_bytes = 7 * pairs_per_set * 4

    return local_in_bytes + local_u_bytes + local_s_bytes + local_v_bytes + local_scratch_bytes


#######################################################
########### AIE_VARIANT Updater and Validator #########
#######################################################
def update_AIE_VARIANT(args):
    return fn_update_aie_variant()


def fn_update_aie_variant():
    legal_set_AIE_VARIANT = [com.AIE, com.AIE_ML, com.AIE_MLv2]
    param_dict = {}

    param_dict.update({"name": "AIE_VARIANT"})
    param_dict.update({"enum": legal_set_AIE_VARIANT})

    return param_dict


def validate_AIE_VARIANT(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    param_dict = update_AIE_VARIANT(args)
    legal_set_AIE_VARIANT = param_dict["enum"]
    return com.validate_legal_set(legal_set_AIE_VARIANT, "AIE_VARIANT", AIE_VARIANT)


#######################################################
########### TT_DATA Updater and Validator #############
#######################################################
def update_TT_DATA(args):
    return fn_update_tt_data()


def fn_update_tt_data():
    legal_set_TT_DATA = ["float", "cfloat"]
    param_dict = {}

    param_dict.update({"name": "TT_DATA"})
    param_dict.update({"enum": legal_set_TT_DATA})

    return param_dict


def validate_TT_DATA(args):
    TT_DATA = args["TT_DATA"]
    param_dict = update_TT_DATA(args)
    legal_set_TT_DATA = param_dict["enum"]
    return com.validate_legal_set(legal_set_TT_DATA, "TT_DATA", TT_DATA)


#######################################################
########### TP_DIM_COLS Updater and Validator #########
#######################################################
# Limit (for now) to a max of 128. min is 2. Does not need to be a multiple of vector size.
# Independent of TP_DIM_ROWS — evaluated first in the dependency tree.
def update_TP_DIM_COLS(args):
    if "TP_DIM_COLS" in args and args["TP_DIM_COLS"]:
        TP_DIM_COLS = int(args["TP_DIM_COLS"])
    else:
        TP_DIM_COLS = 0

    return fn_update_tp_dim_cols(TP_DIM_COLS)


def fn_update_tp_dim_cols(TP_DIM_COLS):
    # Min is 2. Does not need to be a multiple of vector size.
    TP_DIM_COLS_min = 2
    # Hard max limit of 128 for now
    TP_DIM_COLS_max = 128

    param_dict = {}
    param_dict.update({"name": "TP_DIM_COLS"})
    param_dict.update({"minimum": TP_DIM_COLS_min})
    param_dict.update({"maximum": TP_DIM_COLS_max})

    return param_dict


def validate_TP_DIM_COLS(args):
    TP_DIM_COLS = int(args["TP_DIM_COLS"])

    param_dict = update_TP_DIM_COLS(args)
    range_TP_DIM_COLS = [param_dict["minimum"], param_dict["maximum"]]

    return com.validate_range(range_TP_DIM_COLS, "TP_DIM_COLS", TP_DIM_COLS)


#######################################################
########### TP_DIM_ROWS Updater and Validator #########
#######################################################
# Describes number of rows in input matrix.
# Must be a multiple of vector size. Which is 256 bits for aie1, and 512 for aie-ml and aie-mlv2. Depends on aie variant and bits of data type (cfloat or float).
# Max row size based on buffer, TP_CASC_LEN_max, and actual TP_DIM_COLS.
# Must also have at least one valid TP_CASC_LEN for the chosen rows/cols combination.
def update_TP_DIM_ROWS(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM_COLS = int(args["TP_DIM_COLS"]) if "TP_DIM_COLS" in args and args["TP_DIM_COLS"] else 2

    if "TP_DIM_ROWS" in args and args["TP_DIM_ROWS"]:
        TP_DIM_ROWS = int(args["TP_DIM_ROWS"])
    else:
        TP_DIM_ROWS = 0

    return fn_update_tp_dim_rows(AIE_VARIANT, TT_DATA, TP_DIM_COLS, TP_DIM_ROWS)


def fn_update_tp_dim_rows(AIE_VARIANT, TT_DATA, TP_DIM_COLS, TP_DIM_ROWS):
    # Kernel implementation currently uses fixed internal vector widths:
    # float -> 8 samples, cfloat -> 4 samples, independent of AIE_VARIANT.
    TP_DIM_ROWS_min = fn_kernel_vec_size(TT_DATA)

    # Max row size when TP_CASC_LEN is at maximum, using actual TP_DIM_COLS
    BUFFER_SIZE = com.k_data_memory_bytes[AIE_VARIANT]
    TP_DIM_ROWS_max = int(
        math.floor((BUFFER_SIZE * TP_CASC_LEN_max) / (com.fn_size_by_byte(TT_DATA) * TP_DIM_COLS))
    )

    param_dict = {}
    param_dict.update({"name": "TP_DIM_ROWS"})
    param_dict.update({"minimum": TP_DIM_ROWS_min})
    param_dict.update({"maximum": TP_DIM_ROWS_max})

    if TP_DIM_ROWS != 0:
        # Round to nearest multiple of vector size
        if (TP_DIM_ROWS % TP_DIM_ROWS_min) != 0:
            TP_DIM_ROWS = int(com.CEIL(TP_DIM_ROWS, TP_DIM_ROWS_min))

        # Ensure the value has at least one valid TP_CASC_LEN; if not, search nearby
        if not fn_has_valid_casc_len(AIE_VARIANT, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS):
            TP_DIM_ROWS = fn_find_nearest_valid_dim_rows(
                AIE_VARIANT, TT_DATA, TP_DIM_COLS, TP_DIM_ROWS, TP_DIM_ROWS_min, TP_DIM_ROWS_max
            )

        param_dict.update({"actual": TP_DIM_ROWS})

    return param_dict


def fn_find_nearest_valid_dim_rows(AIE_VARIANT, TT_DATA, TP_DIM_COLS, TP_DIM_ROWS, TP_DIM_ROWS_min, TP_DIM_ROWS_max):
    """Search for the nearest TP_DIM_ROWS value (up then down) that has a valid TP_CASC_LEN."""
    step = TP_DIM_ROWS_min
    # Search upward and downward alternately
    for offset in range(step, TP_DIM_ROWS_max, step):
        candidate_up = TP_DIM_ROWS + offset
        if candidate_up <= TP_DIM_ROWS_max and fn_has_valid_casc_len(AIE_VARIANT, TT_DATA, candidate_up, TP_DIM_COLS):
            return candidate_up
        candidate_down = TP_DIM_ROWS - offset
        if candidate_down >= TP_DIM_ROWS_min and fn_has_valid_casc_len(AIE_VARIANT, TT_DATA, candidate_down, TP_DIM_COLS):
            return candidate_down
    # Fallback to minimum
    return TP_DIM_ROWS_min


def fn_has_valid_casc_len(AIE_VARIANT, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS):
    """Check if the given TP_DIM_ROWS has at least one valid TP_CASC_LEN
    for the given TP_DIM_COLS."""
    BUFFER_SIZE = com.k_data_memory_bytes[AIE_VARIANT]
    total_matrix_bytes = TP_DIM_ROWS * TP_DIM_COLS * com.fn_size_by_byte(TT_DATA)
    vec_size = fn_kernel_vec_size(TT_DATA)

    casc_len_min = max(TP_CASC_LEN_min, int(math.ceil(total_matrix_bytes / BUFFER_SIZE)))
    casc_len_max = min(TP_DIM_ROWS, TP_CASC_LEN_max)

    for c in range(casc_len_min, casc_len_max + 1):
        if (
            TP_DIM_ROWS % c == 0
            and (TP_DIM_ROWS // c) % vec_size == 0
            and fn_estimated_kernel_data_bytes(TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, c) <= BUFFER_SIZE
        ):
            return True
    return False


def validate_TP_DIM_ROWS(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM_COLS = int(args["TP_DIM_COLS"]) if "TP_DIM_COLS" in args and args["TP_DIM_COLS"] else 2
    TP_DIM_ROWS = int(args["TP_DIM_ROWS"])

    param_dict = update_TP_DIM_ROWS(args)
    range_TP_DIM_ROWS = [param_dict["minimum"], param_dict["maximum"]]

    if TP_DIM_ROWS % param_dict["minimum"] != 0:
        return com.isError(f"TP_DIM_ROWS must be a multiple of {param_dict['minimum']}.")

    range_check = com.validate_range(range_TP_DIM_ROWS, "TP_DIM_ROWS", TP_DIM_ROWS)
    if range_check["is_valid"] == False:
        return range_check

    if not fn_has_valid_casc_len(AIE_VARIANT, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS):
        return com.isError(
            f"TP_DIM_ROWS = {TP_DIM_ROWS} has no valid TP_CASC_LEN for TP_DIM_COLS = {TP_DIM_COLS}. "
            f"TP_DIM_ROWS must be divisible by some cascade length in [1, {TP_CASC_LEN_max}] "
            f"such that (TP_DIM_ROWS / TP_CASC_LEN) is a multiple of the vector size."
        )

    return com.isValid


#######################################################
########### TP_CASC_LEN Updater and Validator #########
#######################################################
    # DIM_ROWS must split evenly into casc_len. Min casc len must satisfy
    # rows * cols / casc_len <= buffer size. DIM_ROWS / CASC_LEN must be a
    # multiple of the kernel's internal vector size.
def update_TP_CASC_LEN(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM_ROWS = int(args["TP_DIM_ROWS"])
    TP_DIM_COLS = int(args["TP_DIM_COLS"])

    if "TP_CASC_LEN" in args and args["TP_CASC_LEN"]:
        TP_CASC_LEN = int(args["TP_CASC_LEN"])
    else:
        TP_CASC_LEN = 1

    return fn_update_tp_casc_len(AIE_VARIANT, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_CASC_LEN)


def fn_update_tp_casc_len(AIE_VARIANT, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_CASC_LEN):
    BUFFER_SIZE = com.k_data_memory_bytes[AIE_VARIANT]
    total_matrix_bytes = TP_DIM_ROWS * TP_DIM_COLS * com.fn_size_by_byte(TT_DATA)
    vec_size = fn_kernel_vec_size(TT_DATA)

    # Max: can't exceed TP_DIM_ROWS (rows must split evenly) or global max
    casc_len_max = min(TP_DIM_ROWS, TP_CASC_LEN_max)

    # Build legal set: TP_DIM_ROWS must split evenly AND rows_per_kernel must be a multiple of vector size
    legal_set = [
        c
        for c in range(TP_CASC_LEN_min, casc_len_max + 1)
        if TP_DIM_ROWS % c == 0
        and (TP_DIM_ROWS // c) % vec_size == 0
        and fn_estimated_kernel_data_bytes(TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, c) <= BUFFER_SIZE
    ]

    param_dict = {}
    param_dict.update({"name": "TP_CASC_LEN"})
    param_dict.update({"enum": legal_set})

    # Snap to nearest valid value for the QOR framework's fallback path
    if legal_set:
        if TP_CASC_LEN in legal_set:
            param_dict.update({"actual": TP_CASC_LEN})
        else:
            nearest = min(legal_set, key=lambda x: abs(x - TP_CASC_LEN))
            param_dict.update({"actual": nearest})

    return param_dict


def validate_TP_CASC_LEN(args):
    TP_CASC_LEN = int(args["TP_CASC_LEN"])
    param_dict = update_TP_CASC_LEN(args)
    legal_set_TP_CASC_LEN = param_dict["enum"]
    return com.validate_legal_set(legal_set_TP_CASC_LEN, "TP_CASC_LEN", TP_CASC_LEN)


#######################################################
########### TP_PASSES Updater and Validator ###########
#######################################################
def update_TP_PASSES(args):
    if "TP_PASSES" in args and args["TP_PASSES"]:
        TP_PASSES = args["TP_PASSES"]
    else:
        TP_PASSES = 1

    return fn_update_tp_passes(TP_PASSES)


def fn_update_tp_passes(TP_PASSES):
    TP_PASSES_min = 1
    TP_PASSES_max = 10

    param_dict = {}
    param_dict.update({"name": "TP_PASSES"})
    param_dict.update({"minimum": TP_PASSES_min})
    param_dict.update({"maximum": TP_PASSES_max})
    param_dict.update({"enum": list(range(TP_PASSES_min, TP_PASSES_max + 1))})

    # Clamp value if out of range
    if TP_PASSES != 0:
        if TP_PASSES < TP_PASSES_min:
            param_dict.update({"actual": TP_PASSES_min})
        elif TP_PASSES > TP_PASSES_max:
            param_dict.update({"actual": TP_PASSES_max})

    return param_dict


def validate_TP_PASSES(args):
    TP_PASSES = args["TP_PASSES"]
    param_dict = update_TP_PASSES(args)
    range_TP_PASSES = [param_dict["minimum"], param_dict["maximum"]]
    return com.validate_range(range_TP_PASSES, "TP_PASSES", TP_PASSES)


def info_ports(args):
    """Standard function creating a static dictionary of information
    for upper software to correctly connect the IP.
    Some IP has a configurable number of ports according to parameter set,
    so port information has to be implemented as a function"""
    TT_DATA = args["TT_DATA"]
    TP_DIM_ROWS = args["TP_DIM_ROWS"]
    TP_DIM_COLS = args["TP_DIM_COLS"]
    TP_CASC_LEN = args["TP_CASC_LEN"]

    vec_size = fn_kernel_vec_size(TT_DATA)
    cols_sched = TP_DIM_COLS + (TP_DIM_COLS % 2)
    cols_padded = int(com.CEIL(cols_sched, vec_size))

    TP_WINDOW_VSIZE_A = (TP_DIM_ROWS * TP_DIM_COLS) // TP_CASC_LEN
    TP_WINDOW_VSIZE_U = (TP_DIM_ROWS * cols_padded) // TP_CASC_LEN
    TP_WINDOW_VSIZE_S = cols_padded
    TP_WINDOW_VSIZE_V = cols_padded * cols_padded

    portsInA = com.get_port_info("in", "in", TT_DATA, TP_WINDOW_VSIZE_A, TP_CASC_LEN, 0, com.API_BUFFER)
    portsOutU = com.get_port_info("outU", "out", TT_DATA, TP_WINDOW_VSIZE_U, TP_CASC_LEN, 0, com.API_BUFFER)
    portsOutS = com.get_port_info("outS", "out", TT_DATA, TP_WINDOW_VSIZE_S, TP_CASC_LEN, 0, com.API_BUFFER)
    portsOutV = com.get_port_info("outV", "out", TT_DATA, TP_WINDOW_VSIZE_V, TP_CASC_LEN, 0, com.API_BUFFER)

    return portsInA + portsOutU + portsOutS + portsOutV


#### graph generator ####
def generate_graph(graphname, args):

    if graphname == "":
        graphname = "default_graphname"

    TT_DATA = args["TT_DATA"]
    TP_DIM_ROWS = args["TP_DIM_ROWS"]
    TP_DIM_COLS = args["TP_DIM_COLS"]
    TP_PASSES = args["TP_PASSES"]
    TP_CASC_LEN = args["TP_CASC_LEN"]

    # Use formatted multi-line string to avoid a lot of \n and \t
    code = f"""
class {graphname} : public adf::graph {{
public:
  static constexpr unsigned int TP_CASC_LEN = {TP_CASC_LEN};
  template <typename dir>
  using casc_port_array = std::array<adf::port<dir>, TP_CASC_LEN>;

  casc_port_array<input> in;
  casc_port_array<output> outU;
  casc_port_array<output> outS;
  casc_port_array<output> outV;

  xf::solver::aie::svd::svd_graph<
    {TT_DATA}, //TT_DATA
    {TP_DIM_ROWS}, //TP_DIM_ROWS
    {TP_DIM_COLS}, //TP_DIM_COLS
    {TP_PASSES}, //TP_PASSES
    {TP_CASC_LEN} //TP_CASC_LEN
  > svd;

  {graphname}() : svd() {{
    adf::kernel *svd_kernels = svd.getKernels();

    for (int i=0; i < TP_CASC_LEN; i++) {{
      adf::connect<> net_in(in[i], svd.in[i]);
      adf::connect<> net_outU(svd.outU[i], outU[i]);
      adf::connect<> net_outS(svd.outS[i], outS[i]);
      adf::connect<> net_outV(svd.outV[i], outV[i]);
    }}
  }}

}};
"""
    out = {}
    out["graph"] = code
    out["port_info"] = info_ports(args)
    out["headerfile"] = "svd_graph.hpp"
    out["searchpaths"] = [
        "L2/include/aie",
        "L2/tests/aie/common/inc",
        "L1/include/aie",
        "L1/src/aie",
        "L1/tests/aie/inc",
        "L1/tests/aie/src",
    ]

    return out
