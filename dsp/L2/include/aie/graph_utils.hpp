/*
 * Copyright 2022 Xilinx, Inc.
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
#ifndef _DSPLIB_GRAPH_UTILSHPP_
#define _DSPLIB_GRAPH_UTILSHPP_
/*
The file captures the definition of the graph utilities commonly used across various library elements
*/

#include <adf.h>
#include <vector>

namespace xf {
namespace dsp {
namespace aie {

using namespace adf;

class empty {};

template <typename DIRECTION, bool Condition>
using port_conditional = typename std::conditional<Condition, port<DIRECTION>, empty>::type;

template <typename DIRECTION, unsigned int SIZE>
using port_array = typename std::array<port<DIRECTION>, SIZE>;

template <typename DIRECTION, bool Condition, unsigned int SIZE>
using port_conditional_array = typename std::conditional<Condition, port_array<DIRECTION, SIZE>, empty>::type;
}
}
} // namespace braces
#endif //_DSPLIB_GRAPH_UTILSHPP_
