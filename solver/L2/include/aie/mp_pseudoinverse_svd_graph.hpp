/*
 * Copyright (C) 2026, Advanced Micro Devices, Inc.
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
#ifndef _SOLVERLIB_MP_PSEUDOINVERSE_SVD_GRAPH_HPP_
#define _SOLVERLIB_MP_PSEUDOINVERSE_SVD_GRAPH_HPP_

/*
The file captures the definition of the 'L2' graph level class for the
Moore-Penrose pseudoinverse function library element.

Algorithm:
  Given input matrix A (TP_DIM_ROWS x TP_DIM_COLS), decompose via SVD:
      A = U * S * V^H
  Then form the Moore-Penrose pseudoinverse:
      A^+ = V * S^+ * U^H
  where S^+ replaces each non-zero singular value s_i by 1/s_i.

Pipeline (all within this graph):
  1. SVD graph                   : A            -> U, S, V
  2. mp_pseudoinverse_svd kernel : S, V         -> T = V * diag(S^{-1})  (kStoreCols x kStoreCols, col-major)
  3. MatMul                      : T * U^H      -> A^+                   (kStoreCols x TP_DIM_ROWS, col-major)

T = V * diag(S^{-1}) is computed by the m_pseudoInvKernel directly, without
ever materialising the kStoreCols x kStoreCols diagonal S^{-1} matrix or
running a general matrix multiply against it: since diag(S^{-1}) is zero
off the diagonal, (V * S^{-1})[i,j] = V[i,j] * S^{-1}[j], i.e. column j of V
scaled by the scalar S^{-1}[j]. A general matmul computing this would spend
the overwhelming majority of its MACs multiplying by the zero off-diagonal
entries; the fused kernel performs only the kStoreCols^2 scalar multiplies
that are actually non-zero.  (An earlier revision of this graph built the
diagonal matrix explicitly and fed it to a dedicated MatMul 1 stage; see
mp_pseudoinverse_svd_design_notes.md for that history.)

This graph pairs the internal SVD with TP_DIAG_INV=1, so the SVD emits 1/S
directly (a free byproduct of normalizing its own U output -- see
svd.cpp/svd_graph.hpp) instead of S. m_pseudoInvKernel is instantiated with
the matching TP_DIAG_INV=1, so it uses that reciprocal as-is instead of
re-deriving its own -- eliminating a redundant per-element division that the
naive SVD(A) -> S -> 1/S -> scale-V pipeline would otherwise perform twice.
This is purely an internal implementation optimization: it does not change
this graph's external interface (in -> out, A -> A^+) or its numerical
result, beyond the near-zero-singular-value truncation threshold now being
SVD's own (see mp_pseudoinverse_svd.cpp/mp_pseudoinverse_svd.hpp for detail).

The remaining MatMul is one of two interchangeable graph types, selected at
compile time by architecture:
  - AIE1 (__AIE_ARCH__==10): xf::dsp's mmul-intrinsic matrix_mult_graph.
  - AIE-ML / AIE-MLv2 (__AIE_ARCH__!=10): matrix_mult_vecapi_graph, a
    vector-API fallback (no mmul intrinsic) used because matrix_mult_graph
    has no hardware tiling for float (AIE-ML) or cfloat (AIE-ML, AIE-MLv2)
    -- see matrix_mult_vecapi_graph.hpp / matrix_mult_vecapi.hpp.
Both graph types share an identical external interface (inA[1]/inB[1]/out[1])
and are instantiated with the same argument list, so the constructor wiring
below is architecture-agnostic.

Conjugate-transpose in MatMul:
  U from the SVD is stored column-major  (pU[i + k*TP_DIM_ROWS] = U[i,k]).
  The byte layout of a column-major M×N matrix is identical to the byte layout
  of a row-major N×M matrix.  Therefore, passing U with TP_DIM_B_LEADING = ROW_MAJOR
  causes the matrix_mult_graph to interpret it as the transpose U^T.
  For TT_DATA = float, U^T == U^H (real data), so no further action is needed.
  For TT_DATA = cfloat, U^T != U^H.  MatMul is instantiated with TP_CONJ_B = 1
  (set automatically via kIsComplex; TP_CONJ_A is explicitly 0), which applies
  aie::conj() to each tiled B vector inside the kernel's mac/mul loop, yielding
  the required U^H natively without any additional conjugation kernel.

Output dimensions:
  The result buffer is kStoreCols x TP_DIM_ROWS samples.  When TP_DIM_COLS is
  already aligned to kStoreAlign (the common case), kStoreCols == TP_DIM_COLS and
  the buffer contains exactly TP_DIM_COLS x TP_DIM_ROWS valid samples.
  If kStoreCols > TP_DIM_COLS (padding required), the extra rows of A^+ are stripped
  by the post-processing depadding step.

Restriction:
  TP_CASC_LEN must be 1.
*/
/**
 * @file mp_pseudoinverse_svd_graph.hpp
 *
 **/

