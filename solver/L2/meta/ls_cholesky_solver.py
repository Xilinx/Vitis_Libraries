#
# Copyright (C) 2026, Advanced Micro Devices, Inc.
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
import cholesky
import substitution
from math import sqrt

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

# Example of validator.
#
# The parameter itself will be passed as first argument for validator functions.
# These functions can have extra parameters as arguments, as specified as last part of in `validator`.
# These extra parameters must appear before current one in "parameters" section.
#
# A validator function returns a dictionary, with required boolean key "is_valid",
# and "err_message" if "is_valid" is False.

# ls_cholesky_solver_graph.hpp instantiates:
#   cholesky_graph<TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_GRID_DIM, TP_CASC_LEN, TP_DIAG_INV=1>
#   substitution_graph<TT_DATA, TP_DIM, TP_SUBST_TYPE=0 (fwd), TP_L_LEADING=1, TP_GRID_DIM, TP_NUM_FRAMES, TP_DIAG_INV=1>
#   substitution_graph<TT_DATA, TP_DIM, TP_SUBST_TYPE=1 (bwd), TP_L_LEADING=1, TP_GRID_DIM, TP_NUM_FRAMES, TP_DIAG_INV=1>
#
# TT_DATA/TP_DIM/TP_GRID_DIM/TP_NUM_FRAMES are passed straight through from
# this IP's own parameters (same names in cholesky.py/substitution.py).
# _cholesky_args/_fwd_subs_args/_bwd_subs_args build the args dict each
# sub-graph would see (pinning the hardcoded template arguments), and each
# parameter below combines this IP's OWN requirement with what the
# cholesky.py and (fwd + bwd) substitution.py updaters allow. The legal set
# or range of the parameter is the intersection of all of them.
# TP_CASC_LEN applies only to the internal Cholesky stage (substitution has
# no such parameter).

TP_DIM_max      = 1024
# Only TP_GRID_DIM = 1 is supported 
# and TP_CASC_LEN up to 32 is allowed for GRID_DIM=1.
TP_GRID_DIM_max = 1
TP_CASC_LEN_max = 32

# AIE1 + cfloat: With a small matrix and a deep Cholesky cascade, require:
TP_DIM_aie1_cfloat_deep_casc_min      = 32
TP_CASC_LEN_aie1_cfloat_small_dim_max = 2


def _cholesky_args(args):
    """args dict for the internal cholesky_graph, as instantiated in
    ls_cholesky_solver_graph.hpp: cholesky_graph<..., TP_DIAG_INV=1>."""
    a = dict(args)
    a["TP_DIAG_INV"] = 1
    return a


def _fwd_subs_args(args):
    """args dict for the internal forward substitution_graph, as instantiated in
    ls_cholesky_solver_graph.hpp:
        substitution_graph<..., TP_SUBST_TYPE=0, TP_L_LEADING=1, ..., TP_DIAG_INV=1>."""
    a = dict(args)
    a["TP_SUBST_TYPE"] = 0
    a["TP_L_LEADING"] = 1
    a["TP_DIAG_INV"] = 1
    return a


def _bwd_subs_args(args):
    """args dict for the internal backward substitution_graph, as instantiated in
    ls_cholesky_solver_graph.hpp:
        substitution_graph<..., TP_SUBST_TYPE=1, TP_L_LEADING=1, ..., TP_DIAG_INV=1>."""
    a = dict(args)
    a["TP_SUBST_TYPE"] = 1
    a["TP_L_LEADING"] = 1
    a["TP_DIAG_INV"] = 1
    return a


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
    return fn_validate_AIE_VARIANT(AIE_VARIANT)

def fn_validate_AIE_VARIANT(AIE_VARIANT):
    param_dict = fn_update_AIE_VARIANT()
    legal_set_AIE_VARIANT = param_dict["enum"]
    return com.validate_legal_set(legal_set_AIE_VARIANT, "AIE_VARIANT", AIE_VARIANT)


#######################################################
########### TT_DATA Updater and Validator #############
#######################################################
def update_TT_DATA(args):
    return fn_update_data_type(args)

