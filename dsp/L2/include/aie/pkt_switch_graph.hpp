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

#pragma once
#ifndef _DSPLIB_PKT_SWITCH_GRAPH_HPP_
#define _DSPLIB_PKT_SWITCH_GRAPH_HPP_

#include <adf.h>
#include <vector>

#include "device_defs.h"
#include "fir_utils.hpp"
#include "graph_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {

using namespace adf;

// ------------------------------------------------------------
// Top Level Graph Wrapper
// ------------------------------------------------------------

/**
 * @defgroup pkt_switch_graph Packet Switch Graphs
 *
 * Packet Switch Graphs Group contains graph classes for adding packet switching streaming in AIE DSP IP.
 *
 */

/**
 * @brief Graph class for adding packet switching streaming in AIE DSP IP.
 *
 *
 * This class implements a graph that splits incoming packet streams into multiple streams,
 * processes them using a wrapped graph instance, and then merges the processed streams back
 * into packet outputs.
 *
 * @ingroup pkt_switch_graph
 *
 * @tparam TP_SSR Number of super sample rate streams the wrapped graph instance uses.
 * @tparam TP_INPUT_PORTS Number of input packet ports.
 * @tparam TP_OUTPUT_PORTS Number of output packet ports.
 * @tparam TT_GRAPH_TYPE Type of the wrapped graph instance.
 *
 * @note The number of streams split from each packet input and merged to each packet output
 *       are calculated as N_STREAMS_SPLIT_FROM_PKT and N_STREAMS_MERGES_TO_PKT respectively.
 */
template <unsigned int TP_SSR, unsigned int TP_INPUT_PORTS, unsigned int TP_OUTPUT_PORTS, typename TT_GRAPH_TYPE>
class pkt_switch_graph : public graph {
   public:
    /**
     * @brief Number of streams each pktsplit element produces.
     */
    static constexpr unsigned N_STREAMS_SPLIT_FROM_PKT = TP_SSR / TP_INPUT_PORTS;

    /**
     * @brief Number of streams each pktmerge element consumes.
     */
    static constexpr unsigned N_STREAMS_MERGES_TO_PKT = TP_SSR / TP_OUTPUT_PORTS;

    /**
     * @brief Input ports.
     */
    std::array<port<input>, TP_INPUT_PORTS> pkt_in;

    /**
     * @brief Output ports.
     */
    std::array<port<output>, TP_OUTPUT_PORTS> pkt_out;

    /**
     * @brief Array (TP_INPUT_PORTS) of pktsplit elements. Each pktsplit takes input packet stream and splits it into
     * N_STREAMS_SPLIT_FROM_PKT streams.
     */
    std::array<pktsplit<N_STREAMS_SPLIT_FROM_PKT>, TP_INPUT_PORTS> split;

    /**
     * @brief Array (TP_OUTPUT_PORTS) of pktmerge elements. Each pktmerge takes input stream and merges it into a single
     * output packet stream.
     */
    std::array<pktmerge<N_STREAMS_MERGES_TO_PKT>, TP_OUTPUT_PORTS> merge;

    /**
     * @brief Wrapped graph class instance.
     */
    TT_GRAPH_TYPE graph_instance;

    /**
     * @brief Returns the kernel architectures used in the wrapped graph instance.
     * @return Kernel architectures.
     */
    auto getKernelArchs() { return graph_instance.getKernelArchs(); }

    /**
     * @brief Returns the kernels used in the wrapped graph instance.
     * @return Kernels.
     */
    auto getKernels() { return graph_instance.getKernels(); }

    /**
     * @brief Returns the kernels for a specific SSR index.
     * @param ssrindex Super sample rate index.
     * @return Kernels for the specified SSR index.
     */
    auto getKernels(int ssrindex) { return graph_instance.getKernels(ssrindex); }

    /**
     * @brief Returns the kernels for specific output path, input phase, and cascade position.
     * @param ssrOutPathIndex Output path index.
     * @param ssrInPhaseIndex Input phase index.
     * @param cascadePosition Cascade position.
     * @return Kernels for the specified parameters.
     */
    auto getKernels(int ssrOutPathIndex, int ssrInPhaseIndex, int cascadePosition) {
        return graph_instance.getKernels(ssrOutPathIndex, ssrInPhaseIndex, cascadePosition);
    }

    /**
     * @brief Constructs the graph with provided filter coefficients.
     * @param taps Vector of filter coefficients for the wrapped graph instance.
     */
    pkt_switch_graph(std::vector<typename TT_GRAPH_TYPE::BTT_COEFF> taps) : graph_instance(taps) {
        create_connections();
    }

    /**
     * @brief Constructs the graph with provided filter coefficients.
     */
    pkt_switch_graph() : graph_instance() { create_connections(); }

