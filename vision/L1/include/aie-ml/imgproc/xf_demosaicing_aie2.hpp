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
    static constexpr int INPUT_TILE_WIDTH = 128; // Assumes input tile is always 64 elements wide
    static constexpr int INPUT_TILE_HEIGHT = (INPUT_TILE_ELEMENTS / 128);
    static constexpr int INTERLEAVE_TILE_HEIGHT = (INPUT_TILE_HEIGHT / 2) + 2;
    static constexpr int INTERLEAVE_TILE_ELEMENTS = (INPUT_TILE_WIDTH * INTERLEAVE_TILE_HEIGHT);

    void xf_demosaic_interleave_input(uint8_t* in_ptr,
                                      uint8_t* out_ptr_e,
                                      uint8_t* out_ptr_o,
                                      const int16_t& image_width,
                                      const int16_t& image_height);

    template <bool beven, bool bodd>
    void xf_demosaic_row(uint8_t* img_in_e,
                         uint8_t* img_in_o,
                         uint8_t* img_out,
                         int16_t image_width,
                         int16_t image_height,
                         int16_t stride_out,
                         const int8_t* coeffpe,
                         const int8_t* coeffpo);

    template <BayerPattern _b>
    void xf_demosaic_r(uint8_t* img_in_e,
                       uint8_t* img_in_o,
                       uint8_t* img_out_e,
                       uint8_t* img_out_o,
                       int16_t image_width,
                       int16_t image_height,
                       int16_t stride_out);

    template <BayerPattern _b>
    void xf_demosaic_g(uint8_t* img_in_e,
                       uint8_t* img_in_o,
                       uint8_t* img_out_e,
                       uint8_t* img_out_o,
                       int16_t image_width,
                       int16_t image_height,
                       int16_t stride_out);

    template <BayerPattern _b>
    void xf_demosaic_b(uint8_t* img_in_e,
                       uint8_t* img_in_o,
                       uint8_t* img_out_e,
                       uint8_t* img_out_o,
                       int16_t image_width,
                       int16_t image_height,
                       int16_t stride_out);

    template <BayerPattern _b>
    void xf_demosaic(uint8_t* in_e,
                     uint8_t* in_o,
                     uint8_t* out_r,
                     uint8_t* out_g,
                     uint8_t* out_b,
                     int16_t image_width,
                     int16_t image_height,
                     int16_t stride_out);

    template <BayerPattern _b>
    void xf_demosaic_wrap(uint8_t* in_e_ptr,
                          uint8_t* in_o_ptr,
                          uint8_t* out_ptr_r,
                          uint8_t* out_ptr_g,
                          uint8_t* out_ptr_b,
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

    uint8_t (&mInEven)[INTERLEAVE_TILE_ELEMENTS];
    uint8_t (&mInOdd)[INTERLEAVE_TILE_ELEMENTS];

    DemosaicPlanar(uint8_t (&iEven)[INTERLEAVE_TILE_ELEMENTS], uint8_t (&iOdd)[INTERLEAVE_TILE_ELEMENTS])
        : mInEven(iEven), mInOdd(iOdd) {}
    void runImpl(adf::input_buffer<uint8>& img_in,
                 adf::output_buffer<uint8>& img_out_r,
                 adf::output_buffer<uint8>& img_out_g,
                 adf::output_buffer<uint8>& img_out_b);
};

template <BayerPattern b, int INPUT_TILE_ELEMENTS>
class DemosaicRGBA : public DemosaicBaseImpl<INPUT_TILE_ELEMENTS> {
   public:
    using DemosaicBaseImpl<INPUT_TILE_ELEMENTS>::INPUT_TILE_WIDTH;
    using DemosaicBaseImpl<INPUT_TILE_ELEMENTS>::INPUT_TILE_HEIGHT;
    using DemosaicBaseImpl<INPUT_TILE_ELEMENTS>::INTERLEAVE_TILE_ELEMENTS;
    using DemosaicBaseImpl<INPUT_TILE_ELEMENTS>::xf_demosaic_interleave_input;
    using DemosaicBaseImpl<INPUT_TILE_ELEMENTS>::xf_demosaic_wrap;

    uint8_t (&mInEven)[INTERLEAVE_TILE_ELEMENTS];
    uint8_t (&mInOdd)[INTERLEAVE_TILE_ELEMENTS];
    uint8_t (&mRChannel)[INPUT_TILE_ELEMENTS];
    uint8_t (&mGChannel)[INPUT_TILE_ELEMENTS];
    uint8_t (&mBChannel)[INPUT_TILE_ELEMENTS];

    void xf_demosaic_rgba_pixel_packing(uint8_t* red,
                                        uint8_t* green,
                                        uint8_t* blue,
                                        uint8_t* out,
                                        int16_t image_width,
                                        int16_t image_height,
                                        int16_t stride_in);
    void xfSetRGBAMetaData(void* img_ptr);

   public:
    DemosaicRGBA(uint8_t (&iEven)[INTERLEAVE_TILE_ELEMENTS],
                 uint8_t (&iOdd)[INTERLEAVE_TILE_ELEMENTS],
                 uint8_t (&rch)[INPUT_TILE_ELEMENTS],
                 uint8_t (&gch)[INPUT_TILE_ELEMENTS],
                 uint8_t (&bch)[INPUT_TILE_ELEMENTS])
        : DemosaicBaseImpl<INPUT_TILE_ELEMENTS>(),
          mInEven(iEven),
          mInOdd(iOdd),
          mRChannel(rch),
          mGChannel(gch),
          mBChannel(bch) {}

    void runImpl(adf::input_buffer<uint8>& in, adf::output_buffer<uint8>& out);
    void runImpl_ptr(uint8_t* in, uint8_t* out);
};

} // aie
} // cv
} // xf

#endif
