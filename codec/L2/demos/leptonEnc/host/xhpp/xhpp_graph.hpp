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
/**********
           Copyright (c) 2018, Xilinx, Inc.
           All rights reserved.

           TODO

**********/

#ifndef _XHPP_GRAPH_
#define _XHPP_GRAPH_

#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>

#include "xhpp_taskbase.hpp"

namespace xhpp {

//! node type enum
enum NodeTYPE { middle = 0, start = 1, end = 2 };

//! Graph object
class graph {
    //  private:
   public:
    bool issetup = false;

    //! number of nodes
    unsigned int numnodes = 0;

    //! number of edges
    unsigned int numedges = 0;

    //! node list (node, name, type)
    std::vector<xhpp::task::base*> nodes;
    std::vector<std::string> nodesname;
    std::vector<NodeTYPE> nodestype;

    //! edge list
    std::vector<std::pair<unsigned int, unsigned int> > edges;

    //! father event list
    std::vector<std::vector<unsigned int> > fatherevtlists;

    //! child event list
    std::vector<std::vector<unsigned int> > childevtlists;

   private:
    //! father events (events that this one depends on)
    std::vector<unsigned int> FatherEventList(const unsigned int nd) {
        std::vector<unsigned int> fatherevt;
        for (auto s = edges.begin(); s < edges.end(); s++) {
            if (s->second == nd) {
                fatherevt.push_back(s->first);
            };
        };
        return fatherevt;
    };

    //! child events (events depending on this)
    std::vector<unsigned int> ChildEventList(const unsigned int nd) {
        std::vector<unsigned int> childevt;
        for (auto s = edges.begin(); s < edges.end(); s++) {
            if (s->first == nd) {
                childevt.push_back(s->second);
            };
        };
        return childevt;
    };

   public:
    //! add node to graph
    template <class TASK>
    int addnode(TASK* node, std::string name, NodeTYPE nt = xhpp::middle) {
        nodes.push_back(node);
        nodesname.push_back(name);
        nodestype.push_back(nt);
        numnodes += 1;
        return 0;
    };

    //! add edge to graph
    int addedge(std::string node_s, std::string node_e) {
        auto its = std::find(nodesname.begin(), nodesname.end(), node_s);
        auto ite = std::find(nodesname.begin(), nodesname.end(), node_e);

        if (its != nodesname.end() && ite != nodesname.end()) {
            auto idxs = std::distance(nodesname.begin(), its);
            auto idxe = std::distance(nodesname.begin(), ite);
            edges.push_back(std::pair<unsigned int, unsigned int>(idxs, idxe));
        } else {
            throw xhpp::error("ERROR: xhpp::graph::addedge(), node does not existe.\n");
        };
        numedges += 1;
        return 0;
    };

    //! setup graph
    int setup() {
        for (unsigned int i = 0; i < numnodes; i++) {
            std::vector<unsigned int> fevt = FatherEventList(i);
            std::vector<unsigned int> cevt = ChildEventList(i);
            fatherevtlists.push_back(fevt);
            childevtlists.push_back(cevt);
        };
        issetup = true;
        return 0;
    };

    //! release graph
    int release() {
        nodes.resize(0);
        nodesname.resize(0);
        nodestype.resize(0);
        edges.resize(0);
        for (unsigned int i = 0; i < numnodes; i++) {
            fatherevtlists[i].resize(0);
            childevtlists[i].resize(0);
        };
        fatherevtlists.resize(0);
        childevtlists.resize(0);
        numnodes = 0;
        numedges = 0;
        issetup = false;
    };

    //! return number of nodes
    unsigned int GetNumNodes() { return numnodes; };

    //! return number of edges
    unsigned int GetNumEdges() { return numedges; };

    //! Get father events (events that this one depends on)
    std::vector<unsigned int> GetFatherEvent(const unsigned int nd) {
        if (issetup == false) {
            throw xhpp::error("ERROR: xhpp::graph::GetFatherEvent(), graph is not setup.\n");
        };
        return fatherevtlists[nd];
    };

    //! Get child events (events depending on this)
    std::vector<unsigned int> GetChildEvent(const unsigned int nd) {
        if (issetup == false) {
            throw xhpp::error("ERROR: xhpp::graph::GetChildEvent(), graph is not setup.\n");
        };
        return childevtlists[nd];
    };

    int removenode(){
        // TODO
    };

    int removeedge(){
        // TODO
    };

    int check(){
        // TODO
    };

    int debug(){
        // std::cout << " ** DEBUG INFO ** " << std::endl;
        // nodes[0]->run();
        // nodes[1]->run();
        // nodes[2]->run();
        // std::cout << nodes.size() << std::endl;
        // std::cout << edges[0].first << " " << edges[0].second << std::endl;
        // std::cout << edges[1].first << " " << edges[1].second << std::endl;
    };

    void drawgraph() {
        std::cout << "drawing the graph..." << std::endl;
        std::ofstream myfile;
        myfile.open("gr.dot");
        myfile << "digraph g { \n";
        for (auto s = edges.begin(); s != edges.end(); s++) {
            auto node1st = nodesname.begin();
            auto node2nd = nodesname.begin();
            std::advance(node1st, s->first);
            std::advance(node2nd, s->second);
            std::cout << *node1st << " -> " << *node2nd << std::endl;
            myfile << "    " << *node1st << "->" << *node2nd << "\n";
        }
        myfile << "}\n";
    };

}; // end of class graph
}; // end of namesapce

#endif
