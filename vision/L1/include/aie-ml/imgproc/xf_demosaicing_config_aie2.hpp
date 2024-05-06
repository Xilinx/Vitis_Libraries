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

#ifndef __XF_DEMOSAICING_CONFIG_HPP__
#define __XF_DEMOSAICING_CONFIG_HPP__

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

template <BayerPattern b, int INPUT_TILE_ELEMENTS>
class DemosaicBaseImpl;

template <BayerPattern b, int INPUT_TILE_ELEMENTS>
class DemosaicPlanar;

template <BayerPattern b, int INPUT_TILE_ELEMENTS>
class DemosaicRGBA;

} // aie
} // cv
} // xf

#endif
