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

#ifndef _DEMOSAIC_RUNNER_H
#define _DEMOSAIC_RUNNER_H

#include <adf/window/types.h>
#include <adf/stream/types.h>
#include "adf.h"
//#include <adf/io_buffer/io_buffer.h>
#include "config.h"
#include "imgproc/xf_demosaicing_aie2.hpp"

class DemosaicRunner {
    static constexpr int INPUT_TILE_WIDTH = 128; // Assumes input tile is always 64 elements wide
    static constexpr int INPUT_TILE_HEIGHT = (TILE_ELEMENTS / 128);
    static constexpr int INTERLEAVE_TILE_HEIGHT = (INPUT_TILE_HEIGHT / 2) + 2;
    static constexpr int INTERLEAVE_TILE_ELEMENTS = (INPUT_TILE_WIDTH * INTERLEAVE_TILE_HEIGHT);

    uint8_t (&mInEven)[INTERLEAVE_TILE_ELEMENTS];
    uint8_t (&mInOdd)[INTERLEAVE_TILE_ELEMENTS];
    uint8_t (&mRChannel)[TILE_ELEMENTS];
    uint8_t (&mGChannel)[TILE_ELEMENTS];
    uint8_t (&mBChannel)[TILE_ELEMENTS];

   public:
    DemosaicRunner(uint8_t (&iEven)[INTERLEAVE_TILE_ELEMENTS],
                   uint8_t (&iOdd)[INTERLEAVE_TILE_ELEMENTS],
                   uint8_t (&rch)[TILE_ELEMENTS],
                   uint8_t (&gch)[TILE_ELEMENTS],
                   uint8_t (&bch)[TILE_ELEMENTS])
        : mInEven(iEven), mInOdd(iOdd), mRChannel(rch), mGChannel(gch), mBChannel(bch) {}
    void run(adf::input_buffer<uint8>& in, adf::output_buffer<uint8>& out);
    static void registerKernelClass() {
        REGISTER_FUNCTION(DemosaicRunner::run);
        REGISTER_PARAMETER(mInEven);
        REGISTER_PARAMETER(mInOdd);
        REGISTER_PARAMETER(mRChannel);
        REGISTER_PARAMETER(mGChannel);
        REGISTER_PARAMETER(mBChannel);
    }
};

#endif
