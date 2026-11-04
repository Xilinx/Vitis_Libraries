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
#ifndef _SOLVERLIB_EIGENVALUE_GRAPH_HPP_
#define _SOLVERLIB_EIGENVALUE_GRAPH_HPP_

/*
The file captures the definition of the 'L2' graph level class for the
eigenvalue solver function library element.

Algorithm (QR iteration, per IP-6984 / the "AIE Solver Library Design of
Eigenvalue Solver" Confluence page): given a square input matrix A, compute
its eigenvalues via a two-stage pipeline:

  1. hessenberg graph : A -> H   (Householder reduction to upper Hessenberg
                                   form; preserves eigenvalues, makes the
                                   following QR iterations far cheaper)
  2. eig_qr graph      : H -> E   (iterative QR decomposition with a simple
                                   shift and deflation, converging H to
                                   upper triangular; the diagonal entries
                                   are the eigenvalues)

The kernels are TT_DATA-generic and TP_DIM-templated, with a user-exposed
TP_PASSES iteration cap.

Numerical note (TT_DATA=float only): TT_DATA=float is only valid for
matrices whose eigenvalues are all real (e.g. symmetric/normal matrices) --
see eig_qr.hpp for why, and the library user guide for details.

Ports:
  in  -- input matrix A (TP_DIM x TP_DIM, row-major).
  out -- eigenvalues of A (TP_DIM elements). The output is a vector, so
         no row-major/column-major concept applies to it.

Restrictions:
  TP_CASC_LEN must be 1 (parallelism across multiple kernels is reserved for
  a future release, per the design-intent page).
*/
/**
 * @file eigenvalue_graph.hpp
 *
 **/

#include <adf.h>
#include <type_traits>
#include <vector>
#include "eigenvalue_traits.hpp"
#include "hessenberg.hpp"
#include "eig_qr.hpp"

using namespace adf;

namespace xf {
namespace solver {
namespace aie {
namespace eigenvalue {

//--------------------------------------------------------------------------------------------------
// eigenvalue_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @defgroup eigenvalue_graph Eigenvalue Solver
 *
 * Eigenvalue solver graph: computes the eigenvalues of a square matrix A via
 * Hessenberg reduction followed by iterative QR with shift and deflation.
 **/

/**
 * @ingroup eigenvalue_graph
 * @brief Eigenvalues of a square matrix A, via QR iteration.
 *
 * Reduces A to upper Hessenberg form (Householder reflections), then applies
 * iterative QR decomposition with a simple shift and deflation until A
 * converges to (block-)upper-triangular form; the diagonal entries are the
 * eigenvalues of A.
 *
 * @tparam TT_DATA      Scalar data type: float or cfloat. TT_DATA=float is
 *                      only valid for matrices whose eigenvalues are all
 *                      real (e.g. symmetric/normal matrices) -- see eig_qr.hpp.
 * @tparam TP_DIM       Dimension of the square matrix A (= number of eigenvalues).
 * @tparam TP_PASSES    Maximum number of QR sweep iterations (with early
 *                      exit once all eigenvalues have deflated). More
 *                      passes improve convergence at the cost of latency.
 * @tparam TP_CASC_LEN  Must be 1.
 **/
template <typename TT_DATA, unsigned int TP_DIM, unsigned int TP_PASSES, unsigned int TP_CASC_LEN>
class eigenvalue_graph : public graph {
   public:
    // -----------------------------------------------------------------------
    // Dimension constants
    // -----------------------------------------------------------------------
    static constexpr unsigned int kVecSizeIO = fnVecSampleNumIO<TT_DATA>();
    static constexpr unsigned int kN = TP_DIM;

