#ifndef _SOLVERLIB_CASTKERNEL_HPP_
#define _SOLVERLIB_CASTKERNEL_HPP_

#include <adf.h>
#include "aie_api/aie_adf.hpp"
#include "aie_api/aie.hpp"
#include "device_defs.h"

using namespace adf;

namespace xf::solver::aie::kernel_cast {

template <unsigned int TP_DIM_SIZE,
unsigned int TP_NUM_FRAMES>
class castKernel {
   private:

   public:
     // Constructor
     castKernel() = default;

    // Main kernel function
     void cast_main(input_buffer<cfloat>& __restrict in,
                    output_buffer<float>& __restrict out);

     // Register Kernel Class
     static void registerKernelClass() {
         REGISTER_FUNCTION(castKernel::cast_main);
     }
};

} // namespace xf::solver::aie::kernel_cast

#endif // _SOLVERLIB_CASTKERNEL_HPP_
