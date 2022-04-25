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

#ifndef __XF_DEMOSAICING_HPP__
#define __XF_DEMOSAICING_HPP__

#include <adf.h>

namespace xf {
namespace cv {
namespace aie {

enum Channel {
    Default = 0,
    R = 1,
    G = 2,
    B = 3,
};

enum BayerPattern {
    RGGB = ((R << 0) + (G << 2) + (G << 4) + (B << 6)),
    GRBG = ((G << 0) + (R << 2) + (B << 4) + (G << 6)),
    GBRG = ((G << 0) + (B << 2) + (R << 4) + (G << 6)),
    BGGR = ((B << 0) + (G << 2) + (G << 4) + (R << 6))
};

enum class DemosaicType {
    Red_At_Green_Red = ((R << 4) + (G << 2) + (R << 0)),
    Red_At_Green_Blue = ((B << 4) + (G << 2) + (R << 0)),
    Red_At_Blue = ((B << 2) + (R << 0)),
    Blue_At_Green_Red = ((R << 4) + (G << 2) + (B << 0)),
    Blue_At_Green_Blue = ((B << 4) + (G << 2) + (B << 0)),
    Blue_At_Red = ((R << 2) + (B << 0)),
    Green_At_Red = ((R << 2) + (G << 0)),
    Green_At_Blue = ((B << 2) + (G << 0))
};

template <int INPUT_TILE_ELEMENTS>
class DemosaicBaseImpl {
   public:
    static constexpr int SRS_SHIFT = 3;
    static constexpr int SRS_SHIFT_DEM = (3 + SRS_SHIFT);
    static constexpr int INPUT_TILE_WIDTH = 64; // Assumes input tile is always 64 elements wide
    static constexpr int INPUT_TILE_HEIGHT = (INPUT_TILE_ELEMENTS / 64);
    static constexpr int INTERLEAVE_TILE_HEIGHT = (INPUT_TILE_HEIGHT / 2) + 2;
    static constexpr int INTERLEAVE_TILE_ELEMENTS = (INPUT_TILE_WIDTH * INTERLEAVE_TILE_HEIGHT);

    void xf_demosaic_interleave_input(int16_t* in_ptr,
                                      int16_t* out_ptr_e,
                                      int16_t* out_ptr_o,
                                      const int16_t& image_width,
                                      const int16_t& image_height);

    template <bool beven, bool bodd>
    void xf_demosaic_row(int16_t* img_in_e,
                         int16_t* img_in_o,
                         int16_t* img_out,
                         int16_t image_width,
                         int16_t image_height,
                         int16_t stride_out,
                         const int16_t* coeffpe,
                         const int16_t* coeffpo);

    template <BayerPattern _b>
    void xf_demosaic_r(int16_t* img_in_e,
                       int16_t* img_in_o,
                       int16_t* img_out_e,
                       int16_t* img_out_o,
                       int16_t image_width,
                       int16_t image_height,
                       int16_t stride_out);

    template <BayerPattern _b>
    void xf_demosaic_g(int16_t* img_in_e,
                       int16_t* img_in_o,
                       int16_t* img_out_e,
                       int16_t* img_out_o,
                       int16_t image_width,
                       int16_t image_height,
                       int16_t stride_out);

    template <BayerPattern _b>
    void xf_demosaic_b(int16_t* img_in_e,
                       int16_t* img_in_o,
                       int16_t* img_out_e,
                       int16_t* img_out_o,
                       int16_t image_width,
                       int16_t image_height,
                       int16_t stride_out);

    template <BayerPattern _b>
    void xf_demosaic(int16_t* in_e,
                     int16_t* in_o,
                     int16_t* out_r,
                     int16_t* out_g,
                     int16_t* out_b,
                     int16_t image_width,
                     int16_t image_height,
                     int16_t stride_out);