#include <adf.h>
#include <type_traits>
#include "graph_utils.hpp"
#include "device_defs.h"
#include "svd.hpp"
#include "svd_graph.hpp"
#include "mp_pseudoinverse_svd.hpp"
#include "matrix_mult_graph.hpp"
#include "matrix_mult_vecapi_graph.hpp"

using namespace adf;

namespace xf {
namespace solver {
namespace aie {
namespace mp_pseudoinverse_svd {

using namespace xf::solver::aie::svd; // SvdScratchSize, traits

//--------------------------------------------------------------------------------------------------
// mp_pseudoinverse_svd_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @defgroup mp_pseudoinverse_svd_graph Moore-Penrose Pseudoinverse
 *
 * Moore-Penrose Pseudoinverse graph: computes A^+ = V * diag(1/S) * U^H via SVD.
 **/

/**
 * @ingroup mp_pseudoinverse_svd_graph
 * @brief Moore-Penrose pseudoinverse of a matrix A.
 *
 * Decomposes A via SVD (A = U * S * V^H) then assembles
 *     A^+ = V * diag(1/S) * U^H
 * by scaling each column of V by 1/S[k] (a fused kernel, no matrix multiply
 * against an explicit diagonal matrix) and then multiplying the result by
 * U^H using an xf::dsp matrix_mult_graph instance.
 *
 * @tparam TT_DATA    Scalar data type: float or cfloat.
 * @tparam TP_DIM_ROWS  Number of rows of A.
 * @tparam TP_DIM_COLS  Number of columns of A (= number of singular values).
 * @tparam TP_PASSES  Number of Jacobi sweep passes in the SVD (more = higher accuracy).
 * @tparam TP_CASC_LEN  Must be 1.
 * @tparam TP_PASSES_SSR Number of pass-pipeline stages in the internal SVD.
 **/
template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_PASSES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_PASSES_SSR = 1>
class mp_pseudoinverse_svd_graph : public graph {
   public:
    // -----------------------------------------------------------------------
    // Dimension constants (mirroring svd_graph)
    // -----------------------------------------------------------------------
    static constexpr bool kIsComplex = fnIsComplexDataType<TT_DATA>();
    static constexpr unsigned int kVecSizeIO = fnVecSampleNumIO<TT_DATA>();
    static constexpr unsigned int kStoreAlign = fnVecStoreAlign<TT_DATA>();
    static constexpr unsigned int kSchedCols = TP_DIM_COLS + (TP_DIM_COLS % 2);
    static constexpr unsigned int kStoreCols = ((kSchedCols + kStoreAlign - 1) / kStoreAlign) * kStoreAlign;

    // Buffer sizes (samples)
    static constexpr int kInWindowVsize = TP_DIM_ROWS * TP_DIM_COLS;  // input A
    static constexpr int kOutUWindowVsize = TP_DIM_ROWS * kStoreCols; // SVD U output
    static constexpr int kOutSWindowVsize = kStoreCols;               // SVD S (float)
    static constexpr int kOutVWindowVsize = kStoreCols * kStoreCols;  // SVD V output

