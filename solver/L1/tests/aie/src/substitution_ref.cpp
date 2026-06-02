#include "adf.h"
#include "device_defs.h"
#include "../inc/substitution_ref.hpp"

namespace xf {
namespace solver {
namespace aie {
namespace substitution {

using namespace std;

template <typename TT_DATA,
          unsigned int TP_DIM_SIZE,
          unsigned int TP_SUBST_TYPE,
          unsigned int TP_L_LEADING,
          unsigned int TP_GRID_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_DIAG_INV>
void substitution_ref<TT_DATA, TP_DIM_SIZE, TP_SUBST_TYPE, TP_L_LEADING, TP_GRID_DIM, TP_NUM_FRAMES, TP_DIAG_INV>::
    substitution_main(input_buffer<TT_DATA>& __restrict in_L,
                      input_buffer<TT_DATA>& __restrict in_y,
                      output_buffer<TT_DATA>& __restrict out_x) {
    TT_DATA* L_in = in_L.data();
    TT_DATA* y_in = in_y.data();
    TT_DATA* x_out = out_x.data();
    float diagElemReal;
    float diagFactor;
    int index = 0;
    TT_DATA Lval;

    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) {
        if (TP_SUBST_TYPE == 0) {
            // Forward substitution: Solves Lx = y
            for (int i = 0; i < TP_DIM_SIZE; i++) {
                TT_DATA sum = nullElem<TT_DATA>();
                for (int j = 0; j < i; j++) {
                    index = TP_L_LEADING == 0 ? i * TP_DIM_SIZE + j : j * TP_DIM_SIZE + i;
                    sum += L_in[index] * x_out[j];
                }
                diagElemReal = fnGetReal<TT_DATA>(L_in[i * TP_DIM_SIZE + i]);
                diagFactor = TP_DIAG_INV == 0 ? (float)1.0 / diagElemReal : diagElemReal;
                if
                    constexpr(std::is_same<TT_DATA, float>::value) { x_out[i] = (y_in[i] - sum) * diagFactor; }
                else {
                    x_out[i].real = (y_in[i].real - sum.real) * diagFactor;
                    x_out[i].imag = (y_in[i].imag - sum.imag) * diagFactor;
                }
            }
        } else {
            // Backward substitution: Solves Ux = y
            for (int i = TP_DIM_SIZE - 1; i >= 0; i--) {
                TT_DATA sum = nullElem<TT_DATA>();
                for (int j = i + 1; j < TP_DIM_SIZE; j++) {
                    index = TP_L_LEADING == 1 ? i * TP_DIM_SIZE + j : j * TP_DIM_SIZE + i;
                    Lval = L_in[index];
                    if
                        constexpr(isComplex<TT_DATA>()) {
                            Lval.imag = -Lval.imag; // conjugate L matrix elements for use in backwards substitution.
                        }
                    sum += Lval * x_out[j];
                }
                diagElemReal = fnGetReal<TT_DATA>(L_in[i * TP_DIM_SIZE + i]);
                diagFactor = TP_DIAG_INV == 0 ? (float)1.0 / diagElemReal : diagElemReal;
                if
                    constexpr(std::is_same<TT_DATA, float>::value) { x_out[i] = (y_in[i] - sum) * diagFactor; }
                else {
                    x_out[i].real = (y_in[i].real - sum.real) * diagFactor;
                    x_out[i].imag = (y_in[i].imag - sum.imag) * diagFactor;
                }
            } // for i
        }     // end of backwards
        L_in += TP_DIM_SIZE * TP_DIM_SIZE;
        y_in += TP_DIM_SIZE;
        x_out += TP_DIM_SIZE;
    } // end of frame loop
}
} // namespace substitution
} // namespace aie
} // namespace solver
} // namespace xf
