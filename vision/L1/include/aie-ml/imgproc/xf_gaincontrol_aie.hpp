/*
 * Copyright 2021 Xilinx, Inc.
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

#include <adf.h>
#include <aie_api/aie.hpp>
#include <common/xf_aie_hw_utils.hpp>

#ifndef _AIE_GAINCONTROL_H_
#define _AIE_GAINCONTROL_H_

namespace xf {
namespace cv {
namespace aie {
template <typename T, int N>
__attribute__((noinline)) auto compute_gain_vector_even(const uint8_t& gain) {
    ::aie::vector<T, N> coeff;
    ::aie::vector<uint8_t, 16> coeff0 = ::aie::broadcast<uint8_t, 16>(gain);
    ::aie::vector<uint8_t, 16> coeff1 = ::aie::broadcast<uint8_t, 16>((1 << 6));
    auto[v1, v2] = ::aie::interleave_zip(coeff0, coeff1, 1);
    coeff = ::aie::concat(v1, v2);

    return coeff;
}

template <typename T, int N>
__attribute__((noinline)) auto compute_gain_vector_odd(const uint8_t& gain) {
    ::aie::vector<T, N> coeff;
    ::aie::vector<uint8_t, 16> coeff0 = ::aie::broadcast<uint8_t, 16>((1 << 6));
    ::aie::vector<uint8_t, 16> coeff1 = ::aie::broadcast<uint8_t, 16>(gain);
    auto[v1, v2] = ::aie::interleave_zip(coeff0, coeff1, 1);
    coeff = ::aie::concat(v1, v2);

    return coeff;
}

template <typename T, int N, int code>
class ComputeGainVector {};

template <typename T, int N>
class ComputeGainVector<T, N, 0> {
   public:
    // code == 0 : RG
    static inline void compute_gain_kernel_coeff(const uint8_t& rgain,
                                                 const uint8_t& bgain,
                                                 ::aie::vector<T, N>& coeff0,
                                                 ::aie::vector<T, N>& coeff1) {
        coeff0 = compute_gain_vector_even<T, N>(rgain);
        coeff1 = compute_gain_vector_odd<T, N>(bgain);
    }
};

template <typename T, int N>
class ComputeGainVector<T, N, 1> {
   public:
    // code == 1 : GR
    static inline void compute_gain_kernel_coeff(const uint8_t& rgain,
                                                 const uint8_t& bgain,
                                                 ::aie::vector<T, N>& coeff0,
                                                 ::aie::vector<T, N>& coeff1) {
        coeff0 = compute_gain_vector_odd<T, N>(rgain);
        coeff1 = compute_gain_vector_even<T, N>(bgain);
    }
};

template <typename T, int N>
class ComputeGainVector<T, N, 2> {
   public:
    // code == 2 : BG
    static inline void compute_gain_kernel_coeff(const uint8_t& rgain,
                                                 const uint8_t& bgain,
                                                 ::aie::vector<T, N>& coeff0,
                                                 ::aie::vector<T, N>& coeff1) {
        coeff0 = compute_gain_vector_even<T, N>> (bgain);
        coeff1 = compute_gain_vector_odd<T, N>(rgain);
    }
};

template <typename T, int N>
class ComputeGainVector<T, N, 3> {
   public:
    // code == 3 : GB
    static inline void compute_gain_kernel_coeff(const uint8_t& rgain,
                                                 const uint8_t& bgain,
                                                 ::aie::vector<T, N>& coeff0,
                                                 ::aie::vector<T, N>& coeff1) {
        coeff0 = compute_gain_vector_odd<T, N>(bgain);
        coeff1 = compute_gain_vector_even<T, N>(rgain);
    }
};

template <typename T, int N, int code, typename OUT_T>
inline void gaincontrol(const T* restrict img_in,
                        T* restrict img_out,
                        int image_width,
                        int image_height,
                        const ::aie::vector<T, N>& coeff0,
                        const ::aie::vector<T, N>& coeff1) {
    auto it = ::aie::begin_vector<N>(img_in);
    auto out = ::aie::begin_vector<N>(img_out);
    set_sat();
    for (int i = 0; i < image_height / 2; i++) chess_prepare_for_pipelining {
            for (int j = 0; j < image_width; j += N) // even rows
                chess_prepare_for_pipelining { *out++ = ::aie::mul(coeff0, *it++).template to_vector<T>(6); }
            for (int j = 0; j < image_width; j += N) // odd rows
                chess_prepare_for_pipelining { *out++ = ::aie::mul(coeff1, *it++).template to_vector<T>(6); }
        }
}

template <int code>
void gaincontrol_api(input_window_uint8* img_in,
                     output_window_uint8* img_out,
                     const uint8_t& rgain,
                     const uint8_t& bgain) {
    uint8_t* img_in_ptr = (uint8_t*)img_in->ptr;
    uint8_t* img_out_ptr = (uint8_t*)img_out->ptr;

    const int img_width = xfGetTileWidth(img_in_ptr);
    const int img_height = xfGetTileHeight(img_in_ptr);

    const uint16_t posH = xfGetTilePosH(img_in_ptr);
    const uint16_t posV = xfGetTilePosV(img_in_ptr);

    xfCopyMetaData(img_in_ptr, img_out_ptr);
    xfUnsignedSaturation(img_out_ptr);

    uint8_t* in_ptr = (uint8_t*)xfGetImgDataPtr(img_in_ptr);
    uint8_t* out_ptr = (uint8_t*)xfGetImgDataPtr(img_out_ptr);

    ::aie::vector<uint8_t, 32> coeff0;
    ::aie::vector<uint8_t, 32> coeff1;

    if (posH % 2 == 0) {
        if (posV % 2 == 0) {
            ComputeGainVector<uint8_t, 32, code>::compute_gain_kernel_coeff(rgain, bgain, coeff0, coeff1);
        } else {
            ComputeGainVector<uint8_t, 32, code>::compute_gain_kernel_coeff(rgain, bgain, coeff1, coeff0);
        }
    } else {
        if (posV % 2 == 0) {
            ComputeGainVector<uint8_t, 32, code>::compute_gain_kernel_coeff(bgain, rgain, coeff0, coeff1);
        } else {
            ComputeGainVector<uint8_t, 32, code>::compute_gain_kernel_coeff(bgain, rgain, coeff1, coeff0);
        }
    }
    gaincontrol<uint8_t, 32, code, uint8_t>(in_ptr, out_ptr, img_width, img_height, coeff0, coeff1);
}

} // aie
} // cv
} // xf
#endif
