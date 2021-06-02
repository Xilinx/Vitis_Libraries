#ifndef _DSPLIB_MATRIX_MULT_TILER_HPP_
#define _DSPLIB_MATRIX_MULT_TILER_HPP_

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
class tilerKernelClass {
   public:
    void tile(input_window<T_D>* inWindow, output_window<T_D>* restrict outWindow);

    static void registerKernelClass() { REGISTER_FUNCTION(tilerKernelClass::tile); }
};
/*
  @brief Entry point from another kernel (using this as a function, instead of a subgraph.)
*/
// template<unsigned M, unsigned N, unsigned inRow, unsigned inCol, unsigned leadingDim, typename T_D >
// static void doTile(T_D* restrict inPtr, T_D* outPtr);
}
}
}
}
}

#endif