    // T = V * diag(S^{-1}) emitted by the fused S-reciprocal/V-scale kernel
    // (kStoreCols x kStoreCols, col-major -- same layout as V, type TT_DATA)
    static constexpr int kSInvScaledVVsize = kStoreCols * kStoreCols;

    // Final output: A^+  (kStoreCols x TP_DIM_ROWS)
    // When kStoreCols == TP_DIM_COLS the result contains exactly TP_DIM_COLS x TP_DIM_ROWS
    // valid elements. When kStoreCols > TP_DIM_COLS the extra rows are stripped
    // by the post-processing depadding step in helper.mk.
    static constexpr int kOutResultVsize = kStoreCols * TP_DIM_ROWS;

    // -----------------------------------------------------------------------
    // Parameter validation
    // -----------------------------------------------------------------------
    static_assert(std::is_same<TT_DATA, float>::value || std::is_same<TT_DATA, cfloat>::value,
                  "ERROR: TT_DATA must be either float or cfloat.");
    static_assert(TP_CASC_LEN == 1, "ERROR: mp_pseudoinverse_svd_graph currently supports TP_CASC_LEN=1 only.");

    // -----------------------------------------------------------------------
    // MatMul graph type aliases
    // -----------------------------------------------------------------------
    // ROW_MAJOR = 0, COL_MAJOR = 1  (constants from matrix_mult.hpp via the include chain)
    //
    // AIE1 uses the mmul-intrinsic matrix_mult_graph (faster; tiling kernels
    // supported only on AIE1, __ALIGN_BYTE_SIZE__ == 32). AIE-ML/AIE-MLv2 use
    // matrix_mult_vecapi_graph instead, since matrix_mult_graph has no mmul
    // hardware tiling for float (AIE-ML) or cfloat (AIE-ML, AIE-MLv2) --
    // see matrix_mult_tiling_scheme.hpp.
    static constexpr unsigned int kTilingEnabled = (__ALIGN_BYTE_SIZE__ == 64) ? 0u : 1u;
    static constexpr bool kUseVecApiMatMul = (__AIE_ARCH__ != 10);

