#ifndef _SUBSTITUTION_REF_HPP_
#define _SUBSTITUTION_REF_HPP_

#include <adf.h>
#include "substitution_ref_utils.hpp"

namespace xf {
namespace solver {
namespace aie {
namespace substitution {

using namespace adf;

template <typename TT_DATA,
          unsigned int TP_DIM_SIZE,
          unsigned int TP_SUBST_TYPE,
          unsigned int TP_L_LEADING,
          unsigned int TP_GRID_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_DIAG_INV>
class substitution_ref {
   public:
    // Constructor
    substitution_ref() = default;

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(substitution_ref::substitution_main); }

    // Main kernel function
    void substitution_main(input_buffer<TT_DATA>& __restrict in_L,
                           input_buffer<TT_DATA>& __restrict in_y,
                           output_buffer<TT_DATA>& __restrict out_x);
};

} // namespace substitution
} // namespace aie
} // namespace solver
} // namespace xf

#endif
