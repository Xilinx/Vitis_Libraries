#ifndef __XF_RGBA2YUV_IMPL_HPP__
#define __XF_RGBA2YUV_IMPL_HPP__

#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/utils.hpp>
#include <common/xf_aie_hw_utils.hpp>

namespace xf {
namespace cv {
namespace aie {

class Rgba2YuvBaseImpl {
   public:
    void runImpl(uint8_t* img_in_rgba,
                 uint8_t* img_out_y,
                 uint8_t* img_out_uv,
                 const int16_t& tile_width,
                 const int16_t& tile_height);
    void xf_rgba2yuv(uint8_t* restrict ptr_rgba,
                     uint8_t* restrict ptr_out_y,
                     uint8_t* restrict ptr_out_uv,
                     const int16_t& tile_width,
                     const int16_t& tile_height);

    void xf_extract_rgb(uint8_t* ptr_rgba,
                        ::aie::vector<uint8_t, 32>& r,
                        ::aie::vector<uint8_t, 32>& g,
                        ::aie::vector<uint8_t, 32>& b);
};

__attribute__((inline)) void Rgba2YuvBaseImpl::xf_extract_rgb(uint8_t* ptr_rgba,
                                                              ::aie::vector<uint8_t, 32>& r,
                                                              ::aie::vector<uint8_t, 32>& g,
                                                              ::aie::vector<uint8_t, 32>& b) {
    ::aie::vector<uint8_t, 32> rgba_channel0, rgba_channel1, rgba_channel3, rgba_channel2;
    rgba_channel0 = ::aie::load_v<32>(ptr_rgba);
    ptr_rgba += 32;
    rgba_channel1 = ::aie::load_v<32>(ptr_rgba);
    ptr_rgba += 32;
    rgba_channel2 = ::aie::load_v<32>(ptr_rgba);
    ptr_rgba += 32;
    rgba_channel3 = ::aie::load_v<32>(ptr_rgba);
    ptr_rgba += 32;

    // Unzip the interleaved channels
    auto[rg_temp, ba_temp] = ::aie::interleave_unzip(::aie::concat(rgba_channel0, rgba_channel1),
                                                     ::aie::concat(rgba_channel2, rgba_channel3), 2);
    r = ::aie::filter_even(rg_temp, 1);
    g = ::aie::filter_odd(rg_temp, 1);
    b = ::aie::filter_even(ba_temp, 1);
}

__attribute__((noinline)) void Rgba2YuvBaseImpl::xf_rgba2yuv(uint8_t* restrict ptr_rgba,
                                                             uint8_t* restrict ptr_out_y,
                                                             uint8_t* restrict ptr_out_uv,
                                                             const int16_t& tile_width,
                                                             const int16_t& tile_height) {
    ::aie::vector<int16_t, 16> WT(66, 129, 25, 32, -19, -37, 56, 128, -9, -47, 56, 128, 0, 0, 0);
    ::aie::vector<uint8_t, 32> c1 = ::aie::broadcast<uint8_t, 32>(128);
    ::aie::vector<uint8_t, 32> r, g, b, y, u, v;
    ::aie::accum<acc32, 32> acc, acc2;
    uint16_t loop_count = (tile_height * tile_width) >> 5;
    uint8_t* restrict ptr_rgba_bckup = ptr_rgba;
    // int16_t odd_row = row_index % 2;

    ptr_rgba = ptr_rgba + (tile_height - 1) * (tile_width * 4);
    ptr_out_y = ptr_out_y + (tile_height - 1) * tile_width;
    ptr_out_uv = ptr_out_uv + (tile_height / 2 - 1) * tile_width;

    // for (int j = 0; j < loop_count; j += 1) chess_prepare_for_pipelining {
    for (int i = tile_height - 1; i > -1; i--) {
        for (int j = 0; j < (tile_width); j += 32) chess_prepare_for_pipelining {
                // READ 32-bit RGBA channels of 32 pixels. Total 1024 bits.
                xf_extract_rgb(ptr_rgba, r, g, b);
                ptr_rgba += 128;
                acc = ::aie::accumulate<32>(WT, 0, r, g, b, c1);
                y = acc.template to_vector<uint8_t>(8);
                ::aie::store_v((uint8_t*)ptr_out_y, y);
                ptr_out_y = ptr_out_y + 32;

                acc = ::aie::accumulate<32>(WT, 4, r, g, b, c1);
                u = acc.template to_vector<uint8_t>(7);
                acc2 = ::aie::accumulate<32>(WT, 8, b, g, r, c1);
                v = acc2.template to_vector<uint8_t>(7);
                auto[uv1_temp, uv2_temp] = ::aie::interleave_zip(u, v, 1);
                auto[uv3_temp, uv4_temp] = ::aie::interleave_unzip(uv1_temp, uv2_temp, 2);
                ::aie::store_v((uint8_t*)ptr_out_uv, uv3_temp);
                ptr_out_uv = ptr_out_uv + 32;
            }
        ptr_out_uv = ptr_out_uv - (2 - (i % 2)) * tile_width;
        ptr_out_y = ptr_out_y - 2 * tile_width;
        ptr_rgba = ptr_rgba - 2 * (tile_width * 4);
    }

    // ptr_rgba = ptr_rgba_bckup;
    /*if (tile_height > 1) {
        if (odd_row == 1) {
            ptr_rgba = ptr_rgba + 4 * tile_width;
        }
    }

    if ((tile_height > 1) or (tile_height == 1 and odd_row == 0)) {
        for (int row = 0; row < tile_height; row += 2) {
            for (int j = 0; j < (tile_width >> 5); j += 1) chess_prepare_for_pipelining {
                    xf_extract_rgb(ptr_rgba, r,g,b); ptr_rgba += 128;
                    acc = ::aie::accumulate<32>(WT, 4, r, g, b, c1);
                    u = acc.template to_vector<uint8_t>(7);
                    acc2 = ::aie::accumulate<32>(WT, 8, b, g, r, c1);
                    v = acc2.template to_vector<uint8_t>(7);
                    auto[uv1_temp, uv2_temp] = ::aie::interleave_zip(u, v, 1);
                    auto[uv3_temp, uv4_temp] = ::aie::interleave_unzip(uv1_temp, uv2_temp, 2);
                    ::aie::store_v((uint8_t*)ptr_out_uv, uv3_temp);
                    ptr_out_uv = ptr_out_uv + 32;
                }
            ptr_rgba = ptr_rgba + tile_width * 4;
        }
    }*/
}

void Rgba2YuvBaseImpl::runImpl(uint8_t* img_in_rgba,
                               uint8_t* img_out_y,
                               uint8_t* img_out_uv,
                               const int16_t& tile_width,
                               const int16_t& tile_height) {
    xfCopyMetaData(img_in_rgba, img_out_y);
    xfCopyMetaData(img_in_rgba, img_out_uv);

    xfSetTileWidth(img_out_uv, xfGetTileWidth(img_out_uv) / 2);
    xfSetTileHeight(img_out_uv, xfGetTileHeight(img_out_uv) / 2);
    xfSetTilePosH(img_out_uv, xfGetTilePosH(img_out_uv) / 2);
    xfSetTilePosV(img_out_uv, xfGetTilePosV(img_out_uv) / 2);
    xfSetTileOutPosH(img_out_uv, xfGetTileOutPosH(img_out_uv) / 2);
    xfSetTileOutPosV(img_out_uv, xfGetTileOutPosV(img_out_uv) / 2);
    xfSetTileOutTWidth(img_out_uv, xfGetTileOutTWidth(img_out_uv) / 2);
    xfSetTileOutTHeight(img_out_uv, xfGetTileOutTHeight(img_out_uv) / 2);
    // OutOffset = ((OutpositionV * OutputImageStride) + OutpositionH);
    uint16_t outOffset_U = xfGetTileOutOffset_U(img_out_uv);
    uint16_t outOffset_L = xfGetTileOutOffset_L(img_out_uv);
    int outOffset = (outOffset_U << 16) + outOffset_L;

    outOffset = xfGetTileOutPosV(img_out_uv) * (xfGetImgStride(img_out_uv) / 2) + xfGetTileOutPosH(img_out_uv);

    xfSetTileOutOffset_L(img_out_uv, (outOffset & 0x0000ffff));
    xfSetTileOutOffset_U(img_out_uv, (outOffset >> 16));

    uint8_t* restrict ptr_in_rgba = (uint8_t*)xfGetImgDataPtr(img_in_rgba);
    uint8_t* restrict ptr_out_y = (uint8_t*)xfGetImgDataPtr(img_out_y);
    uint8_t* restrict ptr_out_uv = (uint8_t*)xfGetImgDataPtr(img_out_uv);

    xf_rgba2yuv(ptr_in_rgba, ptr_out_y, ptr_out_uv, tile_width, tile_height);
}
} // aie
} // cv
} // xf
#endif