    template <BayerPattern _b>
    void xf_demosaic_wrap(int16_t* in_e_ptr,
                          int16_t* in_o_ptr,
                          int16_t* out_ptr_r,
                          int16_t* out_ptr_g,
                          int16_t* out_ptr_b,
                          const int16_t& posH,
                          const int16_t& posV,
                          const int16_t& image_width,
                          const int16_t& image_height,
                          const int16_t& stride_out);
};

template <BayerPattern b, int INPUT_TILE_ELEMENTS>
class DemosaicPlanar : public DemosaicBaseImpl<INPUT_TILE_ELEMENTS> {
   public:
    using DemosaicBaseImpl<INPUT_TILE_ELEMENTS>::INPUT_TILE_WIDTH;
    using DemosaicBaseImpl<INPUT_TILE_ELEMENTS>::INPUT_TILE_HEIGHT;
    using DemosaicBaseImpl<INPUT_TILE_ELEMENTS>::INTERLEAVE_TILE_ELEMENTS;
    using DemosaicBaseImpl<INPUT_TILE_ELEMENTS>::xf_demosaic_interleave_input;
    using DemosaicBaseImpl<INPUT_TILE_ELEMENTS>::xf_demosaic_wrap;

    int16_t (&mInEven)[INTERLEAVE_TILE_ELEMENTS];
    int16_t (&mInOdd)[INTERLEAVE_TILE_ELEMENTS];

    DemosaicPlanar(int16_t (&iEven)[INTERLEAVE_TILE_ELEMENTS], int16_t (&iOdd)[INTERLEAVE_TILE_ELEMENTS])
        : mInEven(iEven), mInOdd(iOdd) {}
    void runImpl(input_window_int16* img_in,
                 output_window_int16* img_out_r,
                 output_window_int16* img_out_g,
                 output_window_int16* img_out_b);
};

template <BayerPattern b, int INPUT_TILE_ELEMENTS>
class DemosaicRGBA : public DemosaicBaseImpl<INPUT_TILE_ELEMENTS> {
   public:
    using DemosaicBaseImpl<INPUT_TILE_ELEMENTS>::INPUT_TILE_WIDTH;
    using DemosaicBaseImpl<INPUT_TILE_ELEMENTS>::INPUT_TILE_HEIGHT;
    using DemosaicBaseImpl<INPUT_TILE_ELEMENTS>::INTERLEAVE_TILE_ELEMENTS;
    using DemosaicBaseImpl<INPUT_TILE_ELEMENTS>::xf_demosaic_interleave_input;
    using DemosaicBaseImpl<INPUT_TILE_ELEMENTS>::xf_demosaic_wrap;

    int16_t (&mInEven)[INTERLEAVE_TILE_ELEMENTS];
    int16_t (&mInOdd)[INTERLEAVE_TILE_ELEMENTS];
    int16_t (&mRChannel)[INPUT_TILE_ELEMENTS];
    int16_t (&mGChannel)[INPUT_TILE_ELEMENTS];
    int16_t (&mBChannel)[INPUT_TILE_ELEMENTS];

    void xf_demosaic_rgba_pixel_packing(int16_t* red,
                                        int16_t* green,
                                        int16_t* blue,
                                        int16_t* out,
                                        int16_t image_width,
                                        int16_t image_height,
                                        int16_t stride_in);
    void xfSetRGBAMetaData(void* img_ptr);

   public:
    DemosaicRGBA(int16_t (&iEven)[INTERLEAVE_TILE_ELEMENTS],
                 int16_t (&iOdd)[INTERLEAVE_TILE_ELEMENTS],
                 int16_t (&rch)[INPUT_TILE_ELEMENTS],
                 int16_t (&gch)[INPUT_TILE_ELEMENTS],
                 int16_t (&bch)[INPUT_TILE_ELEMENTS])
        : DemosaicBaseImpl<INPUT_TILE_ELEMENTS>(),
          mInEven(iEven),
          mInOdd(iOdd),
          mRChannel(rch),
          mGChannel(gch),
          mBChannel(bch) {}

    void runImpl(input_window_int16* img_in, output_window_int16* img_out);
};

} // aie
} // cv
} // xf

#endif