    /**
     * MatMul: T * U^H = A^+   where T = V * diag(S^{-1}) (from m_pseudoInvKernel)
     *
     * A = T  (kStoreCols x kStoreCols, col-major -- same layout as V, since
     *         scaling V's columns by S^{-1} does not change its layout)
     *        TP_DIM_A_LEADING = COL_MAJOR (1)
     * B = U^H  supplied as U in col-major storage (byte-identical to U^T row-major).
     *        TP_DIM_B_LEADING = ROW_MAJOR (0)  -> kernel sees U^T.
     *        TP_CONJ_B = kIsComplex             -> each element of U^T is conjugated,
     *                                             yielding the required U^H.
     *        For real TT_DATA (float) U^T == U^H, so TP_CONJ_B = 0 is correct.
     * C = A^+  (kStoreCols x TP_DIM_ROWS, col-major output)
     *        TP_DIM_OUT_LEADING = COL_MAJOR (1)
     */
    using MatMul_t = typename std::conditional<
        kUseVecApiMatMul,
        matrix_mult_vecapi_graph<TT_DATA,     // TT_DATA_A
                                 TT_DATA,     // TT_DATA_B
                                 kStoreCols,  // TP_DIM_A   - rows of T = V*S^{-1}
                                 kStoreCols,  // TP_DIM_AB  - shared dimension
                                 TP_DIM_ROWS, // TP_DIM_B   - cols of U^T (= rows of U)
                                 0,           // TP_SHIFT
                                 0,           // TP_RND
                                 1,           // TP_DIM_A_LEADING   = COL_MAJOR: T from m_pseudoInvKernel is col-major
                                 0,           // TP_DIM_B_LEADING   = ROW_MAJOR: col-major U == row-major U^T
                                 1,           // TP_DIM_OUT_LEADING = COL_MAJOR: A^+ in col-major
                                 kTilingEnabled,           // TP_ADD_TILING_A    - unused by the vecapi graph
                                 kTilingEnabled,           // TP_ADD_TILING_B
                                 kTilingEnabled,           // TP_ADD_DETILING_OUT
                                 kStoreCols * kStoreCols,  // TP_INPUT_WINDOW_VSIZE_A  (= TP_DIM_A * TP_DIM_AB)
                                 TP_DIM_ROWS * kStoreCols, // TP_INPUT_WINDOW_VSIZE_B  (= TP_DIM_B * TP_DIM_AB)
                                 1,                        // TP_CASC_LEN
                                 1,                        // TP_SAT
                                 1,                        // TP_SSR
                                 TT_DATA,                  // TT_OUT_DATA: output same type as inputs
                                 0u,                       // TP_CONJ_A: A operand (T = V*S^{-1}) is never conjugated
                                 kIsComplex ? 1u : 0u // TP_CONJ_B: conjugate U^T -> U^H for cfloat; no-op for float
                                 >,
        xf::dsp::aie::blas::matrix_mult::matrix_mult_graph<
            TT_DATA,                  // TT_DATA_A
            TT_DATA,                  // TT_DATA_B
            kStoreCols,               // TP_DIM_A   - rows of T = V*S^{-1}
            kStoreCols,               // TP_DIM_AB  - shared dimension
            TP_DIM_ROWS,              // TP_DIM_B   - cols of U^T (= rows of U)
            0,                        // TP_SHIFT
            0,                        // TP_RND
            1,                        // TP_DIM_A_LEADING   = COL_MAJOR: T from m_pseudoInvKernel is col-major
            0,                        // TP_DIM_B_LEADING   = ROW_MAJOR: col-major U == row-major U^T
            1,                        // TP_DIM_OUT_LEADING = COL_MAJOR: A^+ in col-major
            kTilingEnabled,           // TP_ADD_TILING_A
            kTilingEnabled,           // TP_ADD_TILING_B
            kTilingEnabled,           // TP_ADD_DETILING_OUT
            kStoreCols * kStoreCols,  // TP_INPUT_WINDOW_VSIZE_A  (= TP_DIM_A * TP_DIM_AB)
            TP_DIM_ROWS * kStoreCols, // TP_INPUT_WINDOW_VSIZE_B  (= TP_DIM_B * TP_DIM_AB)
            1,                        // TP_CASC_LEN
            1,                        // TP_SAT
            1,                        // TP_SSR
            TT_DATA,                  // TT_OUT_DATA: output same type as inputs
            0u,                       // TP_CONJ_A: A operand (T = V*S^{-1}) is never conjugated
            kIsComplex ? 1u : 0u      // TP_CONJ_B: conjugate U^T -> U^H for cfloat; no-op for float
            > >::type;

    // -----------------------------------------------------------------------
    // External ports
    // -----------------------------------------------------------------------
    /**
     * Input matrix A (TP_DIM_ROWS x TP_DIM_COLS, column-major).
     **/
    port<input> in;

    /**
     * Output pseudoinverse A^+ (kStoreCols x TP_DIM_ROWS, column-major).
     * When kStoreCols == TP_DIM_COLS the buffer holds exactly
     * TP_DIM_COLS x TP_DIM_ROWS valid elements.
     **/
    port<output> out;

    // -----------------------------------------------------------------------
    // Sub-graph and kernel instances
    // -----------------------------------------------------------------------
    /**
     * Internal SVD graph (TP_CASC_LEN = 1). TP_DIAG_INV = 1: SVD emits 1/S
     * directly (see mp_pseudoinverse_svd_graph's top-of-file comment) so
     * m_pseudoInvKernel below does not need to invert S itself.
     **/
    xf::solver::aie::svd::svd_graph<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, 1, TP_PASSES_SSR, 1 /*TP_DIAG_INV*/>
        m_svdGraph;

    /**
     * Fused S-reciprocal / V-column-scale kernel.
     * Takes S (kStoreCols floats) and V (kStoreCols x kStoreCols, col-major)
     * from SVD and emits T = V * diag(S^{-1}) (kStoreCols x kStoreCols,
     * col-major, TT_DATA) directly, without materialising the S^{-1}
     * diagonal matrix or invoking a matrix-multiply kernel.
     * TP_DIAG_INV = 1: matches m_svdGraph above -- S already holds 1/S, so
     * this kernel uses it as-is instead of inverting it again.
     **/
    kernel m_pseudoInvKernel;

