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

#ifndef __XF_DEMOSAICING_IMPL_HPP__
#define __XF_DEMOSAICING_IMPL_HPP__

#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/utils.hpp>
#include <common/xf_aie_hw_utils.hpp>

namespace xf {
namespace cv {
namespace aie {

/* In each iteration 32 output lanes are computes
 * For RGGB bayer pattern
 *     Even row computes R at Gr - 16 lanes
 *                       R at R  - 16 lanes
 *     Odd row computes  R at Gb - 16 lanes
 *                       R at B  - 16 lanes
*/

/**
 *
 *   _____________________________
 *  |  |                       |  |
 *  |6_|__________2____________|7_|  first row
 *  |  |                       |  |
 *  |  |                       |  |
 *  |  |                       |  |
 *  |4 |          1            |5 |
 *  |  |                       |  |
 *  |  |                       |  |
 *  |__|_______________________|__|
 *  |  |                       |  |
 *  |8_|__________3____________|9_|  last row
 *
 */

#define MACRO_SLIDING_MUL(acc, coeff, data_buf, c_s, d_s, Lanes, Points, CoeffStep, DataStepX, DataStepY)          \
    acc = ::aie::sliding_mul<Lanes, Points, (kmap[CoeffStep] - kmap[c_s]), DataStepX, DataStepY>(coeff, kmap[c_s], \
                                                                                                 data_buf, d_s);

#define MACRO_SLIDING_MAC(acc, coeff, data_buf, c_s, d_s, Lanes, Points, CoeffStep, DataStepX, DataStepY) \
    acc = ::aie::sliding_mac<Lanes, Points, (kmap[CoeffStep] - kmap[c_s]), DataStepX, DataStepY>(         \
        acc, coeff, kmap[c_s], data_buf, d_s);

#define MACRO_MUL(acc, coeff, data_buf, c_s) acc = ::aie::mul(data_buf, coeff[kmap[c_s]]);
#define MACRO_MAC(acc, coeff, data_buf, c_s) acc = ::aie::mac(acc, data_buf, coeff[kmap[c_s]]);

/*Coeff arrangement
     *  k0    k1    k2    k3    k4
     *  k4    k6    k7    k8    k9
     * k10   k11   k12   k13   k14
     * k15   k16   k17   k18   k19
     * k20   k21   k22   k23   k24
*/

/*NOTE: AIE Coefficient vectors are populated only for Diamond shape region
     *              k2
     *        k6    k7    k8
     * k10   k11   k12   k13   k14
     *       k16   k17   k18
     *             k22
*/

// Coefficient vector for default cases (i.e. Red at Red / Blue at Blue / Green at Green)
/*
     //Values
     *  0     0	    0	  0	    0
     *  0     0     0     0     0
     *  0     0     1     0     0
     *  0     0     0     0     0
     *  0     0     0     0     0
*/
template <DemosaicType _t, int SHIFT>
auto getCoefficient() {
    alignas(16) static constexpr int16_t coeff[16] = {
        0,            //      0
        (0 << SHIFT), // k2    4
        (0 << SHIFT), // k6    1
        (0 << SHIFT), // k7    2
        (0 << SHIFT), // k8    3
        (0 << SHIFT), // k10   5
        (0 << SHIFT), // k11   6
        (1 << SHIFT), // k12   7
        (0 << SHIFT), // k13   8
        (0 << SHIFT), // k14   9
        (0 << SHIFT), // k16  10
        (0 << SHIFT), // k17  11
        (0 << SHIFT), // k18  12
        (0 << SHIFT), // k22  13
        0,            //     14
        0             //     15
    };
    return coeff;
}

// Coefficient specializations
// Coefficient vector for Red at Green_Red / Blue at Green_Blue
/*
     //Values
     *  0     0	   1/2	  0	    0
     *  0    -1     0    -1     0
     * -1     4     5     4    -1
     *  0    -1     0    -1     0
     *  0     0    1/2    0     0
*/
template <DemosaicType _t, int SHIFT>
requires((_t == DemosaicType::Red_At_Green_Red) || (_t == DemosaicType::Blue_At_Green_Blue)) auto getCoefficient() {
    alignas(16) static constexpr int16_t coeff[16] = {
        0,                //      0
        (1 << SHIFT) / 2, // k2    4
        -(1 << SHIFT),    // k6    1
        (0 << SHIFT),     // k7    2
        -(1 << SHIFT),    // k8    3
        -(1 << SHIFT),    // k10   5
        (4 << SHIFT),     // k11   6
        (5 << SHIFT),     // k12   7
        (4 << SHIFT),     // k13   8
        -(1 << SHIFT),    // k14   9
        -(1 << SHIFT),    // k16  10
        (0 << SHIFT),     // k17  11
        -(1 << SHIFT),    // k18  12
        (1 << SHIFT) / 2, // k22  13
        0,                //     14
        0                 //     15
    };
    return coeff;
}

// Coefficient vector for Red at Blue / Blue at Red
/*
     //Values
     *  0     0	  -3/2	  0	    0
     *  0     2     0     2     0
     *-3/2    0     6     0   -3/2
     *  0     2     0     2     0
     *  0     0   -3/2    0     0
*/
template <DemosaicType _t, int SHIFT>
requires((_t == DemosaicType::Red_At_Blue) || (_t == DemosaicType::Blue_At_Red)) auto getCoefficient() {
    alignas(16) static constexpr int16_t coeff[16] = {
        0,                 //      0
        -(3 << SHIFT) / 2, // k2    4
        (2 << SHIFT),      // k6    1
        (0 << SHIFT),      // k7    2
        (2 << SHIFT),      // k8    3
        -(3 << SHIFT) / 2, // k10   5
        (0 << SHIFT),      // k11   6
        (6 << SHIFT),      // k12   7
        (0 << SHIFT),      // k13   8
        -(3 << SHIFT) / 2, // k14   9
        (2 << SHIFT),      // k16  10
        (0 << SHIFT),      // k17  11
        (2 << SHIFT),      // k18  12
        -(3 << SHIFT) / 2, // k22  13
        0,                 //     14
        0                  //     15
    };
    return coeff;
}

// Coefficient vector for Red at Green_Blue / Blue at Green_Red
/*
     //Values
     *  0     0	   -1	  0	    0
     *  0    -1     4    -1     0
     * 1/2    0     5     0    1/2
     *  0    -1     4    -1     0
     *  0     0    -1     0     0
*/
template <DemosaicType _t, int SHIFT>
requires((_t == DemosaicType::Red_At_Green_Blue) || (_t == DemosaicType::Blue_At_Green_Red)) auto getCoefficient() {
    alignas(16) static constexpr int16_t coeff[16] = {
        0,                //      0
        -(1 << SHIFT),    // k2    4
        -(1 << SHIFT),    // k6    1
        (4 << SHIFT),     // k7    2
        -(1 << SHIFT),    // k8    3
        (1 << SHIFT) / 2, // k10   5
        (0 << SHIFT),     // k11   6
        (5 << SHIFT),     // k12   7
        (0 << SHIFT),     // k13   8
        (1 << SHIFT) / 2, // k14   9
        -(1 << SHIFT),    // k16  10
        (4 << SHIFT),     // k17  11
        -(1 << SHIFT),    // k18  12
        -(1 << SHIFT),    // k22  13
        0,                //     14
        0                 //     15
    };
    return coeff;
}

// Coefficient vector for Green at Red / Green at Blue
/*
     //Values
     *  0     0	   -1	  0	    0
     *  0     0     2     0     0
     * -1     2     4     2    -1
     *  0     0     2     0     0
     *  0     0    -1     0     0
*/
template <DemosaicType _t, int SHIFT>
requires((_t == DemosaicType::Green_At_Blue) || (_t == DemosaicType::Green_At_Red)) auto getCoefficient() {
    alignas(16) static constexpr int16_t coeff[16] = {
        0,             //      0
        -(1 << SHIFT), // k2    4
        (0 << SHIFT),  // k6    1
        (2 << SHIFT),  // k7    2
        (0 << SHIFT),  // k8    3
        -(1 << SHIFT), // k10   5
        (2 << SHIFT),  // k11   6
        (4 << SHIFT),  // k12   7
        (2 << SHIFT),  // k13   8
        -(1 << SHIFT), // k14   9
        (0 << SHIFT),  // k16  10
        (2 << SHIFT),  // k17  11
        (0 << SHIFT),  // k18  12
        -(1 << SHIFT), // k22  13
        0,             //     14
        0              //     15
    };
    return coeff;
}

/* Coefficient map is compressed to exclude 0 entries (Example : k0, k1, k3, k4 etc.. in above convolution)
*/
static constexpr int8_t kmap[26] = {
    // Location
    0,  //  k0
    0,  //  k1
    1,  //  k2
    0,  //  k3
    0,  //  k4
    0,  //  k5
    2,  //  k6
    3,  //  k7
    4,  //  k8
    0,  //  k9
    5,  // k10
    6,  // k11
    7,  // k12
    8,  // k13
    9,  // k14
    0,  // k15
    10, // k16
    11, // k17
    12, // k18
    0,  // k19
    0,  // k20
    0,  // k21
    13, // k22
    0,  // k23
    0,  // k24
    0   // Pad
};

template <BayerPattern b, Channel c, int loc>
static constexpr bool compute_at() {
    return ((b & (0x0003 << (2 * loc))) != (c << (2 * loc)));
}

template <BayerPattern b, Channel c, int loc>
static constexpr DemosaicType getCoefficientKey() {
    constexpr Channel c0 = c;
    constexpr Channel c1 = static_cast<Channel>((b & (0x0003 << (2 * loc))) >> (2 * loc));
    if
        constexpr(c1 == G) {
            int nloc = 0;
            if
                constexpr(loc == 0) nloc = 1;
            if
                constexpr(loc == 1) nloc = 0;
            if
                constexpr(loc == 2) nloc = 3;
            if
                constexpr(loc == 3) nloc = 2;
            Channel c2 = static_cast<Channel>((b & (0x0003 << (2 * nloc))) >> (2 * nloc));
            return static_cast<DemosaicType>((c2 << 4) + (c1 << 2) + (c0 << 0));
        }
    else {
        return static_cast<DemosaicType>((c1 << 2) + (c0 << 0));
    }
}

template <BayerPattern b, Channel c, int loc, int SHIFT>
auto getCoefficient() {
    return getCoefficient<getCoefficientKey<b, c, loc>(), SHIFT>();
}

template <BayerPattern b, int loc>
static constexpr BayerPattern getRelativeBayerPattern() {
    int u = (b & (0x0003 << 0)) >> 0;
    int v = (b & (0x0003 << 2)) >> 2;
    int w = (b & (0x0003 << 4)) >> 4;
    int x = (b & (0x0003 << 6)) >> 6;

    if
        constexpr(loc == 1) { return static_cast<BayerPattern>((v << 0) + (u << 2) + (x << 4) + (w << 6)); }

    if
        constexpr(loc == 2) { return static_cast<BayerPattern>((w << 0) + (x << 2) + (u << 4) + (v << 6)); }

    if
        constexpr(loc == 3) { return static_cast<BayerPattern>((x << 0) + (w << 2) + (v << 4) + (u << 6)); }

    return b;
}

template <int INPUT_TILE_ELEMENTS>
__attribute__((noinline)) void DemosaicBaseImpl<INPUT_TILE_ELEMENTS>::xf_demosaic_interleave_input(
    int16_t* in_ptr, int16_t* out_ptr_e, int16_t* out_ptr_o, const int16_t& image_width, const int16_t& image_height) {
    {
        int16_t* restrict lptr0 = in_ptr;
        for (int j = 0; j < image_width; j += 32) chess_prepare_for_pipelining chess_loop_range(1, ) {
                auto a = ::aie::load_v<16>(lptr0);
                auto b = ::aie::load_v<16>((int16_t*)(lptr0 + 16));
                std::tie(a, b) = ::aie::interleave_unzip(a, b, 1);
                ::aie::store_v(out_ptr_e, a);
                ::aie::store_v((int16_t*)(out_ptr_e + 16), b);
                ::aie::store_v(out_ptr_o, a);
                ::aie::store_v((int16_t*)(out_ptr_o + 16), b);

                lptr0 += 32;
                out_ptr_e += 32;
                out_ptr_o += 32;
            }
    }

    {
        int16_t* restrict lptr0 = in_ptr;
        int16_t* restrict lptr1 = (int16_t*)(in_ptr + image_width);
        for (int i = 0; i < image_height; i += 2) {
            for (int j = 0; j < image_width; j += 32) chess_prepare_for_pipelining chess_loop_range(1, ) {
                    auto a = ::aie::load_v<16>(lptr0);
                    auto b = ::aie::load_v<16>((int16_t*)(lptr0 + 16));
                    std::tie(a, b) = ::aie::interleave_unzip(a, b, 1);
                    ::aie::store_v(out_ptr_e, a);
                    ::aie::store_v((int16_t*)(out_ptr_e + 16), b);

                    a = ::aie::load_v<16>(lptr1);
                    b = ::aie::load_v<16>((int16_t*)(lptr1 + 16));
                    std::tie(a, b) = ::aie::interleave_unzip(a, b, 1);
                    ::aie::store_v(out_ptr_o, a);
                    ::aie::store_v((int16_t*)(out_ptr_o + 16), b);

                    lptr0 += 32;
                    lptr1 += 32;
                    out_ptr_e += 32;
                    out_ptr_o += 32;
                }
            lptr0 += image_width;
            lptr1 += image_width;
        }
    }

    {
        int16_t* restrict lptr0 = (int16_t*)(in_ptr + (image_height - 1) * image_width);
        for (int j = 0; j < image_width; j += 32) chess_prepare_for_pipelining chess_loop_range(1, ) {
                auto a = ::aie::load_v<16>(lptr0);
                auto b = ::aie::load_v<16>((int16_t*)(lptr0 + 16));
                std::tie(a, b) = ::aie::interleave_unzip(a, b, 1);
                ::aie::store_v(out_ptr_e, a);
                ::aie::store_v((int16_t*)(out_ptr_e + 16), b);
                ::aie::store_v(out_ptr_o, a);
                ::aie::store_v((int16_t*)(out_ptr_o + 16), b);

                lptr0 += 32;
                out_ptr_e += 32;
                out_ptr_o += 32;
            }
    }
}

template <int INPUT_TILE_ELEMENTS>
template <bool beven, bool bodd>
__attribute__((noinline)) void DemosaicBaseImpl<INPUT_TILE_ELEMENTS>::xf_demosaic_row(int16_t* img_in_e,
                                                                                      int16_t* img_in_o,
                                                                                      int16_t* img_out,
                                                                                      int16_t image_width,
                                                                                      int16_t image_height,
                                                                                      int16_t stride_out,
                                                                                      const int16_t* coeffpe,
                                                                                      const int16_t* coeffpo) {
    int16_t* lptre0 = (int16_t*)(img_in_e);
    int16_t* lptre1 = (int16_t*)(img_in_o);
    int16_t* lptre2 = (int16_t*)(img_in_e + image_width);
    int16_t* lptre3 = (int16_t*)(img_in_o + image_width);
    int16_t* lptre4 = (int16_t*)(img_in_e + 2 * image_width);

    int16_t* lptro0 = (int16_t*)(img_in_e + 16);
    int16_t* lptro1 = (int16_t*)(img_in_o + 16);
    int16_t* lptro2 = (int16_t*)(img_in_e + image_width + 16);
    int16_t* lptro3 = (int16_t*)(img_in_o + image_width + 16);
    int16_t* lptro4 = (int16_t*)(img_in_e + 2 * image_width + 16);

    int16_t* lptr_out = img_out;

    auto coeffe = ::aie::load_v<16>(coeffpe);
    auto coeffo = ::aie::load_v<16>(coeffpo);

    for (int16_t i = 0; i < (image_height >> 1); i++) chess_prepare_for_pipelining chess_loop_range(1, ) {
            int16_t* data_outp = lptr_out;
            // Left region
            //@{
            {
                ::aie::accum<acc32, 16> acce;
                ::aie::accum<acc32, 16> acco;
                //@Convolution Row2 {
                {
                    ::aie::vector<int16_t, 32> data_buf;
                    data_buf.insert(0, ::aie::load_v<16>(lptre1));
                    data_buf.insert(1, ::aie::load_v<16>(lptro1));

                    if
                        constexpr(beven) {
                            // k7:k8
                            // c_s = 7, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 8, DataStepX = 16, DataStepY =
                            // 1
                            MACRO_SLIDING_MUL(acce, coeffe, data_buf, 7, 0, 16, 2, 8, 16, 1)
                        }

                    if
                        constexpr(bodd) {
                            // k6:k7
                            // c_s = 6, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 7, DataStepX = 16, DataStepY =
                            // 1
                            MACRO_SLIDING_MUL(acco, coeffo, data_buf, 6, 0, 16, 2, 7, 16, 1)
                        }

                    if
                        constexpr(beven) {
                            //-:k6
                            // c_s = 25, d_s = 14, Lanes = 16, Points = 2, CoeffStep = 6, DataStepX = 1, DataStepY =
                            // 1
                            int16_t a = data_buf[15];
                            data_buf[15] = data_buf[0];
                            MACRO_SLIDING_MAC(acce, coeffe, data_buf, 25, 14, 16, 2, 6, 1, 1)
                            data_buf[15] = a;
                        }

                    if
                        constexpr(bodd) {
                            //-:k8
                            data_buf.insert(1, ::aie::load_v<16>((int16_t*)(lptro1 + 16)));
                            // c_s = 25, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 8, DataStepX = 1, DataStepY =
                            // 1
                            MACRO_SLIDING_MAC(acco, coeffo, data_buf, 25, 0, 16, 2, 8, 1, 1)
                        }
                }
                //@}

                //@Convolution Row3 {
                if
                    constexpr(beven) {
                        ::aie::vector<int16_t, 32> data_buf;
                        data_buf.insert(0, ::aie::load_v<16>(lptre2));
                        data_buf.insert(1, ::aie::load_v<16>(lptro2));

                        // k12:k13
                        // c_s = 12, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 13, DataStepX = 16, DataStepY = 1
                        MACRO_SLIDING_MAC(acce, coeffe, data_buf, 12, 0, 16, 2, 13, 16, 1)

                        // k10:k11
                        int16_t a = data_buf[0];
                        data_buf = data_buf.push(a);
                        data_buf[16] = a;
                        // c_s = 10, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 11, DataStepX = 16, DataStepY = 1
                        MACRO_SLIDING_MAC(acce, coeffe, data_buf, 10, 0, 16, 2, 11, 16, 1)

                        //-:k14
                        data_buf.insert(0, ::aie::load_v<16>(lptre2));
                        data_buf.insert(1, ::aie::load_v<16>((int16_t*)(lptro2 + 16)));
                        // c_s = 25, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 14, DataStepX = 1, DataStepY = 1
                        MACRO_SLIDING_MAC(acce, coeffe, data_buf, 25, 0, 16, 2, 14, 1, 1)
                    }
                //@}

                //@Convolution Row3 {
                if
                    constexpr(bodd) {
                        ::aie::vector<int16_t, 32> data_buf;
                        data_buf.insert(0, ::aie::load_v<16>(lptre2));
                        data_buf.insert(1, ::aie::load_v<16>(lptro2));

                        // k11:k12
                        // c_s = 11, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 12, DataStepX = 16, DataStepY = 1
                        MACRO_SLIDING_MAC(acco, coeffo, data_buf, 11, 0, 16, 2, 12, 16, 1)

                        //-:k10
                        data_buf[15] = data_buf[0];
                        // c_s = 25, d_s = 14, Lanes = 16, Points = 2, CoeffStep = 10, DataStepX = 1, DataStepY = 1
                        MACRO_SLIDING_MAC(acco, coeffo, data_buf, 25, 14, 16, 2, 10, 1, 1)

                        //-:k13
                        data_buf.insert(0, ::aie::load_v<16>(lptre2));
                        data_buf.insert(1, ::aie::load_v<16>((int16_t*)(lptro2 + 16)));
                        // c_s = 25, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 13, DataStepX = 1, DataStepY = 1
                        MACRO_SLIDING_MAC(acco, coeffo, data_buf, 25, 0, 16, 2, 13, 1, 1)

                        //-:k14
                        data_buf.insert(0, ::aie::load_v<16>(lptro2));
                        data_buf.insert(1, ::aie::load_v<16>((int16_t*)(lptro2 + 32)));
                        // c_s = 25, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 14, DataStepX = 1, DataStepY = 1
                        MACRO_SLIDING_MAC(acco, coeffo, data_buf, 25, 0, 16, 2, 14, 1, 1)
                    }
                //@}

                //@Convolution Row4 {
                if
                    constexpr(beven) {
                        ::aie::vector<int16_t, 32> data_buf;
                        data_buf.insert(0, ::aie::load_v<16>(lptre3));
                        data_buf.insert(1, ::aie::load_v<16>(lptro3));

                        // k17:k18
                        // c_s = 17, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 18, DataStepX = 16, DataStepY = 1
                        MACRO_SLIDING_MAC(acce, coeffe, data_buf, 17, 0, 16, 2, 18, 16, 1)

                        //-:k16
                        // c_s = 25, d_s = 14, Lanes = 16, Points = 2, CoeffStep = 16, DataStepX = 1, DataStepY = 1
                        data_buf[15] = data_buf[0];
                        MACRO_SLIDING_MAC(acce, coeffe, data_buf, 25, 14, 16, 2, 16, 1, 1)
                    }
                //@}

                //@Convolution Row4 {
                if
                    constexpr(bodd) {
                        ::aie::vector<int16_t, 32> data_buf;
                        data_buf.insert(0, ::aie::load_v<16>(lptre3));
                        data_buf.insert(1, ::aie::load_v<16>(lptro3));

                        // k16:k17
                        // c_s = 16, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 17, DataStepX = 16, DataStepY = 1
                        MACRO_SLIDING_MAC(acco, coeffo, data_buf, 16, 0, 16, 2, 17, 16, 1)

                        //-:k18
                        data_buf.insert(1, ::aie::load_v<16>((int16_t*)(lptro3 + 16)));
                        // c_s = 25, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 18, DataStepX = 1, DataStepY = 1
                        MACRO_SLIDING_MAC(acco, coeffo, data_buf, 25, 0, 16, 2, 18, 1, 1)
                    }
                //@}

                //@Convolution Row1, Row5 {
                if
                    constexpr(beven) {
                        ::aie::vector<int16_t, 32> data_buf;
                        data_buf.insert(0, ::aie::load_v<16>(lptre0));
                        data_buf.insert(1, ::aie::load_v<16>(lptre4));

                        // k2:k22
                        // c_s = 2, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 22, DataStepX = 16, DataStepY = 1
                        MACRO_SLIDING_MAC(acce, coeffe, data_buf, 2, 0, 16, 2, 22, 16, 1)
                    }
                //@}

                //@Convolution Row1, Row5 {
                if
                    constexpr(bodd) {
                        ::aie::vector<int16_t, 32> data_buf;
                        data_buf.insert(0, ::aie::load_v<16>(lptro0));
                        data_buf.insert(1, ::aie::load_v<16>(lptro4));

                        // k2:k22
                        // c_s = 2, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 22, DataStepX = 16, DataStepY = 1
                        MACRO_SLIDING_MAC(acco, coeffo, data_buf, 2, 0, 16, 2, 22, 16, 1)
                    }
                //@}

                ::aie::vector<int16_t, 16> data_bufe;
                ::aie::vector<int16_t, 16> data_bufo;
                if
                    constexpr(beven) data_bufe = acce.template to_vector<int16_t>(SRS_SHIFT_DEM);
                else
                    data_bufe = ::aie::load_v<16>(lptre2);

                if
                    constexpr(bodd) data_bufo = acco.template to_vector<int16_t>(SRS_SHIFT_DEM);
                else
                    data_bufo = ::aie::load_v<16>(lptro2);

                std::tie(data_bufe, data_bufo) = ::aie::interleave_zip(data_bufe, data_bufo, 1);
                ::aie::store_v(data_outp, ::aie::concat(data_bufe, data_bufo));

                lptre0 += 32;
                lptre1 += 32;
                lptre2 += 32;
                lptre3 += 32;
                lptre4 += 32;
                lptro0 += 32;
                lptro1 += 32;
                lptro2 += 32;
                lptro3 += 32;
                lptro4 += 32;
                data_outp += 32;
            }
            //@}

            // Right region
            //@{
            {
                ::aie::accum<acc32, 16> acce;
                ::aie::accum<acc32, 16> acco;
                //@Convolution Row2 {
                {
                    ::aie::vector<int16_t, 32> data_buf;
                    data_buf.insert(0, ::aie::load_v<16>(lptre1));
                    data_buf.insert(1, ::aie::load_v<16>(lptro1));
                    ::aie::vector<int16_t, 32> data_buf1;
                    data_buf1 = data_buf;

                    if
                        constexpr(beven) {
                            // k7:k8
                            // c_s = 7, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 8, DataStepX = 16, DataStepY =
                            // 1
                            MACRO_SLIDING_MUL(acce, coeffe, data_buf, 7, 0, 16, 2, 8, 16, 1)
                        }

                    if
                        constexpr(bodd) {
                            // k6:k7
                            // c_s = 6, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 7, DataStepX = 16, DataStepY =
                            // 1
                            MACRO_SLIDING_MUL(acco, coeffo, data_buf, 6, 0, 16, 2, 7, 16, 1)
                        }

                    if
                        constexpr(beven) {
                            //-:k6
                            data_buf1.insert(0, ::aie::load_v<16>((int16_t*)(lptre1 - 16)));
                            // c_s = 25, d_s = 14, Lanes = 16, Points = 2, CoeffStep = 6, DataStepX = 1, DataStepY =
                            // 1
                            MACRO_SLIDING_MAC(acce, coeffe, data_buf1, 25, 14, 16, 2, 6, 1, 1)
                        }

                    if
                        constexpr(bodd) {
                            //-:k8
                            data_buf[16] = data_buf[31];
                            // c_s = 25, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 8, DataStepX = 1, DataStepY =
                            // 1
                            MACRO_SLIDING_MAC(acco, coeffo, data_buf, 25, 0, 16, 2, 8, 1, 1)
                        }
                }
                //@}

                //@Convolution Row3 {
                if
                    constexpr(beven) {
                        ::aie::vector<int16_t, 32> data_buf;
                        data_buf.insert(0, ::aie::load_v<16>(lptre2));
                        data_buf.insert(1, ::aie::load_v<16>(lptro2));
                        int16_t a = data_buf[31];

                        // k12:k13
                        // c_s = 12, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 13, DataStepX = 16, DataStepY = 1
                        MACRO_SLIDING_MAC(acce, coeffe, data_buf, 12, 0, 16, 2, 13, 16, 1)

                        // k10:k11
                        data_buf = data_buf.push(*(lptre2 - 17));
                        data_buf[16] = *(lptre2 - 1);
                        // c_s = 10, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 11, DataStepX = 16, DataStepY = 1
                        MACRO_SLIDING_MAC(acce, coeffe, data_buf, 10, 0, 16, 2, 11, 16, 1)

                        //-:k14
                        data_buf.insert(0, ::aie::load_v<16>(lptre2));
                        data_buf[16] = a;
                        // c_s = 25, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 14, DataStepX = 1, DataStepY = 1
                        MACRO_SLIDING_MAC(acce, coeffe, data_buf, 25, 0, 16, 2, 14, 1, 1)
                    }
                //@}

                //@Convolution Row3 {
                if
                    constexpr(bodd) {
                        ::aie::vector<int16_t, 32> data_buf;
                        data_buf.insert(0, ::aie::load_v<16>(lptre2));
                        data_buf.insert(1, ::aie::load_v<16>(lptro2));
                        int16_t a = data_buf[31];

                        // k11:k12
                        // c_s = 11, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 12, DataStepX = 16, DataStepY = 1
                        MACRO_SLIDING_MAC(acco, coeffo, data_buf, 11, 0, 16, 2, 12, 16, 1)

                        //-:k10
                        data_buf.insert(0, ::aie::load_v<16>((int16_t*)(lptre2 - 16)));
                        // c_s = 25, d_s = 14, Lanes = 16, Points = 2, CoeffStep = 10, DataStepX = 1, DataStepY = 1
                        MACRO_SLIDING_MAC(acco, coeffo, data_buf, 25, 14, 16, 2, 10, 1, 1)

                        //-:k13
                        data_buf.insert(0, ::aie::load_v<16>(lptre2));
                        data_buf[16] = a;
                        // c_s = 25, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 13, DataStepX = 1, DataStepY = 1
                        MACRO_SLIDING_MAC(acco, coeffo, data_buf, 25, 0, 16, 2, 13, 1, 1)

                        //-:k14
                        data_buf.insert(0, ::aie::load_v<16>(lptro2));
                        data_buf[16] = a;
                        // c_s = 25, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 14, DataStepX = 1, DataStepY = 1
                        MACRO_SLIDING_MAC(acco, coeffo, data_buf, 25, 0, 16, 2, 14, 1, 1)
                    }
                //@}

                //@Convolution Row4 {
                if
                    constexpr(beven) {
                        ::aie::vector<int16_t, 32> data_buf;
                        data_buf.insert(0, ::aie::load_v<16>(lptre3));
                        data_buf.insert(1, ::aie::load_v<16>(lptro3));

                        // k17:k18
                        // c_s = 17, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 18, DataStepX = 16, DataStepY = 1
                        MACRO_SLIDING_MAC(acce, coeffe, data_buf, 17, 0, 16, 2, 18, 16, 1)

                        //-:k16
                        data_buf.insert(0, ::aie::load_v<16>((int16_t*)(lptre3 - 16)));
                        // c_s = 25, d_s = 14, Lanes = 16, Points = 2, CoeffStep = 16, DataStepX = 1, DataStepY = 1
                        MACRO_SLIDING_MAC(acce, coeffe, data_buf, 25, 14, 16, 2, 16, 1, 1)
                    }
                //@}

                //@Convolution Row4 {
                if
                    constexpr(bodd) {
                        ::aie::vector<int16_t, 32> data_buf;
                        data_buf.insert(0, ::aie::load_v<16>(lptre3));
                        data_buf.insert(1, ::aie::load_v<16>(lptro3));

                        // k16:k17
                        // c_s = 16, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 17, DataStepX = 16, DataStepY = 1
                        MACRO_SLIDING_MAC(acco, coeffo, data_buf, 16, 0, 16, 2, 17, 16, 1)

                        //-:k18
                        data_buf[16] = data_buf[31];
                        // c_s = 25, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 18, DataStepX = 1, DataStepY = 1
                        MACRO_SLIDING_MAC(acco, coeffo, data_buf, 25, 0, 16, 2, 18, 1, 1)
                    }
                //@}

                //@Convolution Row1, Row5 {
                if
                    constexpr(beven) {
                        ::aie::vector<int16_t, 32> data_buf;
                        data_buf.insert(0, ::aie::load_v<16>(lptre0));
                        data_buf.insert(1, ::aie::load_v<16>(lptre4));

                        // k2:k22
                        // c_s = 2, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 22, DataStepX = 16, DataStepY = 1
                        MACRO_SLIDING_MAC(acce, coeffe, data_buf, 2, 0, 16, 2, 22, 16, 1)
                    }
                //@}

                //@Convolution Row1, Row5 {
                if
                    constexpr(bodd) {
                        ::aie::vector<int16_t, 32> data_buf;
                        data_buf.insert(0, ::aie::load_v<16>(lptro0));
                        data_buf.insert(1, ::aie::load_v<16>(lptro4));

                        // k2:k22
                        // c_s = 2, d_s = 0, Lanes = 16, Points = 2, CoeffStep = 22, DataStepX = 16, DataStepY = 1
                        MACRO_SLIDING_MAC(acco, coeffo, data_buf, 2, 0, 16, 2, 22, 16, 1)
                    }
                //@}

                ::aie::vector<int16_t, 16> data_bufe;
                ::aie::vector<int16_t, 16> data_bufo;
                if
                    constexpr(beven) data_bufe = acce.template to_vector<int16_t>(SRS_SHIFT_DEM);
                else
                    data_bufe = ::aie::load_v<16>(lptre2);

                if
                    constexpr(bodd) data_bufo = acco.template to_vector<int16_t>(SRS_SHIFT_DEM);
                else
                    data_bufo = ::aie::load_v<16>(lptro2);

                std::tie(data_bufe, data_bufo) = ::aie::interleave_zip(data_bufe, data_bufo, 1);
                ::aie::store_v(data_outp, ::aie::concat(data_bufe, data_bufo));

                lptre0 += 32;
                lptre1 += 32;
                lptre2 += 32;
                lptre3 += 32;
                lptre4 += 32;
                lptro0 += 32;
                lptro1 += 32;
                lptro2 += 32;
                lptro3 += 32;
                lptro4 += 32;
                data_outp += 32;
            }

            lptr_out += (stride_out << 1);
            //@}
        }
}

template <int INPUT_TILE_ELEMENTS>
template <BayerPattern _b>
void DemosaicBaseImpl<INPUT_TILE_ELEMENTS>::xf_demosaic_r(int16_t* img_in_e,
                                                          int16_t* img_in_o,
                                                          int16_t* img_out_e,
                                                          int16_t* img_out_o,
                                                          int16_t image_width,
                                                          int16_t image_height,
                                                          int16_t stride_out) {
    // Computing Red at Blue will also compute Red at Green_Blue internally
    // First line
    xf_demosaic_row<compute_at<_b, R, 0>(), compute_at<_b, R, 1>()>(
        img_in_e, img_in_o, img_out_e, image_width, image_height, stride_out, getCoefficient<_b, R, 0, SRS_SHIFT>(),
        getCoefficient<_b, R, 1, SRS_SHIFT>());
    // Next line
    xf_demosaic_row<compute_at<_b, R, 2>(), compute_at<_b, R, 3>()>(
        img_in_o, (img_in_e + image_width), img_out_o, image_width, image_height, stride_out,
        getCoefficient<_b, R, 2, SRS_SHIFT>(), getCoefficient<_b, R, 3, SRS_SHIFT>());
}

template <int INPUT_TILE_ELEMENTS>
template <BayerPattern _b>
void DemosaicBaseImpl<INPUT_TILE_ELEMENTS>::xf_demosaic_g(int16_t* img_in_e,
                                                          int16_t* img_in_o,
                                                          int16_t* img_out_e,
                                                          int16_t* img_out_o,
                                                          int16_t image_width,
                                                          int16_t image_height,
                                                          int16_t stride_out) {
    // Computing Red at Blue will also compute Red at Green_Blue internally
    // First line
    xf_demosaic_row<compute_at<_b, G, 0>(), compute_at<_b, G, 1>()>(
        img_in_e, img_in_o, img_out_e, image_width, image_height, stride_out, getCoefficient<_b, G, 0, SRS_SHIFT>(),
        getCoefficient<_b, G, 1, SRS_SHIFT>());
    // Next line
    xf_demosaic_row<compute_at<_b, G, 2>(), compute_at<_b, G, 3>()>(
        img_in_o, (img_in_e + image_width), img_out_o, image_width, image_height, stride_out,
        getCoefficient<_b, G, 2, SRS_SHIFT>(), getCoefficient<_b, G, 3, SRS_SHIFT>());
}

template <int INPUT_TILE_ELEMENTS>
template <BayerPattern _b>
void DemosaicBaseImpl<INPUT_TILE_ELEMENTS>::xf_demosaic_b(int16_t* img_in_e,
                                                          int16_t* img_in_o,
                                                          int16_t* img_out_e,
                                                          int16_t* img_out_o,
                                                          int16_t image_width,
                                                          int16_t image_height,
                                                          int16_t stride_out) {
    // Computing Red at Blue will also compute Red at Green_Blue internally
    // First line
    xf_demosaic_row<compute_at<_b, B, 0>(), compute_at<_b, B, 1>()>(
        img_in_e, img_in_o, img_out_e, image_width, image_height, stride_out, getCoefficient<_b, B, 0, SRS_SHIFT>(),
        getCoefficient<_b, B, 1, SRS_SHIFT>());
    // Next line
    xf_demosaic_row<compute_at<_b, B, 2>(), compute_at<_b, B, 3>()>(
        img_in_o, (img_in_e + image_width), img_out_o, image_width, image_height, stride_out,
        getCoefficient<_b, B, 2, SRS_SHIFT>(), getCoefficient<_b, B, 3, SRS_SHIFT>());
}

template <int INPUT_TILE_ELEMENTS>
template <BayerPattern _b>
void DemosaicBaseImpl<INPUT_TILE_ELEMENTS>::xf_demosaic(int16_t* in_e,
                                                        int16_t* in_o,
                                                        int16_t* out_r,
                                                        int16_t* out_g,
                                                        int16_t* out_b,
                                                        int16_t image_width,
                                                        int16_t image_height,
                                                        int16_t stride_out) {
    xf_demosaic_r<_b>(in_e, in_o, out_r, (out_r + stride_out), image_width, image_height, stride_out);
    xf_demosaic_g<_b>(in_e, in_o, out_g, (out_g + stride_out), image_width, image_height, stride_out);
    xf_demosaic_b<_b>(in_e, in_o, out_b, (out_b + stride_out), image_width, image_height, stride_out);
}

template <int INPUT_TILE_ELEMENTS>
template <BayerPattern _b>
void DemosaicBaseImpl<INPUT_TILE_ELEMENTS>::xf_demosaic_wrap(int16_t* in_e_ptr,
                                                             int16_t* in_o_ptr,
                                                             int16_t* out_ptr_r,
                                                             int16_t* out_ptr_g,
                                                             int16_t* out_ptr_b,
                                                             const int16_t& posH,
                                                             const int16_t& posV,
                                                             const int16_t& image_width,
                                                             const int16_t& image_height,
                                                             const int16_t& stride_out) {
    if (posV % 2 == 0) {
        if (posH % 2 == 0) {
            xf_demosaic<_b>(in_e_ptr, in_o_ptr, out_ptr_r, out_ptr_g, out_ptr_b, image_width, image_height, stride_out);
        } else {
            xf_demosaic<getRelativeBayerPattern<_b, 1>()>(in_e_ptr, in_o_ptr, out_ptr_r, out_ptr_g, out_ptr_b,
                                                          image_width, image_height, stride_out);
        }
    } else {
        if (posH % 2 == 0) {
            xf_demosaic<getRelativeBayerPattern<_b, 2>()>(in_e_ptr, in_o_ptr, out_ptr_r, out_ptr_g, out_ptr_b,
                                                          image_width, image_height, stride_out);
        } else {
            xf_demosaic<getRelativeBayerPattern<_b, 3>()>(in_e_ptr, in_o_ptr, out_ptr_r, out_ptr_g, out_ptr_b,
                                                          image_width, image_height, stride_out);
        }
    }
}

template <BayerPattern _b, int INPUT_TILE_ELEMENTS>
__attribute__((noinline)) void DemosaicPlanar<_b, INPUT_TILE_ELEMENTS>::runImpl(input_window_int16* img_in,
                                                                                output_window_int16* img_out_r,
                                                                                output_window_int16* img_out_g,
                                                                                output_window_int16* img_out_b) {
    int16_t* img_in_ptr = (int16_t*)img_in->ptr;
    int16_t* img_out_r_ptr = (int16_t*)img_out_r->ptr;
    int16_t* img_out_g_ptr = (int16_t*)img_out_g->ptr;
    int16_t* img_out_b_ptr = (int16_t*)img_out_b->ptr;

    const int16_t overlapT = xfGetTileOVLP_VT(img_in_ptr);
    const int16_t overlapB = xfGetTileOVLP_VB(img_in_ptr);

    int16_t image_width = xfGetTileWidth(img_in_ptr);
    int16_t image_height = xfGetTileHeight(img_in_ptr);

    RUNTIME_ASSERT(((overlapT % 2) == 0), "Top overlap is always expected to be multiple of 2 (i.e. 0/2/4)");
    RUNTIME_ASSERT(((overlapB % 2) == 0), "Bottom overlap is always expected to be multiple of 2 (i.e. 0/2/4)");
    RUNTIME_ASSERT((image_width == 64), "Incorrect tile width, expected tile width is 64");

    xfCopyMetaData(img_in_ptr, img_out_r_ptr);
    xfCopyMetaData(img_in_ptr, img_out_g_ptr);
    xfCopyMetaData(img_in_ptr, img_out_b_ptr);
    xfUnsignedSaturation(img_out_r_ptr);
    xfUnsignedSaturation(img_out_g_ptr);
    xfUnsignedSaturation(img_out_b_ptr);

    int16_t* in_ptr = (int16_t*)xfGetImgDataPtr(img_in_ptr);
    int16_t* out_ptr_r = (int16_t*)xfGetImgDataPtr(img_out_r_ptr);
    int16_t* out_ptr_g = (int16_t*)xfGetImgDataPtr(img_out_g_ptr);
    int16_t* out_ptr_b = (int16_t*)xfGetImgDataPtr(img_out_b_ptr);

    const int16_t posH = xfGetTilePosH(img_in_ptr);
    const int16_t posV = xfGetTilePosV(img_in_ptr);

    xf_demosaic_interleave_input(in_ptr, mInEven, mInOdd, image_width, image_height);

    image_height = image_height - (overlapT + overlapB);
    int16_t* in_e = (int16_t*)(mInEven + (overlapT >> 1) * image_width);
    int16_t* in_o = (int16_t*)(mInOdd + (overlapT >> 1) * image_width);
    out_ptr_r = (int16_t*)(out_ptr_r + overlapT * image_width);
    out_ptr_g = (int16_t*)(out_ptr_g + overlapT * image_width);
    out_ptr_b = (int16_t*)(out_ptr_b + overlapT * image_width);

    this->template xf_demosaic_wrap<_b>(in_e, in_o, out_ptr_r, out_ptr_g, out_ptr_b, posH, posV, image_width,
                                        image_height, image_width);
}

template <BayerPattern _b, int INPUT_TILE_ELEMENTS>
__attribute__((noinline)) void DemosaicRGBA<_b, INPUT_TILE_ELEMENTS>::xf_demosaic_rgba_pixel_packing(
    int16_t* red,
    int16_t* green,
    int16_t* blue,
    int16_t* out,
    int16_t image_width,
    int16_t image_height,
    int16_t stride_in) {
    auto zerovec = ::aie::zeros<int16_t, 16>();
    int16_t* lptr_red = red;
    int16_t* lptr_green = green;
    int16_t* lptr_blue = blue;
    for (int i = 0; i < image_height; i++) chess_prepare_for_pipelining chess_loop_range(1, ) {
            int16_t* r_data_p = lptr_red;
            int16_t* g_data_p = lptr_green;
            int16_t* b_data_p = lptr_blue;
            for (int j = 0; j < image_width; j += 16) chess_prepare_for_pipelining {
                    auto r = ::aie::load_v<16>(r_data_p);
                    auto g = ::aie::load_v<16>(g_data_p);
                    auto b = ::aie::load_v<16>(b_data_p);
                    ::aie::vector<int16_t, 16> a;
                    std::tie(r, g) = ::aie::interleave_zip(r, g, 1);
                    std::tie(b, a) = ::aie::interleave_zip(b, zerovec, 1);
                    auto[x, y] = ::aie::interleave_zip(::aie::concat(r, g), ::aie::concat(b, a), 2);
                    ::aie::store_v(out, x);
                    ::aie::store_v((int16_t*)(out + 32), y);
                    r_data_p += 16;
                    g_data_p += 16;
                    b_data_p += 16;
                    out += 64;
                }
            lptr_red += stride_in;
            lptr_green += stride_in;
            lptr_blue += stride_in;
        }
}

template <BayerPattern _b, int INPUT_TILE_ELEMENTS>
inline void DemosaicRGBA<_b, INPUT_TILE_ELEMENTS>::xfSetRGBAMetaData(void* img_ptr) {
    xfSetTileWidth(img_ptr, xfGetTileWidth(img_ptr) * 4);
    xfSetTilePosH(img_ptr, xfGetTilePosH(img_ptr) * 4);
    xfSetTileOutPosH(img_ptr, xfGetTileOutPosH(img_ptr) * 4);
    xfSetTileOutTWidth(img_ptr, xfGetTileOutTWidth(img_ptr) * 4);
    xfSetTileOVLP_HL(img_ptr, xfGetTileOVLP_HL(img_ptr) * 4);
    xfSetTileOVLP_HR(img_ptr, xfGetTileOVLP_HR(img_ptr) * 4);
    uint16_t outOffset_U = xfGetTileOutOffset_U(img_ptr);
    uint16_t outOffset_L = xfGetTileOutOffset_L(img_ptr);
    int outOffset = (outOffset_U << 16) + outOffset_L;
    outOffset = 4 * outOffset;
    xfSetTileOutOffset_L(img_ptr, (outOffset & 0x0000ffff));
    xfSetTileOutOffset_U(img_ptr, (outOffset >> 16));
}

template <BayerPattern _b, int INPUT_TILE_ELEMENTS>
__attribute__((noinline)) void DemosaicRGBA<_b, INPUT_TILE_ELEMENTS>::runImpl(input_window_int16* img_in,
                                                                              output_window_int16* img_out) {
    int16_t* img_in_ptr = (int16_t*)img_in->ptr;
    int16_t* img_out_ptr = (int16_t*)img_out->ptr;

    const int16_t overlapT = xfGetTileOVLP_VT(img_in_ptr);
    const int16_t overlapB = xfGetTileOVLP_VB(img_in_ptr);

    int16_t image_width = xfGetTileWidth(img_in_ptr);
    int16_t image_height = xfGetTileHeight(img_in_ptr);
    const int16_t stride_out = (image_width << 2);

    RUNTIME_ASSERT(((overlapT % 2) == 0), "Top overlap is always expected to be multiple of 2 (i.e. 0/2/4)");
    RUNTIME_ASSERT(((overlapB % 2) == 0), "Bottom overlap is always expected to be multiple of 2 (i.e. 0/2/4)");
    RUNTIME_ASSERT((image_width == 64), "Incorrect tile width, expected tile width is 64");

    xfCopyMetaData(img_in_ptr, img_out_ptr);
    xfUnsignedSaturation(img_out_ptr);
    xfSetRGBAMetaData((void*)img_out_ptr);

    int16_t* in_ptr = (int16_t*)xfGetImgDataPtr(img_in_ptr);
    int16_t* out_ptr = (int16_t*)xfGetImgDataPtr(img_out_ptr);

    const int16_t posH = xfGetTilePosH(img_in_ptr);
    const int16_t posV = xfGetTilePosV(img_in_ptr);

    xf_demosaic_interleave_input(in_ptr, mInEven, mInOdd, image_width, image_height);

    image_height = image_height - (overlapT + overlapB);
    int16_t* in_e = (int16_t*)(mInEven + (overlapT >> 1) * image_width);
    int16_t* in_o = (int16_t*)(mInOdd + (overlapT >> 1) * image_width);
    out_ptr = (int16_t*)(out_ptr + overlapT * stride_out);

    this->template xf_demosaic_wrap<_b>(in_e, in_o, mRChannel, mGChannel, mBChannel, posH, posV, image_width,
                                        image_height, image_width);
    xf_demosaic_rgba_pixel_packing(mRChannel, mGChannel, mBChannel, out_ptr, image_width, image_height, image_width);
}

} // aie
} // cv
} // xf

#endif