def fn_update_data_type(args):
    legal_set_TT_DATA = ["float", "cfloat"]
    chol_args = _cholesky_args(args)
    chol_legal_set_TT_DATA = cholesky.update_TT_DATA(chol_args)["enum"]
    fwd_subs_args = _fwd_subs_args(args)
    fwd_subs_legal_set_TT_DATA = substitution.update_TT_DATA(fwd_subs_args)["enum"]
    bwd_subs_args = _bwd_subs_args(args)
    bwd_subs_legal_set_TT_DATA = substitution.update_TT_DATA(bwd_subs_args)["enum"]

    param_dict = {}
    param_dict.update({"name": "TT_DATA"})
    all_legal_sets_TT_DATA = [
        t for t in legal_set_TT_DATA
        if t in chol_legal_set_TT_DATA
        and t in fwd_subs_legal_set_TT_DATA
        and t in bwd_subs_legal_set_TT_DATA
    ]
    param_dict.update({"enum": all_legal_sets_TT_DATA})
    return param_dict

def validate_TT_DATA(args):
    TT_DATA = args["TT_DATA"]
    param_dict = update_TT_DATA(args)
    all_legal_sets_TT_DATA = param_dict["enum"]
    return com.validate_all_legal_sets([all_legal_sets_TT_DATA], "TT_DATA", TT_DATA)


#######################################################
######### TP_GRID_DIM Updater and Validator ###########
#######################################################
def update_TP_GRID_DIM(args):
    return fn_update_grid_dim(args)

def fn_update_grid_dim(args):
    TP_GRID_DIM = args["TP_GRID_DIM"] if "TP_GRID_DIM" in args and args["TP_GRID_DIM"] else 0
    chol_args = _cholesky_args(args)
    chol_param_dict = cholesky.update_TP_GRID_DIM(chol_args)
    chol_TP_GRID_DIM_min = chol_param_dict["minimum"]
    chol_TP_GRID_DIM_max = chol_param_dict["maximum"]
    fwd_subs_args = _fwd_subs_args(args)
    fwd_subs_param_dict = substitution.update_TP_GRID_DIM(fwd_subs_args)
    fwd_subs_TP_GRID_DIM_min = fwd_subs_param_dict["minimum"]
    fwd_subs_TP_GRID_DIM_max = fwd_subs_param_dict["maximum"]
    bwd_subs_args = _bwd_subs_args(args)
    bwd_subs_param_dict = substitution.update_TP_GRID_DIM(bwd_subs_args)
    bwd_subs_TP_GRID_DIM_min = bwd_subs_param_dict["minimum"]
    bwd_subs_TP_GRID_DIM_max = bwd_subs_param_dict["maximum"]

    # This IP's own range
    TP_GRID_DIM_min = 1

    # Intersection of all ranges
    all_legal_ranges_TP_GRID_DIM = [
        [chol_TP_GRID_DIM_min, chol_TP_GRID_DIM_max],
        [fwd_subs_TP_GRID_DIM_min, fwd_subs_TP_GRID_DIM_max],
        [bwd_subs_TP_GRID_DIM_min, bwd_subs_TP_GRID_DIM_max],
        [TP_GRID_DIM_min, TP_GRID_DIM_max],
    ]
    all_TP_GRID_DIM_min = max(r[0] for r in all_legal_ranges_TP_GRID_DIM)
    all_TP_GRID_DIM_max = min(r[1] for r in all_legal_ranges_TP_GRID_DIM)

    param_dict = {}
    param_dict.update({"name": "TP_GRID_DIM"})
    param_dict.update({"minimum": all_TP_GRID_DIM_min})
    param_dict.update({"maximum": all_TP_GRID_DIM_max})
    param_dict.update({"actual": com.CLIP(TP_GRID_DIM, all_TP_GRID_DIM_min, all_TP_GRID_DIM_max)})
    return param_dict

