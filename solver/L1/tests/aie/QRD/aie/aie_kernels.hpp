#ifndef _AIE_KERNELS_HPP_
#define _AIE_KERNELS_HPP_

#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include "adf/stream/streams.h"
#include <adf.h>

#define ELEM_PER_ITER 8
#define VEC_NUM 4
#define ALLIGN_NUM (ELEM_PER_ITER * VEC_NUM)

void givens_rotation(input_stream<float>* lower_row,
                     input_stream<float>* higher_row,
                     output_stream<float>* update_lower_row,
                     output_stream<float>* update_higher_row,
                     unsigned int row_num,
                     unsigned int column_num);

#endif
