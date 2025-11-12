/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_PLIO_FILE_CONNECTIONS_HPP_
#define _DSPLIB_PLIO_FILE_CONNECTIONS_HPP_

/*
This file holds the definition of the dds_mixer
Reference model graph.
*/

#include <adf.h>
#include <vector>
#include "device_defs.h"
#include "test_stim.hpp"
#include "graph_utils.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)

namespace xf {
namespace dsp {
namespace aie {
namespace testcase {
using namespace adf;

template <unsigned int ssr, unsigned int dual, typename plioType, unsigned int myPlioWidth = 64>
void createPLIOFileConnections(std::array<plioType, ssr*(dual + 1)>& plioPorts,
                               std::string filename,
                               std::string plioDescriptor = "in") {
    plio_type plioAlias = (myPlioWidth == 64) ? adf::plio_64_bits : adf::plio_32_bits;
    for (unsigned int ssrIdx = 0; ssrIdx < ssr; ++ssrIdx) {
        for (unsigned int dualIdx = 0; dualIdx < (dual + 1); ++dualIdx) {
            std::string filenameInternal = filename;
            // Insert SSR index and dual stream index into filename before extension (.txt)
            filenameInternal.insert(filenameInternal.length() - 4,
                                    ("_" + std::to_string(ssrIdx) + "_" + std::to_string(dualIdx)));
            plioPorts[ssrIdx * (dual + 1) + dualIdx] = plioType::create(
                "PLIO_" + plioDescriptor + "_" + std::to_string(ssrIdx) + "_" + std::to_string(dualIdx),
                (myPlioWidth == 64) ? adf::plio_64_bits : adf::plio_32_bits, filenameInternal);
        }
    }
}
}
}
}
}
#endif // _DSPLIB_PLIO_FILE_CONNECTIONS_HPP_
