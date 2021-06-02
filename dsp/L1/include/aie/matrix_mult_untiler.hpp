#ifndef _DSPLIB_MATRIX_MULT_UNTILER_HPP_
#define _DSPLIB_MATRIX_MULT_UNTILER_HPP_

#include <adf.h>

namespace xf {
namespace dsp {
namespace aie {
namespace blas {
namespace matrix_mult {
/*
* @brief Acts as a wrapper and the entry point from the graph.
*/
template <unsigned M, unsigned N, unsigned inRow, unsigned inCol, unsigned leadingDim, typename T_D>
class untilerKernelClass {
   public:
    void unTile(input_window<T_D>* inWindow, output_window<T_D>* restrict outWindow);

    static void registerKernelClass() { REGISTER_FUNCTION(untilerKernelClass::unTile); }
};
}
}
}
}
}

#endif