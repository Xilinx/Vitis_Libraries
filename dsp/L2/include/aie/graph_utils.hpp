/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
#include "device_defs.h"

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

//--------------------------------------------------------------------------------------------------
// fn_exceeds_half_data_mem / conditional_single_buffer
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup graph_utils
 *
 * @brief Returns true when a buffer of NUM_SAMPLES elements of type TT_DATA would occupy more
 * than half of the device's data memory module, in which case double-buffering that port is
 * infeasible and single_buffer() must be applied.
 * @tparam TT_DATA the sample data type carried by the port.
 * @tparam NUM_SAMPLES number of samples the port's buffer will hold (as passed to dimensions()).
 */
template <typename TT_DATA, unsigned int NUM_SAMPLES>
static constexpr bool fn_exceeds_half_data_mem() {
    return (NUM_SAMPLES * sizeof(TT_DATA)) > (__DATA_MEM_BYTES__ / 2);
}

/**
 * @ingroup graph_utils
 *
 * @brief Applies single_buffer() to one or more ports when their buffer size (NUM_SAMPLES elements
 * of type TT_DATA) exceeds half of the device's data memory module. Otherwise a no-op, leaving the
 * ADF default (double buffering) in place.
 * @tparam TT_DATA the sample data type carried by the port(s).
 * @tparam NUM_SAMPLES number of samples the port's buffer will hold.
 * @param ports one or more ADF ports that share this sizing decision (e.g. a tiler's in/out ports
 * and the downstream kernel's in port, which must agree on single-vs-double buffering together).
 */
template <typename TT_DATA, unsigned int NUM_SAMPLES, typename... Ports>
static void conditional_single_buffer(Ports&... ports) {
    if
        constexpr(fn_exceeds_half_data_mem<TT_DATA, NUM_SAMPLES>()) { (single_buffer(ports), ...); }
}
}
}
} // namespace braces
#endif //_DSPLIB_GRAPH_UTILS_HPP_