    /**
     * @brief Creates the connections between input ports, splitters, wrapped graph instance, mergers, and output ports.
     */
    void create_connections() {
        // Create split & connect input ports:
        for (unsigned pp = 0; pp < TP_INPUT_PORTS; pp++) {
            split[pp] = pktsplit<N_STREAMS_SPLIT_FROM_PKT>::create();
            connect<>(pkt_in[pp], split[pp].in[0]);
        }
        // Create merge & connect output ports:
        for (unsigned pp = 0; pp < TP_OUTPUT_PORTS; pp++) {
            merge[pp] = pktmerge<N_STREAMS_MERGES_TO_PKT>::create();
            connect<>(merge[pp].out[0], pkt_out[pp]);
        }
        // Connect split to wrapped object:
        for (unsigned inputPktPort = 0; inputPktPort < TP_INPUT_PORTS; inputPktPort++) {
            for (unsigned inputSplitPort = 0; inputSplitPort < N_STREAMS_SPLIT_FROM_PKT; inputSplitPort++) {
                unsigned tile = inputPktPort * N_STREAMS_SPLIT_FROM_PKT + inputSplitPort;
                connect<>(split[inputPktPort].out[inputSplitPort], graph_instance.in[tile]);
            }
        }
        // Connect wrapped object to merge:
        for (unsigned outputPorts = 0; outputPorts < TP_OUTPUT_PORTS; outputPorts++) {
            for (unsigned mm = 0; mm < N_STREAMS_MERGES_TO_PKT; mm++) {
                unsigned tile = outputPorts * N_STREAMS_MERGES_TO_PKT + mm;
                connect<>(graph_instance.out[tile], merge[outputPorts].in[mm]);
            }
        }
    }
};

/**
  * @cond NOCOMMENTS
  */

template <unsigned int TP_SSR, unsigned int TP_INPUT_PORTS>
class pkt_switch_input : public graph {
   public:
    static constexpr unsigned N_STREAMS_SPLIT_FROM_PKT = TP_SSR / TP_INPUT_PORTS;

    std::array<port<input>, TP_INPUT_PORTS> pkt_in;
    std::array<port<input>, TP_SSR> pkt_ssr_out;

    std::array<pktsplit<N_STREAMS_SPLIT_FROM_PKT>, TP_INPUT_PORTS> split;

    pkt_switch_input() { create_connections(); }

    void create_connections() {
        // Create split & connect input ports:
        for (unsigned pp = 0; pp < TP_INPUT_PORTS; pp++) {
            split[pp] = pktsplit<N_STREAMS_SPLIT_FROM_PKT>::create();
            connect<>(pkt_in[pp], split[pp].in[0]);
        }

        // Connect split to wrapped object:
        for (unsigned inputPktPort = 0; inputPktPort < TP_INPUT_PORTS; inputPktPort++) {
            for (unsigned inputSplitPort = 0; inputSplitPort < N_STREAMS_SPLIT_FROM_PKT; inputSplitPort++) {
                unsigned tile = inputPktPort * N_STREAMS_SPLIT_FROM_PKT + inputSplitPort;
                connect<>(split[inputPktPort].out[inputSplitPort], pkt_ssr_out[tile]);
            }
        }
    }
};

template <unsigned int TP_SSR, unsigned int TP_OUTPUT_PORTS>
class pkt_switch_output : public graph {
   public:
    static constexpr unsigned N_STREAMS_MERGES_TO_PKT = TP_SSR / TP_OUTPUT_PORTS;

    std::array<port<output>, TP_SSR> pkt_ssr_in;
    std::array<port<output>, TP_OUTPUT_PORTS> pkt_out;

    std::array<pktmerge<N_STREAMS_MERGES_TO_PKT>, TP_OUTPUT_PORTS> merge;

    pkt_switch_output() { create_connections(); }

    void create_connections() {
        // Create merge & connect output ports:
        for (unsigned pp = 0; pp < TP_OUTPUT_PORTS; pp++) {
            merge[pp] = pktmerge<N_STREAMS_MERGES_TO_PKT>::create();
            connect<>(merge[pp].out[0], pkt_out[pp]);
        }

        // Connect wrapped object to merge:
        for (unsigned outputPorts = 0; outputPorts < TP_OUTPUT_PORTS; outputPorts++) {
            for (unsigned mm = 0; mm < N_STREAMS_MERGES_TO_PKT; mm++) {
                unsigned tile = outputPorts * N_STREAMS_MERGES_TO_PKT + mm;
                connect<>(pkt_ssr_in[tile], merge[outputPorts].in[mm]);
            }
        }
    }
};

/**
 * @endcond
 */

} // namespace aie
} // namespace dsp
} // namespace xf

#endif //_DSPLIB_PKT_SWITCH_GRAPH_HPP_