    // eig_qr (eig_qr.hpp) is the tighter of the two kernel tiles: it keeps 3
    // concurrent TP_DIM x TP_DIM buffers resident (the working matrix "A" --
    // aliased to its input window -- plus "Q" and "R", see eig_qr.hpp:112-113
    // / m_QStorage/m_RStorage below), versus hessenberg's 2 (input + output
    // window). Since both kernels run at the same TP_DIM, eig_qr's buffer
    // count governs the achievable maximum. Matches the sizing convention
    // used by eigenvalue.py's fn_max_dim() in L2/meta.
    static constexpr unsigned int kEigQrNumTpDimSquaredBuffers = 3;

    // Per this repo's single_buffer convention (see e.g. svd_graph.hpp):
    // TP_DIM is sized as though every buffer is single-buffered (not
    // ping-ponged) -- ping-ponging, if applied by default, is a separate
    // placement concern handled below via single_buffer() once TP_DIM is
    // fixed, not part of this limit. All of eig_qr's buffers must together
    // fit within one AIE memory bank/module (__DATA_MEM_BYTES__):
    //   kEigQrNumTpDimSquaredBuffers * TP_DIM^2 * sizeof(TT_DATA) <= __DATA_MEM_BYTES__
    static constexpr unsigned int kMaxSamplesPerBank =
        __DATA_MEM_BYTES__ / (kEigQrNumTpDimSquaredBuffers * sizeof(TT_DATA));
    static constexpr unsigned int kMaxDimCols = (isqrt(kMaxSamplesPerBank) / kVecSizeIO) * kVecSizeIO;

    static constexpr int kInWindowVsize = TP_DIM * TP_DIM; // input A
    static constexpr int kOutWindowVsize = TP_DIM;         // output E (eigenvalues)

    // ADF window ports are ping-pong (double) buffered by default. hessenberg
    // has 2 such ports (in, out) and eig_qr has 1 (in -- Q/R are plain
    // constructor-injected storage, not ADF ports, so they're never
    // ping-ponged); each kernel's worst case is therefore
    // (num_ports * 2 + num_fixed_buffers) * TP_DIM^2 * sizeof(TT_DATA), which
    // reduces to the same 4x window size for both kernels here. Whenever that
    // would overflow the bank, single_buffer() is applied (see constructor
    // below) to bring the live footprint back down to the
    // kEigQrNumTpDimSquaredBuffers-buffers-single-buffered budget kMaxDimCols
    // was sized against.
    static constexpr unsigned int kWindowBytes = kInWindowVsize * sizeof(TT_DATA);
    static constexpr bool kWindowsNeedSingleBuffer = (4 * kWindowBytes) > __DATA_MEM_BYTES__;

    // -----------------------------------------------------------------------
    // Parameter validation
    // -----------------------------------------------------------------------
    static_assert(std::is_same<TT_DATA, float>::value || std::is_same<TT_DATA, cfloat>::value,
                  "ERROR: TT_DATA must be either float or cfloat.");
    static_assert(TP_DIM % kVecSizeIO == 0,
                  "ERROR: TP_DIM must be a multiple of the internal vector size "
                  "(8 for float, 4 for cfloat).");
    static_assert(TP_DIM >= kVecSizeIO && TP_DIM <= kMaxDimCols,
                  "ERROR: TP_DIM is out of range -- see kMaxDimCols (derived from "
                  "__DATA_MEM_BYTES__ for the target AIE_VARIANT; per-tile data "
                  "memory limit).");
    static_assert(TP_PASSES >= 1 && TP_PASSES <= 100, "ERROR: TP_PASSES must be in the range [1, 100].");
    static_assert(TP_CASC_LEN == 1,
                  "ERROR: eigenvalue_graph currently supports TP_CASC_LEN = 1 only; parallelism across "
                  "multiple kernels is reserved for a future release.");

    // -----------------------------------------------------------------------
    // External ports
    // -----------------------------------------------------------------------
    /**
     * Input matrix A (TP_DIM x TP_DIM, row-major).
     **/
    port<input> in;

    /**
     * Output eigenvalues of A (TP_DIM elements).
     **/
    port<output> out;