    /** MatMul: T * U^H  ->  A^+  (kStoreCols x TP_DIM_ROWS, col-major) **/
    MatMul_t m_matMul;

    /**
     * Access function: returns pointer to the S^{-1} kernel.
     **/
    kernel* getKernels() { return &m_pseudoInvKernel; }

    // -----------------------------------------------------------------------
    // Constructor
    // -----------------------------------------------------------------------
    /**
     * @brief Constructor for the mp_pseudoinverse_svd graph.
     **/
    mp_pseudoinverse_svd_graph() {
        // Create the fused S-reciprocal/V-scale kernel instance.
        // TP_DIAG_INV=1: matches m_svdGraph's TP_DIAG_INV=1 above.
        m_pseudoInvKernel =
            kernel::create_object<mp_pseudoinverse_svd<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, 1 /*TP_DIAG_INV*/> >();

        // ------------------------------------------------------------------
        // Input: connect the external port to the SVD graph input.
        // ------------------------------------------------------------------
        connect(in, m_svdGraph.in[0]);

        // ------------------------------------------------------------------
        // SVD S output -> kernel in[0]
        // ------------------------------------------------------------------
        connect(m_svdGraph.outS[0], m_pseudoInvKernel.in[0]);
        dimensions(m_pseudoInvKernel.in[0]) = {kOutSWindowVsize};

        // ------------------------------------------------------------------
        // SVD V output -> kernel in[1]
        //
        // V is col-major (kStoreCols x kStoreCols) from the SVD graph.
        // The kernel scales each column of V by S^{-1}[k] directly, so no
        // separate matrix-multiply stage against a diagonal matrix is needed.
        //
        // S^{-1}[k] = 0 for k >= TP_DIM_COLS, so the corresponding columns
        // of T = V * S^{-1} are zero, and the padding rows of A^+ are
        // handled correctly by the post-processing depadding step.
        // ------------------------------------------------------------------
        connect(m_svdGraph.outV[0], m_pseudoInvKernel.in[1]);
        dimensions(m_pseudoInvKernel.in[1]) = {kOutVWindowVsize};

        // ------------------------------------------------------------------
        // Kernel output (T = V * diag(S^{-1}), col-major) -> MatMul input A
        // ------------------------------------------------------------------
        connect(m_pseudoInvKernel.out[0], m_matMul.inA[0]);
        dimensions(m_pseudoInvKernel.out[0]) = {kSInvScaledVVsize};

        // ------------------------------------------------------------------
        // SVD U output -> MatMul input B
        // U from SVD is col-major, TP_DIM_ROWS x kStoreCols.
        // Col-major U has the same byte layout as row-major U^T.
        // MatMul uses TP_DIM_B_LEADING = ROW_MAJOR -> sees U^T.
        // ------------------------------------------------------------------
        connect(m_svdGraph.outU[0], m_matMul.inB[0]);

        // ------------------------------------------------------------------
        // Output: MatMul result -> external port
        // A^+ is kStoreCols x TP_DIM_ROWS in col-major.
        // ------------------------------------------------------------------
        connect(m_matMul.out[0], out);

        // ------------------------------------------------------------------
        // Fused S-reciprocal/V-scale kernel configuration
        // ------------------------------------------------------------------
        runtime<ratio>(m_pseudoInvKernel) = 0.9;
        source(m_pseudoInvKernel) = "mp_pseudoinverse_svd.cpp";
        headers(m_pseudoInvKernel) = {"mp_pseudoinverse_svd.hpp"};
        stack_size(m_pseudoInvKernel) = 4096;
    }
};

} // namespace mp_pseudoinverse_svd
} // namespace aie
} // namespace solver
} // namespace xf

#endif // _SOLVERLIB_MP_PSEUDOINVERSE_SVD_GRAPH_HPP_
