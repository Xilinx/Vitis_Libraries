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
"""
Metadata module for the eigenvalue IP core.

Provides parameter validators and updaters for the eigenvalue_graph ADF
graph. The graph computes the eigenvalues of a square matrix A via:

    1. Hessenberg reduction (Householder reflections):  A -> H
    2. Iterative QR with shift and deflation:            H -> E (eigenvalues)

Naming conventions (from mp_pseudoinverse.py / svd.py pattern):
  validate_  — boolean/error validator returning {"is_valid": bool, ...}
  update_    — updater returning param_dict with legal set or range
  info_      — creates port/connection information
  fn_        — internal helper
"""
import sys, os
from math import sqrt
import aie_common as com
from aie_common import *

script_directory = os.path.dirname(os.path.abspath(__file__))
L2path = script_directory + "/.."
include_path = L2path + "/tests/aie/common/scripts"
sys.path.insert(0, include_path)

# -----------------------------------------------------------------------
# Cascade length constants — eigenvalue only supports CASC_LEN = 1.
# Parallelism across multiple kernels is reserved for a future release.
# -----------------------------------------------------------------------
TP_CASC_LEN_min = 1
TP_CASC_LEN_max = 1

# -----------------------------------------------------------------------
# TP_PASSES range.
# -----------------------------------------------------------------------
TP_PASSES_min = 1
TP_PASSES_max = 100
TP_PASSES_default = 20


def fn_kernel_vec_size(TT_DATA):
    """Return the internal vector lane count used by the hessenberg/eig_qr
    kernels. Fixed at the 256-bit width (8 float lanes, 4 cfloat lanes)
    regardless of AIE variant — see fnVecSampleNumIO() in eigenvalue_traits.hpp.
    """
    return 4 if TT_DATA == "cfloat" else 8


# eig_qr (eig_qr.hpp) is the tighter of the two kernel tiles: it keeps 3
# concurrent TP_DIM x TP_DIM buffers resident (the working matrix "A" --
# aliased to its input window -- plus "Q" and "R", see eig_qr.hpp:112-113 /
# eigenvalue_graph.hpp:162-163), versus hessenberg's 2 (input + output
# window). Since both kernels must run at the same TP_DIM, eig_qr's buffer
# count is what governs the achievable maximum.
K_EIG_QR_NUM_TP_DIM_SQUARED_BUFFERS = 3


def fn_max_dim(TT_DATA, AIE_VARIANT):
    """Maximum supported matrix size, driven by per-tile data memory
    (com.k_data_memory_bytes in aie_common.py -- the size of a single AIE
    memory bank/module). Per this repo's single_buffer convention (see e.g.
    svd_graph.hpp), TP_DIM is sized as though every buffer is single-buffered
    (not ping-ponged); ping-ponging (if applied) is a separate placement
    concern handled in the graph once TP_DIM is fixed, not part of this
    limit. eig_qr's 3 concurrent TP_DIM x TP_DIM buffers (K_EIG_QR_NUM_
    TP_DIM_SQUARED_BUFFERS) must together fit within one bank:

        3 * TP_DIM^2 * sizeof(TT_DATA) <= k_data_memory_bytes[AIE_VARIANT]

    so max_dim = floor(sqrt(MEM_PER_BANK / (3 * sizeof(TT_DATA)))), rounded
    down to a multiple of the kernel's internal vector size."""
    vec_size = fn_kernel_vec_size(TT_DATA)
    max_samples_per_bank = com.k_data_memory_bytes[AIE_VARIANT] // (
        K_EIG_QR_NUM_TP_DIM_SQUARED_BUFFERS * com.sizeof(TT_DATA)
    )
    return com.FLOOR(int(sqrt(max_samples_per_bank)), vec_size)


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
    return com.validate_legal_set(param_dict["enum"], "AIE_VARIANT", AIE_VARIANT)


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
    return com.validate_legal_set(param_dict["enum"], "TT_DATA", TT_DATA)