    // -----------------------------------------------------------------------
    // Kernel instances
    // -----------------------------------------------------------------------
    /** Stage 1: Hessenberg reduction (A -> H). **/
    kernel m_hessenbergKernel;

    /** Stage 2: Iterative QR eigenvalue extraction (H -> E). **/
    kernel m_eigQrKernel;

    // Q/R scratch buffers for m_eigQrKernel, constructor-injected as
    // vector-backed storage (Q_storage/R_storage bound to the kernel's
    // array-reference constructor parameters via kernel::create_object).
    std::vector<TT_DATA> m_QStorage;
    std::vector<TT_DATA> m_RStorage;

    /**
     * Access function: returns pointer to the first kernel (hessenberg).
     **/
    kernel* getKernels() { return &m_hessenbergKernel; }

    // -----------------------------------------------------------------------
    // Constructor
    // -----------------------------------------------------------------------
    /**
     * @brief Constructor for the eigenvalue graph.
     **/
    eigenvalue_graph() : m_QStorage(TP_DIM * TP_DIM), m_RStorage(TP_DIM * TP_DIM) {
        // Create the Hessenberg-reduction kernel instance.
        m_hessenbergKernel = kernel::create_object<hessenberg<TT_DATA, TP_DIM> >();

        // Create the QR eigenvalue-extraction kernel instance.
        m_eigQrKernel = kernel::create_object<eig_qr<TT_DATA, TP_DIM, TP_PASSES> >(m_QStorage, m_RStorage);

        // ------------------------------------------------------------------
        // Input: A -> Hessenberg kernel.
        // ------------------------------------------------------------------
        connect(in, m_hessenbergKernel.in[0]);
        dimensions(m_hessenbergKernel.in[0]) = {kInWindowVsize};

        // ------------------------------------------------------------------
        // Hessenberg kernel output H -> QR kernel input.
        // ------------------------------------------------------------------
        connect(m_hessenbergKernel.out[0], m_eigQrKernel.in[0]);
        dimensions(m_hessenbergKernel.out[0]) = {kInWindowVsize};
        dimensions(m_eigQrKernel.in[0]) = {kInWindowVsize};

        // ------------------------------------------------------------------
        // Output: QR kernel result E -> external port.
        // ------------------------------------------------------------------
        connect(m_eigQrKernel.out[0], out);
        dimensions(m_eigQrKernel.out[0]) = {kOutWindowVsize};

        // ------------------------------------------------------------------
        // Memory placement: force single-buffering on the TP_DIM x TP_DIM
        // window ports once default ping-pong buffering would no longer fit
        // within one AIE memory bank alongside eig_qr's Q/R storage -- see
        // kWindowsNeedSingleBuffer / kMaxDimCols above. Mirrors the
        // single_buffer() convention used by svd_graph.hpp et al.
        // ------------------------------------------------------------------
        if (kWindowsNeedSingleBuffer) {
            single_buffer(m_hessenbergKernel.in[0]);
            single_buffer(m_hessenbergKernel.out[0]);
            single_buffer(m_eigQrKernel.in[0]);
        }

        // ------------------------------------------------------------------
        // Kernel configuration
        // ------------------------------------------------------------------
        runtime<ratio>(m_hessenbergKernel) = 0.9;
        source(m_hessenbergKernel) = "hessenberg.cpp";
        headers(m_hessenbergKernel) = {"hessenberg.hpp"};
        stack_size(m_hessenbergKernel) = 4096;

        runtime<ratio>(m_eigQrKernel) = 0.9;
        source(m_eigQrKernel) = "eig_qr.cpp";
        headers(m_eigQrKernel) = {"eig_qr.hpp"};
        stack_size(m_eigQrKernel) = 4096;
    }
};

} // namespace eigenvalue
} // namespace aie
} // namespace solver
} // namespace xf

#endif // _SOLVERLIB_EIGENVALUE_GRAPH_HPP_
