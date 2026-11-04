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
Metadata module for the mp_pseudoinverse_svd IP core.

Provides parameter validators and updaters for the mp_pseudoinverse_svd_graph
ADF graph.  The graph computes the Moore-Penrose pseudoinverse via SVD:

    A^+ = V · diag(1/S) · U^H

The parameter space mirrors that of the underlying svd_graph, with the
additional restriction that TP_CASC_LEN = 1.  Higher cascade lengths
require a gather stage for the distributed U row partitions before the
pseudoinverse multiply, which is not yet supported.

Naming conventions (from svd.py pattern):
  validate_  — boolean/error validator returning {"is_valid": bool, ...}
  update_    — updater returning param_dict with legal set or range
  info_      — creates port/connection information
  fn_        — internal helper
"""
import sys, os
import aie_common as com
import aie_common_fir_updaters as com_upd
from aie_common import *
import svd

import math as math
from math import floor, ceil, sqrt

script_directory = os.path.dirname(os.path.abspath(__file__))
L2path = script_directory + "/.."
include_path = L2path + "/tests/aie/common/scripts"
sys.path.insert(0, include_path)

# The MatMul stage (T * U^H -> A^+) is xf::dsp's matrix_mult_graph (AIE1) /
# matrix_mult_vecapi_graph (AIE-ML/AIE-MLv2), which lives in the sibling
# xf_dsp library, not this one. Locate its L2/meta the same way the build
# (see e.g. L2/tests/aie/mp_pseudoinverse_svd/Makefile's DSP_DIR ?= ../dsp,
# overridable via the DSP_DIR env var) locates xf_dsp's headers, so this
# script and the compiled graph always agree on where xf_dsp lives.
xflib_dir = os.path.normpath(os.path.join(L2path, ".."))
dsp_dir = os.environ.get("DSP_DIR", "../dsp")
dsp_meta_path = os.path.normpath(os.path.join(xflib_dir, dsp_dir, "L2", "meta"))
sys.path.insert(0, dsp_meta_path)
import matrix_mult

# -----------------------------------------------------------------------
# Cascade length constants — mp_pseudoinverse_svd only supports CASC_LEN = 1.
# Higher values are a future extension.
# -----------------------------------------------------------------------
TP_CASC_LEN_min = 1
TP_CASC_LEN_max = 1   # Restricted to 1 — see module docstring.


# -----------------------------------------------------------------------
# mp_pseudoinverse_svd_graph.hpp instantiates:
#   svd_graph<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN=1, TP_PASSES_SSR, TP_DIAG_INV=1>
#
# TT_DATA/TP_DIM_ROWS/TP_DIM_COLS/TP_PASSES/TP_PASSES_SSR are passed straight
# through from this IP's own parameters (identical names in svd.py), so
# rather than re-deriving svd_graph's own legality math here, the functions
# below build the args dict svd_graph would see (via _svd_args, which pins
# the two hardcoded template arguments) and call svd.py's own update_/
# validate_ functions directly -- the true constraint always lives in
# exactly one place: svd.py.
#
# Wrinkle: svd.py's own TP_DIM_ROWS/TP_DIM_COLS range functions consider any
# legal TP_CASC_LEN (1..16); this graph fixes TP_CASC_LEN=1, so those ranges
# are tightened further below, and svd.validate_TP_CASC_LEN is additionally
# consulted to confirm CASC_LEN=1 itself is legal at the chosen dimensions.
# -----------------------------------------------------------------------
def _svd_args(args):
    """args dict for the internal svd_graph, as instantiated in
    mp_pseudoinverse_svd_graph.hpp: svd_graph<..., TP_CASC_LEN=1, ..., TP_DIAG_INV=1>."""
    a = dict(args)
    a["TP_CASC_LEN"] = 1
    a["TP_DIAG_INV"] = 1
    return a


# -----------------------------------------------------------------------
# Graph-own buffer calculations (NOT subgraph calculations -- the fused
# S-reciprocal/V-scale kernel and MatMul are this graph's own kernels, not
# part of svd_graph, so svd.py has no knowledge of them). kStoreCols reuses
# svd.fn_kernel_store_align (AIE_VARIANT-dependent store alignment) rather
# than re-deriving it, since mp_pseudoinverse_svd_graph.hpp explicitly
# mirrors svd_graph's own column padding convention.
# -----------------------------------------------------------------------
def fn_kernel_store_cols(AIE_VARIANT, TT_DATA, TP_DIM_COLS):
    """Return kStoreCols exactly as computed in mp_pseudoinverse_svd_graph.hpp:

        kSchedCols  = TP_DIM_COLS + (TP_DIM_COLS % 2)
        kStoreAlign = fnVecStoreAlign<TT_DATA>()
        kStoreCols  = CEIL(kSchedCols, kStoreAlign)
    """
    cols_sched = TP_DIM_COLS + (TP_DIM_COLS % 2)
    store_align = svd.fn_kernel_store_align(AIE_VARIANT, TT_DATA)
    return int(com.CEIL(cols_sched, store_align))


def fn_estimated_kernel_data_bytes(AIE_VARIANT, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_CASC_LEN=1):
    """Largest data memory requirement (bytes) that must fit in one memory module
    (com.k_data_memory_bytes[AIE_VARIANT]), across the graph's three kernel stages:

    - SVD stage: svd.py's own per-kernel estimate (Input + U + S + V + scratch).
    - Fused kernel (m_pseudoInvKernel) and MatMul (m_matMul): the largest single
      buffer (S, V, T, U / U^T view, A^+; each kStoreCols-padded). These are graph
      buffers, which the mapper places independently across the kernel's
      neighbouring memory modules, so the limit is per buffer, not their sum --
      the same per-buffer rule xf_dsp uses (FIR graphs' bufferSize <
      kMemoryModuleSize, matrix_mult.py's per-operand cascade rule). Verified on
      AIE1 float 512x2 (2026-10-06): the summed MatMul total (33024 B) exceeds
      32 KB, yet it compiled, mapped (U and A^+ on different tiles) and passed aiesim.
    """
    svd_bytes = svd.fn_estimated_kernel_data_bytes(
        AIE_VARIANT, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_CASC_LEN
    )

    kStoreCols = fn_kernel_store_cols(AIE_VARIANT, TT_DATA, TP_DIM_COLS)
    data_bytes = com.fn_size_by_byte(TT_DATA)

    fused_kernel_buffer_bytes = max(
        kStoreCols * 4,                          # S input, always float
        kStoreCols * kStoreCols * data_bytes,    # V input / T output
    )

    matmul_buffer_bytes = max(
        kStoreCols * kStoreCols * data_bytes,    # T (input A)
        TP_DIM_ROWS * kStoreCols * data_bytes,   # U / U^T view (input B)
        kStoreCols * TP_DIM_ROWS * data_bytes,   # A^+ output
    )

    return max(svd_bytes, fused_kernel_buffer_bytes, matmul_buffer_bytes)


# -----------------------------------------------------------------------
# MatMul (T * U^H -> A^+) sub-graph args and validation, delegated to
# matrix_mult.py (xf_dsp) -- the real metadata for
# xf::dsp::aie::blas::matrix_mult::matrix_mult_graph, matching how svd.py is
# used above for svd_graph.
# -----------------------------------------------------------------------
def _matmul_args(args):
    """args dict for the internal MatMul, as instantiated in
    mp_pseudoinverse_svd_graph.hpp:
        TT_DATA_A=TT_DATA, TT_DATA_B=TT_DATA, TT_OUT_DATA=TT_DATA,
        TP_DIM_A=kStoreCols, TP_DIM_AB=kStoreCols, TP_DIM_B=TP_DIM_ROWS,
        TP_SHIFT=0, TP_RND=0,
        TP_DIM_A_LEADING=1 (COL_MAJOR), TP_DIM_B_LEADING=0 (ROW_MAJOR),
        TP_DIM_OUT_LEADING=1 (COL_MAJOR),
        TP_ADD_TILING_A/B/DETILING_OUT=kTilingEnabled,
        TP_INPUT_WINDOW_VSIZE_A=kStoreCols*kStoreCols,
        TP_INPUT_WINDOW_VSIZE_B=TP_DIM_ROWS*kStoreCols,
        TP_CASC_LEN=1, TP_SAT=1, TP_SSR=1,
        TP_CONJ_A=0, TP_CONJ_B=1 if TT_DATA is complex else 0.
    """
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    TP_DIM_ROWS = int(args["TP_DIM_ROWS"]) if "TP_DIM_ROWS" in args and args["TP_DIM_ROWS"] else svd.fn_kernel_vec_size(TT_DATA)
    TP_DIM_COLS = int(args["TP_DIM_COLS"]) if "TP_DIM_COLS" in args and args["TP_DIM_COLS"] else 2
    kStoreCols = fn_kernel_store_cols(AIE_VARIANT, TT_DATA, TP_DIM_COLS)
    kIsComplex = 1 if TT_DATA == "cfloat" else 0
    # kTilingEnabled = (__ALIGN_BYTE_SIZE__ == 64) ? 0u : 1u in the C++ header;
    # AIE1 is 32-byte aligned (tiling enabled), AIE-ML/AIE-MLv2 are 64-byte
    # aligned (tiling disabled, and AIE1's matrix_mult_graph is swapped out
    # for the local matrix_mult_vecapi_graph there anyway -- see below).
    kTilingEnabled = 1 if AIE_VARIANT == com.AIE else 0

    return {
        "AIE_VARIANT": AIE_VARIANT,
        "TT_DATA_A": TT_DATA,
        "TT_DATA_B": TT_DATA,
        "TT_OUT_DATA": TT_DATA,
        "TP_DIM_A": kStoreCols,
        "TP_DIM_AB": kStoreCols,
        "TP_DIM_B": TP_DIM_ROWS,
        "TP_SHIFT": 0,
        "TP_RND": 0,
        "TP_DIM_A_LEADING": 1,
        "TP_DIM_B_LEADING": 0,
        "TP_DIM_OUT_LEADING": 1,
        "TP_ADD_TILING_A": kTilingEnabled,
        "TP_ADD_TILING_B": kTilingEnabled,
        "TP_ADD_DETILING_OUT": kTilingEnabled,
        "TP_INPUT_WINDOW_VSIZE_A": kStoreCols * kStoreCols,
        "TP_INPUT_WINDOW_VSIZE_B": TP_DIM_ROWS * kStoreCols,
        "TP_CASC_LEN": 1,
        "TP_SAT": 1,
        "TP_SSR": 1,
        "TP_CONJ_A": 0,
        "TP_CONJ_B": kIsComplex,
    }


def fn_matmul_validate(AIE_VARIANT, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS):
    """Validate the internal MatMul instantiation against matrix_mult.py.

    Only applies on AIE1 (AIE_VARIANT == com.AIE): mp_pseudoinverse_svd_graph.hpp
    uses xf::dsp's matrix_mult_graph there, matching matrix_mult.py exactly.
    On AIE-ML/AIE-MLv2 it swaps in the local matrix_mult_vecapi_graph instead
    (matrix_mult_graph has no mmul hardware tiling for float/cfloat on those
    variants) -- a different implementation with no published metadata
    script, so there is nothing to cross-check there.
    """
    if AIE_VARIANT != com.AIE:
        return com.isValid

    matmul_args = _matmul_args(
        {"AIE_VARIANT": AIE_VARIANT, "TT_DATA": TT_DATA, "TP_DIM_ROWS": TP_DIM_ROWS, "TP_DIM_COLS": TP_DIM_COLS}
    )

    for check_name, validator in (
        ("TP_DIM_A", matrix_mult.validate_TP_DIM_A),
        ("TP_DIM_B", matrix_mult.validate_TP_DIM_B),
        ("TP_DIM_AB", matrix_mult.validate_TP_DIM_AB),
        ("TP_INPUT_WINDOW_VSIZE_A", matrix_mult.validate_TP_INPUT_WINDOW_VSIZE_A),
        ("TP_INPUT_WINDOW_VSIZE_B", matrix_mult.validate_TP_INPUT_WINDOW_VSIZE_B),
    ):
        check = validator(matmul_args)
        if not check["is_valid"]:
            return com.isError(
                f"TP_DIM_ROWS = {TP_DIM_ROWS} / TP_DIM_COLS = {TP_DIM_COLS} is not legal for "
                f"the internal MatMul stage ({check_name}): {check.get('err_message', '')}"
            )
    return com.isValid


def fn_matmul_fits(AIE_VARIANT, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS):
    return fn_matmul_validate(AIE_VARIANT, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS)["is_valid"]


#######################################################
########### AIE_VARIANT Updater and Validator #########
#######################################################
# AIE_VARIANT selects the target platform; it is simply this IP's own
# template parameter value, not something derived from svd_graph, so it is
# not delegated to svd.py like the other parameters below.
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
'''def fn_matmul_tt_data_legal_sets(args, TT_DATA):
    """Legal sets of the internal MatMul's TT_DATA_A, TT_DATA_B and TT_OUT_DATA
    (matrix_mult.py updaters) when this IP's TT_DATA is the candidate TT_DATA.
    The _matmul_args() dims only matter for the other MatMul validators, but
    are filled in the same way the TP_DIM_* updaters below default them."""
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_DIM_ROWS = int(args["TP_DIM_ROWS"]) if "TP_DIM_ROWS" in args and args["TP_DIM_ROWS"] else svd.fn_kernel_vec_size(TT_DATA)
    TP_DIM_COLS = int(args["TP_DIM_COLS"]) if "TP_DIM_COLS" in args and args["TP_DIM_COLS"] else 2
    matmul_args = _matmul_args(AIE_VARIANT, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS)
    return [
        matrix_mult.update_TT_DATA_A(matmul_args)["enum"],
        matrix_mult.update_TT_DATA_B(matmul_args)["enum"],
        matrix_mult.update_TT_OUT_DATA(matmul_args)["enum"],
    ]

def fn_tt_data_legal_sets(args):
    """List of legal sets for TT_DATA, one per constraining sub-graph parameter.
    Their intersection is the legal set of this IP's TT_DATA."""
    legal_sets = [svd.update_TT_DATA(args)["enum"]]

    # MatMul constraints only apply on AIE1 (see fn_matmul_validate).
    if args["AIE_VARIANT"] == com.AIE:
        # TT_DATA_A, TT_DATA_B and TT_OUT_DATA all take this IP's TT_DATA, and
        # the B/OUT legal sets depend on A, so run the matrix_mult updaters
        # once per candidate TT_DATA value and keep the candidate in each set.
        candidates = legal_sets[0]
        per_candidate = {c: fn_matmul_tt_data_legal_sets(args, c) for c in candidates}
        for i in range(3):
            legal_sets.append([c for c in candidates if c in per_candidate[c][i]])
    return legal_sets

def update_TT_DATA(args):
    legal_sets = fn_tt_data_legal_sets(args)
    legal_set_TT_DATA = legal_sets[0]
    for legal_set in legal_sets[1:]:
        legal_set_TT_DATA = [t for t in legal_set_TT_DATA if t in legal_set]
    return {"name": "TT_DATA", "enum": legal_set_TT_DATA}'''

def update_TT_DATA(args):
    return fn_update_tt_data(args)


def fn_update_tt_data(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    legal_set_TT_DATA = ["float", "cfloat"]
    svd_args = _svd_args(args)  # not needed here but included for consistency
    svd_legal_set_TT_DATA = svd.update_TT_DATA(args)["enum"]

    # MatMul constraints only apply on AIE1 (see fn_matmul_validate). matrix_mult.py
    # removes float/cfloat on AIE-ML/AIE-MLv2, which would empty this set.
    matmul_legal_set_TT_DATA_A = legal_set_TT_DATA
    matmul_legal_set_TT_DATA_B = legal_set_TT_DATA
    matmul_legal_set_TT_OUT_DATA = legal_set_TT_DATA
    if AIE_VARIANT == com.AIE:
        # TT_DATA_A, TT_DATA_B and TT_OUT_DATA all take this IP's TT_DATA, and the B/OUT
        # legal sets depend on A, so probe each candidate on its own instead of reading
        # args["TT_DATA"]: this is TT_DATA's own updater, so that value may not be set yet
        # (config_helper calls it with TT_DATA = ""), and using it would also collapse the
        # legal set to just the current value.
        matmul_legal_set_TT_DATA_A = []
        matmul_legal_set_TT_DATA_B = []
        matmul_legal_set_TT_OUT_DATA = []
        for candidate in legal_set_TT_DATA:
            matmul_args = _matmul_args({**args, "TT_DATA": candidate})
            if candidate in matrix_mult.update_TT_DATA_A(matmul_args)["enum"]:
                matmul_legal_set_TT_DATA_A.append(candidate)
            if candidate in matrix_mult.update_TT_DATA_B(matmul_args)["enum"]:
                matmul_legal_set_TT_DATA_B.append(candidate)
            if candidate in matrix_mult.update_TT_OUT_DATA(matmul_args)["enum"]:
                matmul_legal_set_TT_OUT_DATA.append(candidate)
    param_dict = {}

    param_dict.update({"name": "TT_DATA"})
    # Keep legal_set_TT_DATA's order: the first entry is the default config_helper picks.
    all_legal_sets_TT_DATA = [t for t in legal_set_TT_DATA if t in svd_legal_set_TT_DATA and t in matmul_legal_set_TT_DATA_A and t in matmul_legal_set_TT_DATA_B and t in matmul_legal_set_TT_OUT_DATA]
    param_dict.update({"enum": all_legal_sets_TT_DATA})

    return param_dict

def validate_TT_DATA(args):
    TT_DATA = args["TT_DATA"]
    param_dict = update_TT_DATA(args)
    all_legal_sets_TT_DATA = param_dict["enum"]
    return com.validate_all_legal_sets([all_legal_sets_TT_DATA], "TT_DATA", args["TT_DATA"])


#######################################################
########### TP_DIM_COLS Updater and Validator #########
#######################################################
def update_TP_DIM_COLS(args):
    return fn_update_tp_dim_cols(args)


def fn_update_tp_dim_cols(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA = args["TT_DATA"]
    svd_args = _svd_args(args)
    matmul_args = _matmul_args(args)  # not needed here but included for consistency (MatMul legality is probed via fn_matmul_fits)

    # Baseline range from svd_graph itself (any legal TP_CASC_LEN).
    svd_param_dict = svd.update_TP_DIM_COLS(svd_args)
    svd_TP_DIM_COLS_min = svd_param_dict["minimum"]
    svd_TP_DIM_COLS_max = svd_param_dict["maximum"]

    # This graph's own range, starting from svd_graph's.
    TP_DIM_COLS_min = svd_TP_DIM_COLS_min
    TP_DIM_COLS_max = svd_TP_DIM_COLS_max
    BUFFER_SIZE = com.k_data_memory_bytes[AIE_VARIANT]
    TP_DIM_ROWS_min = svd.fn_kernel_vec_size(TT_DATA)

    # Tighten to this graph's own fixed TP_CASC_LEN=1 plus its extra
    # fused-kernel/MatMul buffers (each must fit one memory module, see
    # fn_estimated_kernel_data_bytes; probed at the minimum legal TP_DIM_ROWS,
    # same approach svd.py itself uses in fn_max_dim_cols), and to the
    # internal MatMul's own legality (matrix_mult.py, AIE1 only).
    while TP_DIM_COLS_max >= TP_DIM_COLS_min:
        if (fn_estimated_kernel_data_bytes(AIE_VARIANT, TT_DATA, TP_DIM_ROWS_min, TP_DIM_COLS_max, 1) <= BUFFER_SIZE
                and fn_matmul_fits(AIE_VARIANT, TT_DATA, TP_DIM_ROWS_min, TP_DIM_COLS_max)):
            break
        TP_DIM_COLS_max -= 1

    # Intersection of all ranges
    all_legal_ranges_TP_DIM_COLS = [
        [svd_TP_DIM_COLS_min, svd_TP_DIM_COLS_max],
        [TP_DIM_COLS_min, TP_DIM_COLS_max],
    ]
    all_TP_DIM_COLS_min = max(r[0] for r in all_legal_ranges_TP_DIM_COLS)
    all_TP_DIM_COLS_max = min(r[1] for r in all_legal_ranges_TP_DIM_COLS)

    param_dict = {}
    param_dict.update({"name": "TP_DIM_COLS"})
    param_dict.update({"minimum": all_TP_DIM_COLS_min})
    param_dict.update({"maximum": all_TP_DIM_COLS_max})

    TP_DIM_COLS = int(args["TP_DIM_COLS"]) if "TP_DIM_COLS" in args and args["TP_DIM_COLS"] else 0
    if TP_DIM_COLS != 0:
        param_dict["actual"] = com.CLIP(TP_DIM_COLS, all_TP_DIM_COLS_min, all_TP_DIM_COLS_max)

    return param_dict


def validate_TP_DIM_COLS(args):
    TP_DIM_COLS = int(args["TP_DIM_COLS"])
    param_dict = update_TP_DIM_COLS(args)
    all_legal_ranges_TP_DIM_COLS = [[param_dict["minimum"], param_dict["maximum"]]]
    return com.validate_all_ranges(all_legal_ranges_TP_DIM_COLS, "TP_DIM_COLS", TP_DIM_COLS)


#######################################################
########### TP_DIM_ROWS Updater and Validator #########
#######################################################
def update_TP_DIM_ROWS(args):
    return fn_update_tp_dim_rows(args)


def fn_update_tp_dim_rows(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA     = args["TT_DATA"]
    TP_DIM_COLS = int(args["TP_DIM_COLS"]) if "TP_DIM_COLS" in args and args["TP_DIM_COLS"] else 2
    svd_args    = _svd_args(args)
    matmul_args = _matmul_args(args)  # not needed here but included for consistency (MatMul legality is probed via fn_matmul_fits)

    # Baseline range from svd_graph itself (any legal TP_CASC_LEN).
    svd_param_dict = svd.update_TP_DIM_ROWS(svd_args)
    svd_TP_DIM_ROWS_min = svd_param_dict["minimum"]
    svd_TP_DIM_ROWS_max = svd_param_dict["maximum"]

    # This graph's own range, starting from svd_graph's.
    TP_DIM_ROWS_min = svd_TP_DIM_ROWS_min
    TP_DIM_ROWS_max = svd_TP_DIM_ROWS_max
    BUFFER_SIZE = com.k_data_memory_bytes[AIE_VARIANT]

    # Tighten to this graph's own fixed TP_CASC_LEN=1 plus its extra
    # fused-kernel/MatMul buffers (each must fit one memory module, see
    # fn_estimated_kernel_data_bytes; svd.py's own range above allows any
    # TP_CASC_LEN up to 16, which this graph does not support), and to the
    # internal MatMul's own legality (matrix_mult.py, AIE1 only).
    while TP_DIM_ROWS_max >= TP_DIM_ROWS_min:
        if (fn_estimated_kernel_data_bytes(AIE_VARIANT, TT_DATA, TP_DIM_ROWS_max, TP_DIM_COLS, 1) <= BUFFER_SIZE
                and fn_matmul_fits(AIE_VARIANT, TT_DATA, TP_DIM_ROWS_max, TP_DIM_COLS)):
            break
        TP_DIM_ROWS_max -= TP_DIM_ROWS_min

    if TP_DIM_ROWS_max < TP_DIM_ROWS_min:
        TP_DIM_ROWS_max = TP_DIM_ROWS_min

    # Intersection of all ranges
    all_legal_ranges_TP_DIM_ROWS = [
        [svd_TP_DIM_ROWS_min, svd_TP_DIM_ROWS_max],
        [TP_DIM_ROWS_min, TP_DIM_ROWS_max],
    ]
    all_TP_DIM_ROWS_min = max(r[0] for r in all_legal_ranges_TP_DIM_ROWS)
    all_TP_DIM_ROWS_max = min(r[1] for r in all_legal_ranges_TP_DIM_ROWS)

    param_dict = {}
    param_dict.update({"name": "TP_DIM_ROWS"})
    param_dict.update({"minimum": all_TP_DIM_ROWS_min})
    param_dict.update({"maximum": all_TP_DIM_ROWS_max})

    TP_DIM_ROWS = int(args["TP_DIM_ROWS"]) if "TP_DIM_ROWS" in args and args["TP_DIM_ROWS"] else 0
    if TP_DIM_ROWS != 0:
        # Snap to nearest multiple of vec_size within valid range
        TP_DIM_ROWS = int(com.CEIL(TP_DIM_ROWS, all_TP_DIM_ROWS_min))
        TP_DIM_ROWS = max(all_TP_DIM_ROWS_min, min(TP_DIM_ROWS, all_TP_DIM_ROWS_max))
        param_dict["actual"] = TP_DIM_ROWS

    return param_dict


def validate_TP_DIM_ROWS(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA     = args["TT_DATA"]
    TP_DIM_COLS = int(args["TP_DIM_COLS"]) if "TP_DIM_COLS" in args and args["TP_DIM_COLS"] else 2
    TP_DIM_ROWS = int(args["TP_DIM_ROWS"])
    svd_args    = _svd_args(args)

    param_dict = update_TP_DIM_ROWS(args)
    all_legal_ranges_TP_DIM_ROWS = [[param_dict["minimum"], param_dict["maximum"]]]
    all_ranges_check = com.validate_all_ranges(all_legal_ranges_TP_DIM_ROWS, "TP_DIM_ROWS", TP_DIM_ROWS)
    if not all_ranges_check["is_valid"]:
        return all_ranges_check

    # svd.py's own real vector-width + any-casc-len memory feasibility check.
    range_check = svd.validate_TP_DIM_ROWS(svd_args)
    if not range_check["is_valid"]:
        return range_check

    # This graph fixes TP_CASC_LEN = 1 specifically -- confirm that value is
    # itself legal at these dimensions, reusing svd.py's own memory-fit
    # check (svd.validate_TP_CASC_LEN) rather than re-deriving it.
    casc_check = svd.validate_TP_CASC_LEN(svd_args)
    if not casc_check["is_valid"]:
        return com.isError(
            f"TP_DIM_ROWS = {TP_DIM_ROWS} / TP_DIM_COLS = {TP_DIM_COLS} has no valid "
            f"TP_CASC_LEN = 1 (mp_pseudoinverse_svd_graph requires TP_CASC_LEN = 1): "
            f"{casc_check.get('err_message', '')}"
        )

    # Must additionally fit this graph's own extra kernels (fused
    # S-reciprocal/V-scale kernel and MatMul), which svd.py has no knowledge of:
    # each of their buffers must fit in one memory module.
    BUFFER_SIZE = com.k_data_memory_bytes[AIE_VARIANT]
    est_bytes = fn_estimated_kernel_data_bytes(AIE_VARIANT, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, 1)
    if est_bytes > BUFFER_SIZE:
        return com.isError(
            f"TP_DIM_ROWS = {TP_DIM_ROWS} needs a buffer larger than one memory module in the "
            f"SVD, fused S-reciprocal/V-scale kernel or MatMul stage (largest {est_bytes} bytes, "
            f"limit {BUFFER_SIZE} bytes). Reduce TP_DIM_ROWS or TP_DIM_COLS."
        )

    # Must additionally be legal for the internal MatMul stage itself
    # (matrix_mult.py, xf_dsp -- AIE1 only, see fn_matmul_validate).
    matmul_check = fn_matmul_validate(AIE_VARIANT, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS)
    if not matmul_check["is_valid"]:
        return matmul_check

    return com.isValid


#######################################################
########### TP_CASC_LEN Updater and Validator #########
#######################################################
def update_TP_CASC_LEN(args):
    return fn_update_tp_casc_len(args)


def fn_update_tp_casc_len(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    # mp_pseudoinverse_svd only supports TP_CASC_LEN = 1.
    legal_set_TP_CASC_LEN = [1]
    svd_args = _svd_args(args)
    svd_legal_set_TP_CASC_LEN = svd.update_TP_CASC_LEN(svd_args)["enum"]
    matmul_args = _matmul_args(args)

    # MatMul constraints only apply on AIE1 (see fn_matmul_validate).
    matmul_legal_set_TP_CASC_LEN = legal_set_TP_CASC_LEN
    if AIE_VARIANT == com.AIE:
        matmul_param_dict = matrix_mult.update_TP_CASC_LEN(matmul_args)
        # matrix_mult.update_TP_CASC_LEN returns False if no cascade length is legal
        matmul_legal_set_TP_CASC_LEN = matmul_param_dict["enum"] if matmul_param_dict else []

    param_dict = {}
    param_dict.update({"name": "TP_CASC_LEN"})
    all_legal_sets_TP_CASC_LEN = sorted(set(legal_set_TP_CASC_LEN) & set(svd_legal_set_TP_CASC_LEN) & set(matmul_legal_set_TP_CASC_LEN))
    param_dict.update({"enum": all_legal_sets_TP_CASC_LEN})
    param_dict.update({"actual": 1})
    return param_dict


def validate_TP_CASC_LEN(args):
    TP_CASC_LEN = int(args["TP_CASC_LEN"])
    param_dict = update_TP_CASC_LEN(args)
    all_legal_sets_TP_CASC_LEN = param_dict["enum"]
    return com.validate_all_legal_sets([all_legal_sets_TP_CASC_LEN], "TP_CASC_LEN", TP_CASC_LEN)


#######################################################
########### TP_PASSES Updater and Validator ###########
#######################################################
def update_TP_PASSES(args):
    return fn_update_tp_passes(args)


def fn_update_tp_passes(args):
    # Default of 3 is this graph's own choice (accuracy/latency trade-off);
    # svd.py itself defaults to 1. Legal range/enum is delegated to svd.py.
    if "TP_PASSES" in args and args["TP_PASSES"]:
        TP_PASSES = int(args["TP_PASSES"])
    else:
        TP_PASSES = 3
    svd_args = _svd_args(args)  # not needed here but included for consistency
    svd_param_dict = svd.fn_update_tp_passes(TP_PASSES)
    svd_TP_PASSES_min = svd_param_dict["minimum"]
    svd_TP_PASSES_max = svd_param_dict["maximum"]
    matmul_args = _matmul_args(args)  # not needed here but included for consistency

    # Intersection of all ranges
    all_legal_ranges_TP_PASSES = [[svd_TP_PASSES_min, svd_TP_PASSES_max]]
    all_TP_PASSES_min = max(r[0] for r in all_legal_ranges_TP_PASSES)
    all_TP_PASSES_max = min(r[1] for r in all_legal_ranges_TP_PASSES)

    param_dict = {}
    param_dict.update({"name": "TP_PASSES"})
    param_dict.update({"minimum": all_TP_PASSES_min})
    param_dict.update({"maximum": all_TP_PASSES_max})
    param_dict.update({"enum": list(range(all_TP_PASSES_min, all_TP_PASSES_max + 1))})
    if "actual" in svd_param_dict:
        param_dict.update({"actual": svd_param_dict["actual"]})

    return param_dict


def validate_TP_PASSES(args):
    TP_PASSES = int(args["TP_PASSES"])
    param_dict = update_TP_PASSES(args)
    all_legal_ranges_TP_PASSES = [[param_dict["minimum"], param_dict["maximum"]]]
    return com.validate_all_ranges(all_legal_ranges_TP_PASSES, "TP_PASSES", TP_PASSES)


#######################################################
######## TP_PASSES_SSR Updater and Validator ##########
#######################################################
def update_TP_PASSES_SSR(args):
    return fn_update_tp_passes_ssr(args)


def fn_update_tp_passes_ssr(args):
    svd_args = _svd_args(args)
    svd_param_dict = svd.update_TP_PASSES_SSR(svd_args)
    svd_legal_set_TP_PASSES_SSR = svd_param_dict["enum"]
    matmul_args = _matmul_args(args)  # not needed here but included for consistency

    param_dict = {}
    param_dict.update({"name": "TP_PASSES_SSR"})
    all_legal_sets_TP_PASSES_SSR = sorted(set(svd_legal_set_TP_PASSES_SSR))
    param_dict.update({"enum": all_legal_sets_TP_PASSES_SSR})
    param_dict.update({"actual": svd_param_dict["actual"]})

    return param_dict


def validate_TP_PASSES_SSR(args):
    TP_PASSES_SSR = int(args["TP_PASSES_SSR"])
    param_dict = update_TP_PASSES_SSR(args)
    all_legal_sets_TP_PASSES_SSR = param_dict["enum"]
    return com.validate_all_legal_sets([all_legal_sets_TP_PASSES_SSR], "TP_PASSES_SSR", TP_PASSES_SSR)


#######################################################
########### Port information ##########################
#######################################################
def info_ports(args):
    """Return port information for the mp_pseudoinverse_svd_graph.

    The graph exposes two ports:
      - in  : input matrix A   (TP_DIM_ROWS x TP_DIM_COLS, column-major)
      - out : pseudoinverse A^+ (kStoreCols x TP_DIM_ROWS, column-major).
        kStoreCols >= TP_DIM_COLS whenever column padding occurs (see
        mp_pseudoinverse_svd_graph.hpp's kOutResultVsize) — using raw
        TP_DIM_COLS here would under-declare the real output buffer size.
    """
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA     = args["TT_DATA"]
    TP_DIM_ROWS = int(args["TP_DIM_ROWS"])
    TP_DIM_COLS = int(args["TP_DIM_COLS"])

    kStoreCols = fn_kernel_store_cols(AIE_VARIANT, TT_DATA, TP_DIM_COLS)

    TP_WINDOW_VSIZE_IN  = TP_DIM_ROWS * TP_DIM_COLS   # samples in A
    TP_WINDOW_VSIZE_OUT = kStoreCols * TP_DIM_ROWS    # samples in A^+ (kOutResultVsize)

    # Single input port and single output port (TP_CASC_LEN = 1)
    port_in  = com.get_port_info("in",  "in",  TT_DATA, TP_WINDOW_VSIZE_IN,  1, 0, com.API_BUFFER)
    port_out = com.get_port_info("out", "out", TT_DATA, TP_WINDOW_VSIZE_OUT, 1, 0, com.API_BUFFER)

    return port_in + port_out


#######################################################
########### Graph code generator ######################
#######################################################
def generate_graph(graphname, args):
    """Generate C++ wrapper code that instantiates mp_pseudoinverse_svd_graph."""
    if graphname == "":
        graphname = "default_graphname"

    TT_DATA     = args["TT_DATA"]
    TP_DIM_ROWS = args["TP_DIM_ROWS"]
    TP_DIM_COLS = args["TP_DIM_COLS"]
    TP_PASSES   = args["TP_PASSES"]
    TP_PASSES_SSR = args["TP_PASSES_SSR"]
    TP_CASC_LEN = 1   # Always 1 for mp_pseudoinverse_svd.

    code = f"""
class {graphname} : public adf::graph {{
public:
  adf::port<input>  in;
  adf::port<output> out;

  xf::solver::aie::mp_pseudoinverse_svd::mp_pseudoinverse_svd_graph<
    {TT_DATA},    // TT_DATA
    {TP_DIM_ROWS}, // TP_DIM_ROWS
    {TP_DIM_COLS}, // TP_DIM_COLS
    {TP_PASSES},   // TP_PASSES
    {TP_CASC_LEN}, // TP_CASC_LEN (must be 1)
    {TP_PASSES_SSR} // TP_PASSES_SSR
  > pinv;

  {graphname}() : pinv() {{
    adf::connect<> net_in(in,       pinv.in);
    adf::connect<> net_out(pinv.out, out);
  }}

}};
"""
    out = {}
    out["graph"]     = code
    out["port_info"] = info_ports(args)
    out["headerfile"] = "mp_pseudoinverse_svd_graph.hpp"
    out["searchpaths"] = [
        "L2/include/aie",
        "L2/tests/aie/common/inc",
        "L1/include/aie",
        "L1/src/aie",
        "L1/tests/aie/inc",
        "L1/tests/aie/src",
    ]
    return out