def validate_TP_GRID_DIM(args):
    TP_GRID_DIM = args["TP_GRID_DIM"]
    param_dict = update_TP_GRID_DIM(args)
    all_legal_ranges_TP_GRID_DIM = [[param_dict["minimum"], param_dict["maximum"]]]
    return com.validate_all_ranges(all_legal_ranges_TP_GRID_DIM, "TP_GRID_DIM", TP_GRID_DIM)


#######################################################
############ TP_DIM Updater and Validator #############
#######################################################
def update_TP_DIM(args):
    return fn_update_TP_DIM(args)

def fn_update_TP_DIM(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_GRID_DIM = args["TP_GRID_DIM"]
    TP_DIM = args["TP_DIM"] if "TP_DIM" in args and args["TP_DIM"] else 0
    chol_args = _cholesky_args(args)
    chol_param_dict = cholesky.update_TP_DIM(chol_args)
    chol_TP_DIM_min = chol_param_dict["minimum"]
    chol_TP_DIM_max = chol_param_dict["maximum"]
    fwd_subs_args = _fwd_subs_args(args)
    fwd_subs_param_dict = substitution.update_TP_DIM(fwd_subs_args)
    fwd_subs_TP_DIM_min = fwd_subs_param_dict["minimum"]
    fwd_subs_TP_DIM_max = fwd_subs_param_dict["maximum"]
    bwd_subs_args = _bwd_subs_args(args)
    bwd_subs_param_dict = substitution.update_TP_DIM(bwd_subs_args)
    bwd_subs_TP_DIM_min = bwd_subs_param_dict["minimum"]
    bwd_subs_TP_DIM_max = bwd_subs_param_dict["maximum"]

    # This IP's own range
    kVecSampleNum = com.fnVecSampleNumMax(AIE_VARIANT, TT_DATA)
    io_samples_max_per_tile = com.k_data_memory_bytes[AIE_VARIANT] // com.sizeof(TT_DATA)
    max_dim_per_tile = com.FLOOR(int(sqrt(io_samples_max_per_tile)), kVecSampleNum)
    io_dim_min = TP_GRID_DIM * kVecSampleNum
    io_dim_max = TP_GRID_DIM * max_dim_per_tile

    # Intersection of all ranges
    all_legal_ranges_TP_DIM = [
        [chol_TP_DIM_min, chol_TP_DIM_max],
        [fwd_subs_TP_DIM_min, fwd_subs_TP_DIM_max],
        [bwd_subs_TP_DIM_min, bwd_subs_TP_DIM_max],
        [io_dim_min, min(io_dim_max, TP_DIM_max)],
    ]
    all_TP_DIM_min = max(r[0] for r in all_legal_ranges_TP_DIM)
    all_TP_DIM_max = min(r[1] for r in all_legal_ranges_TP_DIM)

    param_dict = {}
    param_dict.update({"name": "TP_DIM"})
    param_dict.update({"minimum": all_TP_DIM_min})
    param_dict.update({"maximum": all_TP_DIM_max})

    TP_DIM = com.CLIP(TP_DIM, all_TP_DIM_min, all_TP_DIM_max)
    TP_DIM = com.ROUND_TO_NEAREST_MULTIPLE(TP_DIM, io_dim_min)
    param_dict.update({"actual": TP_DIM})
    return param_dict

def validate_TP_DIM(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_GRID_DIM = args["TP_GRID_DIM"]
    TP_DIM = args["TP_DIM"]
    chol_args = _cholesky_args(args)
    fwd_subs_args = _fwd_subs_args(args)
    bwd_subs_args = _bwd_subs_args(args)

    # This IP's own requirements. These also guard the sub-graph updaters below:
    # cholesky.update_TP_DIM divides by a vector width that is False when
    # TP_DIM / TP_GRID_DIM is not a multiple of any vector width.
    if TP_DIM % TP_GRID_DIM != 0:
        return com.isError("TP_DIM must be a multiple of TP_GRID_DIM.")
    kVecSampleNum = com.fnVecSampleNumMax(AIE_VARIANT, TT_DATA)
    kKernelDim = TP_DIM // TP_GRID_DIM
    if kKernelDim % kVecSampleNum != 0:
        return com.isError("TP_DIM / TP_GRID_DIM should be a multiple of vecSampleNum.")

    param_dict = update_TP_DIM(args)
    all_legal_ranges_TP_DIM = [[param_dict["minimum"], param_dict["maximum"]]]
    all_ranges_check = com.validate_all_ranges(all_legal_ranges_TP_DIM, "TP_DIM", TP_DIM)
    if not all_ranges_check["is_valid"]:
        return all_ranges_check

    # Each sub-graph has a constraint on TP_DIM.
    chol_check = cholesky.validate_TP_DIM(chol_args)
    if not chol_check["is_valid"]:
        return chol_check
    fwd_subs_check = substitution.validate_TP_DIM(fwd_subs_args)
    if not fwd_subs_check["is_valid"]:
        return fwd_subs_check
    bwd_subs_check = substitution.validate_TP_DIM(bwd_subs_args)
    if not bwd_subs_check["is_valid"]:
        return bwd_subs_check

    return com.isValid


#######################################################
####### TP_NUM_FRAMES Updater and Validator ###########
#######################################################
def update_TP_NUM_FRAMES(args):
    return fn_update_TP_NUM_FRAMES(args)

def fn_update_TP_NUM_FRAMES(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_GRID_DIM = args["TP_GRID_DIM"]
    TP_DIM = args["TP_DIM"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"] if "TP_NUM_FRAMES" in args and args["TP_NUM_FRAMES"] else 0
    chol_args = _cholesky_args(args)
    chol_param_dict = cholesky.update_TP_NUM_FRAMES(chol_args)
    chol_TP_NUM_FRAMES_min = chol_param_dict["minimum"]
    chol_TP_NUM_FRAMES_max = chol_param_dict["maximum"]
    fwd_subs_args = _fwd_subs_args(args)
    fwd_subs_param_dict = substitution.update_TP_NUM_FRAMES(fwd_subs_args)
    fwd_subs_TP_NUM_FRAMES_min = fwd_subs_param_dict["minimum"]
    fwd_subs_TP_NUM_FRAMES_max = fwd_subs_param_dict["maximum"]
    bwd_subs_args = _bwd_subs_args(args)
    bwd_subs_param_dict = substitution.update_TP_NUM_FRAMES(bwd_subs_args)
    bwd_subs_TP_NUM_FRAMES_min = bwd_subs_param_dict["minimum"]
    bwd_subs_TP_NUM_FRAMES_max = bwd_subs_param_dict["maximum"]

    # This IP's own range
    MAX_FRAMES = 4  # substitution buffer-descriptor limit
    # AIE1 + cfloat: complex consumes one dimension, so NUM_FRAMES must be 1.
    if AIE_VARIANT == com.AIE and TT_DATA == "cfloat":
        MAX_FRAMES = 1
    kKernelDim = TP_DIM // TP_GRID_DIM
    frame_memory_per_kernel = kKernelDim * kKernelDim * com.sizeof(TT_DATA)
    max_frames_memory = com.k_data_memory_bytes[AIE_VARIANT] // frame_memory_per_kernel

    # Intersection of all ranges
    all_legal_ranges_TP_NUM_FRAMES = [
        [chol_TP_NUM_FRAMES_min, chol_TP_NUM_FRAMES_max],
        [fwd_subs_TP_NUM_FRAMES_min, fwd_subs_TP_NUM_FRAMES_max],
        [bwd_subs_TP_NUM_FRAMES_min, bwd_subs_TP_NUM_FRAMES_max],
        [1, min(MAX_FRAMES, max_frames_memory)],
    ]
    all_TP_NUM_FRAMES_min = max(r[0] for r in all_legal_ranges_TP_NUM_FRAMES)
    all_TP_NUM_FRAMES_max = min(r[1] for r in all_legal_ranges_TP_NUM_FRAMES)

    param_dict = {}
    param_dict.update({"name": "TP_NUM_FRAMES"})
    param_dict.update({"minimum": all_TP_NUM_FRAMES_min})
    param_dict.update({"maximum": all_TP_NUM_FRAMES_max})
    param_dict.update({"actual": com.CLIP(TP_NUM_FRAMES, all_TP_NUM_FRAMES_min, all_TP_NUM_FRAMES_max)})
    return param_dict

def validate_TP_NUM_FRAMES(args):
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    param_dict = update_TP_NUM_FRAMES(args)
    all_legal_ranges_TP_NUM_FRAMES = [[param_dict["minimum"], param_dict["maximum"]]]
    return com.validate_all_ranges(all_legal_ranges_TP_NUM_FRAMES, "TP_NUM_FRAMES", TP_NUM_FRAMES)


#######################################################
######### TP_CASC_LEN Updater and Validator ###########
#######################################################
# TP_CASC_LEN applies only to the internal Cholesky stage; substitution has
# no such parameter. This IP's own requirement (TP_CASC_LEN_max) is
# intersected with cholesky.py's own.
def update_TP_CASC_LEN(args):
    return fn_update_casc_len(args)

def fn_update_casc_len(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM = args["TP_DIM"]
    TP_CASC_LEN = args["TP_CASC_LEN"] if "TP_CASC_LEN" in args and args["TP_CASC_LEN"] else 0
    chol_args = _cholesky_args(args)
    chol_param_dict = cholesky.update_TP_CASC_LEN(chol_args)
    chol_TP_CASC_LEN_min = chol_param_dict["minimum"]
    chol_TP_CASC_LEN_max = chol_param_dict["maximum"]
    fwd_subs_args = _fwd_subs_args(args)  # not needed here but included for consistency
    bwd_subs_args = _bwd_subs_args(args)  # not needed here but included for consistency

    # Intersection of all ranges
    all_legal_ranges_TP_CASC_LEN = [
        [chol_TP_CASC_LEN_min, chol_TP_CASC_LEN_max],
        [1, min(TP_CASC_LEN_max, TP_DIM)],
    ]
    # AIE1 + cfloat with a small matrix: deeper cascades are not supported.
    if AIE_VARIANT == com.AIE and TT_DATA == "cfloat" and TP_DIM < TP_DIM_aie1_cfloat_deep_casc_min:
        all_legal_ranges_TP_CASC_LEN.append([1, TP_CASC_LEN_aie1_cfloat_small_dim_max])
    all_TP_CASC_LEN_min = max(r[0] for r in all_legal_ranges_TP_CASC_LEN)
    all_TP_CASC_LEN_max = min(r[1] for r in all_legal_ranges_TP_CASC_LEN)

    param_dict = {}
    param_dict.update({"name": "TP_CASC_LEN"})
    param_dict.update({"minimum": all_TP_CASC_LEN_min})
    param_dict.update({"maximum": all_TP_CASC_LEN_max})
    param_dict.update({"actual": com.CLIP(TP_CASC_LEN, all_TP_CASC_LEN_min, all_TP_CASC_LEN_max)})
    return param_dict

def validate_TP_CASC_LEN(args):
    TP_CASC_LEN = args["TP_CASC_LEN"]
    param_dict = update_TP_CASC_LEN(args)
    all_legal_ranges_TP_CASC_LEN = [[param_dict["minimum"], param_dict["maximum"]]]
    return com.validate_all_ranges(all_legal_ranges_TP_CASC_LEN, "TP_CASC_LEN", TP_CASC_LEN)


# #### port info ####
def get_port_info(portname, dir, dataType, windowVsize, apiType, idx=0):
    windowSize = windowVsize * com.fn_size_by_byte(dataType)
    return [
        {
            "name": f"{portname}[{idx}]",
            "type": f"{apiType}",
            "direction": f"{dir}",
            "data_type": dataType,
            "fn_is_complex": com.fn_is_complex(dataType),
            "window_size": windowSize,
            "margin_size": 0,
        }
    ]

def info_ports(args):
    """Standard function creating a static dictionary of information
    for upper software to correctly connect the IP.

    Ports:
      A_in[0..kNumKernels-1]  : input SPD matrix tiles (kKernelDim x kKernelDim each)
      y_in[0..kNumKernels-1]  : input vector slices    (kKernelDim each)
      x_out[0..TP_GRID_DIM-1] : output vector slices   (kKernelDim each)
    where kNumKernels = TP_GRID_DIM*(TP_GRID_DIM+1)//2
    and   kKernelDim  = TP_DIM // TP_GRID_DIM.
    """
    TT_DATA     = args["TT_DATA"]
    TP_DIM      = args["TP_DIM"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_GRID_DIM = args["TP_GRID_DIM"]

    kNumKernels = TP_GRID_DIM * (TP_GRID_DIM + 1) // 2
    kKernelDim  = TP_DIM // TP_GRID_DIM

    portsInA  = []
    portsInY  = []
    portsOutX = []

    for kernel in range(kNumKernels):
        # A_in: lower-triangular sub-matrix tiles (square, kKernelDim x kKernelDim)
        portsInA += get_port_info(
            portname="A_in",
            dir="in",
            dataType=TT_DATA,
            windowVsize=kKernelDim * kKernelDim * TP_NUM_FRAMES,
            apiType="window",
            idx=kernel,
        )
        # y_in: vector slices (kKernelDim elements each)
        portsInY += get_port_info(
            portname="y_in",
            dir="in",
            dataType=TT_DATA,
            windowVsize=kKernelDim * TP_NUM_FRAMES,
            apiType="window",
            idx=kernel,
        )

    for tile in range(TP_GRID_DIM):
        # x_out: solution vector slices (kKernelDim elements each)
        portsOutX += get_port_info(
            portname="x_out",
            dir="out",
            dataType=TT_DATA,
            windowVsize=kKernelDim * TP_NUM_FRAMES,
            apiType="window",
            idx=tile,
        )

    return portsInA + portsInY + portsOutX


#### graph generator ####
def generate_graph(graphname, args):

    if graphname == "":
        graphname = "default_graphname"

    TT_DATA       = args["TT_DATA"]
    TP_DIM        = args["TP_DIM"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_GRID_DIM   = args["TP_GRID_DIM"]
    TP_CASC_LEN   = args["TP_CASC_LEN"]

    kNumKernels = TP_GRID_DIM * (TP_GRID_DIM + 1) // 2

    code = f"""
class {graphname} : public adf::graph {{
public:
  std::array<adf::port<input>,  {kNumKernels}> A_in;
  std::array<adf::port<input>,  {kNumKernels}> y_in;
  std::array<adf::port<output>, {TP_GRID_DIM}> x_out;

  xf::solver::aie::ls_cholesky_solver::ls_cholesky_solver_graph<
    {TT_DATA},       // TT_DATA
    {TP_DIM},        // TP_DIM
    {TP_NUM_FRAMES}, // TP_NUM_FRAMES
    {TP_GRID_DIM},   // TP_GRID_DIM
    {TP_CASC_LEN}    // TP_CASC_LEN
  > ls_cholesky_solver;

  {graphname}() : ls_cholesky_solver() {{
    for (int i = 0; i < {kNumKernels}; i++) {{
      adf::connect<> net_A_in(A_in[i], ls_cholesky_solver.A_in[i]);
      adf::connect<> net_y_in(y_in[i], ls_cholesky_solver.y_in[i]);
    }}
    for (int i = 0; i < {TP_GRID_DIM}; i++) {{
      adf::connect<> net_x_out(ls_cholesky_solver.x_out[i], x_out[i]);
    }}
  }}
}};
"""
    out = {}
    out["graph"] = code
    out["port_info"] = info_ports(args)
    out["headerfile"] = "ls_cholesky_solver_graph.hpp"
    out["searchpaths"] = [
        "L2/include/aie",
        "L2/tests/aie/common/inc",
        "L1/include/aie",
        "L1/src/aie",
        "L1/tests/aie/inc",
        "L1/tests/aie/src",
    ]

    return out