#######################################################
########### TP_DIM Updater and Validator ###############
#######################################################
def update_TP_DIM(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    if "TP_DIM" in args and args["TP_DIM"]:
        TP_DIM = int(args["TP_DIM"])
    else:
        TP_DIM = 0
    return fn_update_tp_dim(AIE_VARIANT, TT_DATA, TP_DIM)


def fn_update_tp_dim(AIE_VARIANT, TT_DATA, TP_DIM):
    vec_size = fn_kernel_vec_size(TT_DATA)
    max_dim = fn_max_dim(TT_DATA, AIE_VARIANT)

    param_dict = {}
    param_dict.update({"name": "TP_DIM"})
    param_dict.update({"minimum": vec_size})
    param_dict.update({"maximum": max_dim})
    param_dict.update({"enum": list(range(vec_size, max_dim + 1, vec_size))})

    if TP_DIM != 0:
        # Snap to nearest legal multiple of vec_size within range.
        snapped = int(com.CEIL(TP_DIM, vec_size))
        snapped = max(vec_size, min(snapped, max_dim))
        param_dict.update({"actual": snapped})

    return param_dict


def validate_TP_DIM(args):
    TT_DATA = args["TT_DATA"]
    TP_DIM = int(args["TP_DIM"])

    param_dict = update_TP_DIM(args)
    vec_size = param_dict["minimum"]
    max_dim = param_dict["maximum"]

    if TP_DIM % vec_size != 0:
        return com.isError(
            f"TP_DIM ({TP_DIM}) must be a multiple of {vec_size} "
            f"(the internal vector size for TT_DATA={TT_DATA})."
        )

    return com.validate_range([vec_size, max_dim], "TP_DIM", TP_DIM)


#######################################################
########### TP_CASC_LEN Updater and Validator #########
#######################################################
def update_TP_CASC_LEN(args):
    """eigenvalue_graph only supports TP_CASC_LEN = 1."""
    param_dict = {}
    param_dict.update({"name": "TP_CASC_LEN"})
    param_dict.update({"enum": [1]})
    param_dict.update({"actual": 1})
    return param_dict


def validate_TP_CASC_LEN(args):
    TP_CASC_LEN = int(args["TP_CASC_LEN"])
    if TP_CASC_LEN != 1:
        return com.isError(
            f"TP_CASC_LEN = {TP_CASC_LEN} is not supported by eigenvalue_graph. "
            "Only TP_CASC_LEN = 1 is currently valid; parallelism across multiple "
            "kernels is reserved for a future release."
        )
    return com.isValid


#######################################################
########### TP_PASSES Updater and Validator ###########
#######################################################
def update_TP_PASSES(args):
    if "TP_PASSES" in args and args["TP_PASSES"]:
        TP_PASSES = int(args["TP_PASSES"])
    else:
        TP_PASSES = TP_PASSES_default
    return fn_update_tp_passes(TP_PASSES)


def fn_update_tp_passes(TP_PASSES):
    param_dict = {}
    param_dict.update({"name": "TP_PASSES"})
    param_dict.update({"minimum": TP_PASSES_min})
    param_dict.update({"maximum": TP_PASSES_max})
    param_dict.update({"enum": list(range(TP_PASSES_min, TP_PASSES_max + 1))})
    return param_dict


def validate_TP_PASSES(args):
    TP_PASSES = int(args["TP_PASSES"])
    param_dict = update_TP_PASSES(args)
    return com.validate_range([param_dict["minimum"], param_dict["maximum"]], "TP_PASSES", TP_PASSES)


#######################################################
########### Port information ##########################
#######################################################
def info_ports(args):
    """Return port information for the eigenvalue_graph.

    The graph exposes two ports:
      - in  : input matrix A (TP_DIM x TP_DIM, row-major)
      - out : eigenvalues of A (TP_DIM elements)
    """
    TT_DATA = args["TT_DATA"]
    TP_DIM = int(args["TP_DIM"])

    TP_WINDOW_VSIZE_IN = TP_DIM * TP_DIM  # samples in A
    TP_WINDOW_VSIZE_OUT = TP_DIM          # samples in E

    port_in = com.get_port_info("in", "in", TT_DATA, TP_WINDOW_VSIZE_IN, 1, 0, com.API_BUFFER)
    port_out = com.get_port_info("out", "out", TT_DATA, TP_WINDOW_VSIZE_OUT, 1, 0, com.API_BUFFER)

    return port_in + port_out


#######################################################
########### Graph code generator ######################
#######################################################
def generate_graph(graphname, args):
    """Generate C++ wrapper code that instantiates eigenvalue_graph."""
    if graphname == "":
        graphname = "default_graphname"

    TT_DATA = args["TT_DATA"]
    TP_DIM = args["TP_DIM"]
    TP_PASSES = args["TP_PASSES"]
    TP_CASC_LEN = 1  # Always 1 for eigenvalue.

    code = f"""
class {graphname} : public adf::graph {{
public:
  adf::port<input>  in;
  adf::port<output> out;

  xf::solver::aie::eigenvalue::eigenvalue_graph<
    {TT_DATA},     // TT_DATA
    {TP_DIM},      // TP_DIM
    {TP_PASSES},   // TP_PASSES
    {TP_CASC_LEN}  // TP_CASC_LEN (must be 1)
  > eig;

  {graphname}() : eig() {{
    adf::connect<> net_in(in,     eig.in);
    adf::connect<> net_out(eig.out, out);
  }}

}};
"""
    out = {}
    out["graph"] = code
    out["port_info"] = info_ports(args)
    out["headerfile"] = "eigenvalue_graph.hpp"
    out["searchpaths"] = [
        "L2/include/aie",
        "L2/tests/aie/common/inc",
        "L1/include/aie",
        "L1/src/aie",
        "L1/tests/aie/inc",
        "L1/tests/aie/src",
    ]
    return out
