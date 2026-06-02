#ifndef _SUBSTITUTION_REF_GRAPH_HPP_
#define _SUBSTITUTION_REF_GRAPH_HPP_

#include <adf.h>
#include "graph_utils.hpp"
#include "substitution_ref.hpp"

namespace xf {
namespace solver {
namespace aie {
namespace substitution {

using namespace adf;
using namespace xf::dsp::aie;

template <typename TT_DATA,
          unsigned int TP_DIM_SIZE,
          unsigned int TP_SUBST_TYPE,
          unsigned int TP_L_LEADING,
          unsigned int TP_GRID_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_DIAG_INV>
class substitution_ref_graph : public adf::graph {
   public:
    static constexpr unsigned int kNumKernels = TP_GRID_DIM * (TP_GRID_DIM + 1) / 2;

    kernel k;

    port_array<input, 1> L_in;
    port_array<input, 1> y_in;
    port_array<output, 1> x_out;

    substitution_ref_graph() {
        k = kernel::create_object<substitution_ref<TT_DATA, TP_DIM_SIZE, TP_SUBST_TYPE, TP_L_LEADING, TP_GRID_DIM,
                                                   TP_NUM_FRAMES, TP_DIAG_INV> >();

        connect(L_in[0], k.in[0]);
        dimensions(k.in[0]) = {TP_NUM_FRAMES * TP_DIM_SIZE * TP_DIM_SIZE};

        connect(y_in[0], k.in[1]);
        dimensions(k.in[1]) = {TP_NUM_FRAMES * TP_DIM_SIZE};

        connect(k.out[0], x_out[0]);
        dimensions(k.out[0]) = {TP_NUM_FRAMES * TP_DIM_SIZE};

        source(k) = "substitution_ref.cpp";

        runtime<adf::ratio>(k) = 0.8;
    }
};

} // namespace substitution
} // namespace aie
} // namespace solver
} // namespace xf

#endif
