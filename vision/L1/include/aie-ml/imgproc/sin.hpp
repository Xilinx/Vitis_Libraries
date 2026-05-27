#ifndef __XF_SIN_
#define __XF_SIN_

#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/utils.hpp>
#include <common/xf_aie_hw_utils.hpp>
#include <type_traits>

alignas(::aie::vector_decl_align) int32_t coeffs[16] = {
    67105182, -11178610, 556320, -12750,
    136}; //{268420400, -44714204, 2225239, -50998,544};//{1046858, -172858, 8160, -141}; // NO_COEFFS in config.h=6

namespace xf {
namespace cv {
namespace aie {

template <int NO_COEFFS, int NO_BOXES_PER_TILE>
class Sin1 {
   public:
    void sin_runImpl_api(adf::input_buffer<int32_t>& ptr_img_in, adf::output_buffer<int32_t>& output);
    void sin_runImpl(int32_t* restrict ptr_img_in, int32_t* restrict output);
};

template <int NO_COEFFS, int NO_BOXES_PER_TILE>
void Sin1<NO_COEFFS, NO_BOXES_PER_TILE>::sin_runImpl_api(adf::input_buffer<int32_t>& ptr_img_in,
                                                         adf::output_buffer<int32_t>& output) {
    int32_t* restrict img_in_ptr = (int32_t*)::aie::begin(ptr_img_in);
    int32_t* restrict img_out_ptr = (int32_t*)::aie::begin(output);

    sin_runImpl(img_in_ptr, img_out_ptr);
}

template <int NO_COEFFS, int NO_BOXES_PER_TILE>
__attribute__((always_inline)) void Sin1<NO_COEFFS, NO_BOXES_PER_TILE>::sin_runImpl(int32_t* restrict ptr_img_in,
                                                                                    int32_t* restrict output) {
    // Load input
    ::aie::vector<int32_t, 32> x1, x1_square, vacc, vcoeff;
    ::aie::accum<acc64, 32> acc;

    for (int j = 0; j < num_elems_PER_TILE / 32; j++) chess_prepare_for_pipelining {
            vacc = ::aie::broadcast<int32_t, 32>(coeffs[NO_COEFFS - 2]);
            vcoeff = ::aie::broadcast<int32_t, 32>(coeffs[NO_COEFFS - 1]);
            ::aie::accum<acc64, 32> acc;
            x1 = ::aie::load_v<32>(ptr_img_in);
            ptr_img_in += 32;

            acc = ::aie::mul(x1, x1);
            x1_square = acc.to_vector<int32_t>(26);
            acc.from_vector(vacc, 26);

            acc = ::aie::mac(acc, x1_square, vcoeff);

            for (int i = NO_COEFFS - 3; i >= 0; i--) chess_prepare_for_pipelining chess_unroll_loop() {
                    vacc = acc.to_vector<int32_t>(26);
                    vcoeff = ::aie::broadcast<int32_t, 32>(coeffs[i]);
                    acc.from_vector(vcoeff, 26);
                    acc = ::aie::mac(acc, x1_square, vacc);
                }
            vacc = acc.to_vector<int32_t>(26);
            acc = ::aie::mul(vacc, x1);
            vacc = acc.to_vector<int32_t>(26);
            ::aie::store_v(output, vacc);
            output += 32;
        }
}

} // aie
} // cv
} // xf
#endif