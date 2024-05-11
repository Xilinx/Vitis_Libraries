/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_GRAPH_UTILS_HPP_
#define _DSPLIB_GRAPH_UTILS_HPP_
/*
The file captures the definition of the graph utilities commonly used across various library elements
*/

#include <adf.h>
#include <vector>

namespace xf {
namespace dsp {
namespace aie {
using namespace adf;

/**
 * @ingroup graph_utils
 *
 * @brief empty class is a helper utility to conditionally remove instances of other classes.
 */
class empty {};

/**
 * @ingroup graph_utils
 *
 * @brief no_port class is a helper utility to conditionally remove instances of other classes.
 */
class no_port {};

//--------------------------------------------------------------------------------------------------
// port_conditional
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup graph_utils
 *
 * @brief port_conditional is an helper alias to conditionally instance port.
 * @tparam DIRECTION describes port direction: input, output or inout.
 * @tparam Condition when met port of a DIRECTION is created, otherwise a no_port struct is instanced.
 */
template <typename DIRECTION, bool Condition>
using port_conditional = typename std::conditional<Condition, port<DIRECTION>, no_port>::type;

//--------------------------------------------------------------------------------------------------
// port_array
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup graph_utils
 *
 * @brief port_array is an helper alias to instance port array
 * Uses: ``std::array`` to instance an array of ``port<DIRECTION>`` classes.
 * @tparam DIRECTION describes port direction: input, output or inout.
 *
 * @tparam SIZE array size.
 */
template <typename DIRECTION, unsigned int SIZE>
using port_array = typename std::array<port<DIRECTION>, SIZE>;

//--------------------------------------------------------------------------------------------------
// port_array
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup graph_utils
 *
 * @brief port_conditional_array is an helper alias to conditionally instance port array
 * Uses: ``std::conditional`` to instance a ``port_array<DIRECTION, SIZE>`` class or an empty ``no_port`` struct array.
 * @tparam DIRECTION describes port direction: input, output or inout.
 * @tparam Condition when met port of a DIRECTION is created, otherwise a no_port struct is instanced.
 * @tparam SIZE array size.
 */
template <typename DIRECTION, bool Condition, unsigned int SIZE>
using port_conditional_array =
    typename std::conditional<Condition, port_array<DIRECTION, SIZE>, std::array<no_port, SIZE> >::type;
}
}
} // namespace braces
#endif //_DSPLIB_GRAPH_UTILS_HPP_
