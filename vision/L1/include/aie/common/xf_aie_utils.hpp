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

#ifndef _XF_AIE_UTILS_H_
#define _XF_AIE_UTILS_H_

#include <common/xf_aie_const.hpp>

namespace xf {
namespace cv {
namespace aie {

//@Get functions {
inline metadata_elem_t xfGetTileWidth(void* img_ptr) {
    return ((metadata_elem_t*)img_ptr)[POS_MDS_TILEWIDTH];
}

inline metadata_elem_t xfGetTileHeight(void* img_ptr) {
    return ((metadata_elem_t*)img_ptr)[POS_MDS_TILEHEIGHT];
}

inline metadata_elem_t xfGetTilePosH(void* img_ptr) {
    return ((metadata_elem_t*)img_ptr)[POS_MDS_POSITIONH];
}

inline metadata_elem_t xfGetTilePosV(void* img_ptr) {
    return ((metadata_elem_t*)img_ptr)[POS_MDS_POSITIONV];
}

inline metadata_elem_t xfGetTileOVLP_HL(void* img_ptr) {
    return ((metadata_elem_t*)img_ptr)[POS_MDS_OVLPH_LEFT];
}

inline metadata_elem_t xfGetTileOVLP_HR(void* img_ptr) {
    return ((metadata_elem_t*)img_ptr)[POS_MDS_OVLPH_RIGHT];
}

inline metadata_elem_t xfGetTileOVLP_VT(void* img_ptr) {
    return ((metadata_elem_t*)img_ptr)[POS_MDS_OVLPV_TOP];
}

inline metadata_elem_t xfGetTileOVLP_VB(void* img_ptr) {
    return ((metadata_elem_t*)img_ptr)[POS_MDS_OVLPV_BOTTOM];
}

inline metadata_elem_t xfGetTileDatBitwidth(void* img_ptr) {
    return ((metadata_elem_t*)img_ptr)[POS_MDS_DATA_BITWIDTH];
}

inline metadata_elem_t xfGetTileFinalWidth(void* img_ptr) {
    return ((metadata_elem_t*)img_ptr)[POS_MDS_FINAL_WIDTH];
}

inline metadata_elem_t xfGetTileFinalHeight(void* img_ptr) {
    return ((metadata_elem_t*)img_ptr)[POS_MDS_FINAL_HEIGHT];
}

inline metadata_elem_t xfGetTileOutOffset_L(void* img_ptr) {
    return ((metadata_elem_t*)img_ptr)[POS_MDS_OUT_OFFSET_LOWER];
}

inline metadata_elem_t xfGetTileOutOffset_U(void* img_ptr) {
    return ((metadata_elem_t*)img_ptr)[POS_MDS_OUT_OFFSET_UPPER];
}

inline metadata_elem_t xfGetTileOutPosH(void* img_ptr) {
    return ((metadata_elem_t*)img_ptr)[POS_MDS_OUT_POSH];
}

inline metadata_elem_t xfGetTileOutPosV(void* img_ptr) {
    return ((metadata_elem_t*)img_ptr)[POS_MDS_OUT_POSV];
}

inline metadata_elem_t xfGetTileOutTWidth(void* img_ptr) {
    return ((metadata_elem_t*)img_ptr)[POS_MDS_OUT_TWIDTH];
}

inline metadata_elem_t xfGetTileOutTHeight(void* img_ptr) {
    return ((metadata_elem_t*)img_ptr)[POS_MDS_OUT_THEIGHT];
}

inline metadata_elem_t xfGetTileSat(void* img_ptr) {
    return ((metadata_elem_t*)img_ptr)[POS_MDS_SAT_EN];
}

inline metadata_elem_t xfGetImgStride(void* img_ptr) {
    return ((metadata_elem_t*)img_ptr)[POS_MDS_IMAGE_STRIDE];
}

inline void* xfGetImgDataPtr(void* img_ptr) {
    return ((metadata_elem_t*)img_ptr + POS_MDS_IMG_PTR);
}
//@}

//@Set functions {
inline void xfSetTileWidth(void* img_ptr, metadata_elem_t width) {
    ((metadata_elem_t*)img_ptr)[POS_MDS_TILEWIDTH] = width;
}

inline void xfSetTileHeight(void* img_ptr, metadata_elem_t height) {
    ((metadata_elem_t*)img_ptr)[POS_MDS_TILEHEIGHT] = height;
}

inline void xfSetTilePosH(void* img_ptr, metadata_elem_t posH) {
    ((metadata_elem_t*)img_ptr)[POS_MDS_POSITIONH] = posH;
}

inline void xfSetTilePosV(void* img_ptr, metadata_elem_t posV) {
    ((metadata_elem_t*)img_ptr)[POS_MDS_POSITIONV] = posV;
}

inline void xfSetTileOVLP_HL(void* img_ptr, metadata_elem_t ovrlp_HL) {
    ((metadata_elem_t*)img_ptr)[POS_MDS_OVLPH_LEFT] = ovrlp_HL;
}

inline void xfSetTileOVLP_HR(void* img_ptr, metadata_elem_t ovrlp_HR) {
    ((metadata_elem_t*)img_ptr)[POS_MDS_OVLPH_RIGHT] = ovrlp_HR;
}

inline void xfSetTileOVLP_VT(void* img_ptr, metadata_elem_t ovrlp_VT) {
    ((metadata_elem_t*)img_ptr)[POS_MDS_OVLPV_TOP] = ovrlp_VT;
}

inline void xfSetTileOVLP_VB(void* img_ptr, metadata_elem_t ovrlp_VB) {
    ((metadata_elem_t*)img_ptr)[POS_MDS_OVLPV_BOTTOM] = ovrlp_VB;
}

inline void xfSetTileDatBitwidth(void* img_ptr, metadata_elem_t data_bitwidth) {
    ((metadata_elem_t*)img_ptr)[POS_MDS_DATA_BITWIDTH] = data_bitwidth;
}

inline void xfSetTileFinalWidth(void* img_ptr, metadata_elem_t final_width) {
    ((metadata_elem_t*)img_ptr)[POS_MDS_FINAL_WIDTH] = final_width;
}

inline void xfSetTileFinalHeight(void* img_ptr, metadata_elem_t final_height) {
    ((metadata_elem_t*)img_ptr)[POS_MDS_FINAL_HEIGHT] = final_height;
}

inline void xfSetTileOutPosH(void* img_ptr, metadata_elem_t out_posh) {
    ((metadata_elem_t*)img_ptr)[POS_MDS_OUT_POSH] = out_posh;
}

inline void xfSetTileOutPosV(void* img_ptr, metadata_elem_t out_posv) {
    ((metadata_elem_t*)img_ptr)[POS_MDS_OUT_POSV] = out_posv;
}

inline void xfSetTileOutTWidth(void* img_ptr, metadata_elem_t out_twidth) {
    ((metadata_elem_t*)img_ptr)[POS_MDS_OUT_TWIDTH] = out_twidth;
}

inline void xfSetTileOutTHeight(void* img_ptr, metadata_elem_t out_theight) {
    ((metadata_elem_t*)img_ptr)[POS_MDS_OUT_THEIGHT] = out_theight;
}

inline void xfSetTileOutOffset_L(void* img_ptr, metadata_elem_t out_offsetL) {
    ((metadata_elem_t*)img_ptr)[POS_MDS_OUT_OFFSET_LOWER] = out_offsetL;
}

inline void xfSetTileOutOffset_U(void* img_ptr, metadata_elem_t out_offsetU) {
    ((metadata_elem_t*)img_ptr)[POS_MDS_OUT_OFFSET_UPPER] = out_offsetU;
}

inline void xfUnsignedSaturation(void* img_ptr) {
    ((metadata_elem_t*)img_ptr)[POS_MDS_SAT_EN] = 1;
}

inline void xfDefaultSaturation(void* img_ptr) {
    ((metadata_elem_t*)img_ptr)[POS_MDS_SAT_EN] = 0;
}

inline void xfSignedSaturation(void* img_ptr) {
    ((metadata_elem_t*)img_ptr)[POS_MDS_SAT_EN] = 2;
}
//@}
}
}
}
#endif
