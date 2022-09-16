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
           Copyright (c) 2019, Xilinx, Inc.
           All rights reserved.

           TODO

**********/

#ifndef _XHPP_SCHEDULER_
#define _XHPP_SCHEDULER_

#include "xhpp_event.hpp"
#include "xhpp_graph.hpp"
#include "xhpp_context.hpp"
#include "xhpp_taskbase.hpp"
#include "xhpp_enums.hpp"
#include <string>
#include <sys/time.h>

namespace xhpp {

//! scheduler
class engine {
   private:
    //! list of event
    std::vector<xhpp::event> evt;

    //! graph
    xhpp::graph* xgr;

    //! context
    xhpp::context* xctx;

    //!
    unsigned int NumSubmit = 0;

    //! num of resource = body + shadowsize
    unsigned int NumDepth = 1;

    //! event to start the first host func task.
    cl_event ent0;

    //! global counter to indicate whether to use buffer or shadow buffer
    int shadow_rc = 0;

    //! to record the num of calling data_inout API.
    int inout_rc = 0; // TODO: same as numsubmit? If so, delete one.

   public:
    // ------------------------------------------------------------

    int tvdiff(struct timeval* tv0, struct timeval* tv1) {
        return (tv1->tv_sec - tv0->tv_sec) * 1000000 + (tv1->tv_usec - tv0->tv_usec);
    }

    // ------------------------------------------------------------

    engine(xhpp::context* ctx, xhpp::graph* gr) {
        xctx = ctx;
        xgr = gr;
        evt.resize(gr->GetNumNodes());
    };

    //! get the max number of CU between all nodes
    int getnumcu() {
        int maxcu = 0;
        for (int i = 0; i < xgr->GetNumNodes(); i++) {
            int numcu = (xgr->nodes)[i]->numofcu();
            if (maxcu < numcu) {
                maxcu = numcu;
            }
        }
        return maxcu;
    }

    //! setup the scheduler, allocate the body and shadow resources
    int setup() {
        int maxcu = getnumcu();
        if ((xctx->pattern) == xhpp::linear) { // linear mode
            std::cout << "INFO: Engine is setup with linear mode." << std::endl;
            NumDepth = maxcu;
            std::cout << "NumDepth = " << NumDepth << std::endl;
            for (int i = 0; i < xgr->GetNumNodes(); i++) {
                (xgr->nodes)[i]->setupbodyshadow(maxcu); // TODO: allocate only necessary buffers
            }
        } else { // pipeline mode
            std::cout << "INFO: Engine is setup with pipeline mode." << std::endl;
            NumDepth = 2 * maxcu;
            std::cout << "NumDepth = " << NumDepth << std::endl;
            for (int i = 0; i < xgr->GetNumNodes(); i++) {
                (xgr->nodes)[i]->setupbodyshadow(NumDepth); // TODO: allocate only necessary buffers
                std::cout << "passe " << i << std::endl;
            }
        }
    };

    //! submit each node in graph
    int submitnode(unsigned int nd) {
        std::vector<unsigned int> fatherevt = xgr->GetFatherEvent(nd); // father events
        std::vector<unsigned int> childevt = xgr->GetChildEvent(nd);   // child events
        xhpp::event waitevt(0);
        // events need to be wait
        int numfatherevt = fatherevt.size();
        int numchildevt;
        if (NumSubmit < NumDepth) {
            numchildevt = 0;
        } else {
            numchildevt = childevt.size();
        }
        int numwaitevt = numfatherevt + numchildevt;

        waitevt.resize(numwaitevt);

        for (int i = 0; i < numfatherevt; i++) {
            waitevt[i] = evt[NumSubmit][fatherevt[i]];
        };
        for (int i = 0; i < numchildevt; i++) {
            waitevt[i + numfatherevt] = evt[NumSubmit - NumDepth][childevt[i]];
        };

        // output event
        xhpp::event outevt(1);

        int idxin, idxout;
        int idxrun = 0;
        int nt = (xgr->nodestype)[nd];
        if (nt == xhpp::start) {
            idxin = 0;
            idxout = NumSubmit % NumDepth;
        } else if (nt == xhpp::end) {
            idxin = NumSubmit % NumDepth;
            idxout = 0;
        } else {
            idxin = NumSubmit % NumDepth;
            idxout = NumSubmit % NumDepth;
        }
        idxrun = NumSubmit % NumDepth;

// submit
// idxin, idxout are introduced to present the data input/output buffer index: the body buffer or shadow buffer.
// idxrun refers to which kenel is used, the body or the shadow (and which shadow kernel.)
#ifdef _XHPP_TIMING_
        struct timeval st_time, end_time;
        xctx->wait();
        gettimeofday(&st_time, 0);
        for (int kk = 0; kk < 6; kk++) {
#endif
            (xgr->nodes)[nd]->submit(&waitevt, &outevt, idxin, idxout, idxrun); // idxrun = numsubmit??
#ifdef _XHPP_TIMING_
            xctx->wait();
        }
        gettimeofday(&end_time, 0);
        int exec_time = tvdiff(&st_time, &end_time);
        std::cout << "Execution time of task:  " << nd << " is " << exec_time / 6 << " us." << std::endl;
#endif
        evt[NumSubmit][nd] = outevt[0]; // comment: pointer of pointer
        return 0;
    };

    //! submit graph by submitting each node.
    int submitgraph() {
        // std::cout <<"------------- submitting graph -------------" << std::endl;
        evt.resize(NumSubmit + 1);
        evt[NumSubmit].resize(xgr->GetNumNodes());
        for (int i = 0; i < xgr->GetNumNodes(); i++) {
            submitnode(i);
            // xctx->wait();
            std::cout << "node: " << i << " passed" << std::endl;
        }
        if ((xctx->pattern) == xhpp::linear) { // linear mode
            xctx->wait();
        }
    }

    //! for the interface task, input data
    template <typename TASK, typename PARAM>
    int data_input(TASK* node, PARAM* param, int order) {
        node->updateparam(inout_rc, order, param);
    };
    //! for the interface task, input data
    template <typename TASK, typename PARAM>
    int data_input(TASK* node, PARAM param, int order) {
        node->updateparam(inout_rc, order, param);
    };
    //! for the interface task, output data
    template <typename TASK, typename PARAM>
    int data_output(TASK* node, PARAM* param, int order) {
        node->updateparam(inout_rc, order, param);
    };
    //! for the interface task, output data
    template <typename TASK, typename PARAM>
    int data_output(TASK* node, PARAM param, int order) {
        node->updateparam(inout_rc, order, param);
    };
    //! the last run
    template <typename TASK, typename PARAM>
    int run(std::string inorout, TASK* node, PARAM* param, int order) {
        if (inorout == "input") {
            data_input(node, param, order);
        } else if (inorout == "output") {
            data_output(node, param, order);
        };
        submitgraph();
        NumSubmit++;
        inout_rc = (inout_rc + 1) % NumDepth;
        return 0;
    };
    //! the last run
    template <typename TASK, typename PARAM>
    int run(std::string inorout, TASK* node, PARAM param, int order) {
        if (inorout == "input") {
            data_input(node, param, order);
        } else if (inorout == "output") {
            data_output(node, param, order);
        };
        submitgraph();
        NumSubmit++;
        inout_rc = (inout_rc + 1) % NumDepth;
        return 0;
    };
    //! the top API of data input and output
    template <typename TASK, typename PARAM, typename... __Args>
    int run(std::string inorout, TASK* node, PARAM* param, int order, __Args... __args) {
        if (inorout == "input") {
            data_input(node, param, order);
        } else if (inorout == "output") {
            data_output(node, param, order);
        }
        run(__args...);
    };
    //! the top API of data input and output
    template <typename TASK, typename PARAM, typename... __Args>
    int run(std::string inorout, TASK* node, PARAM param, int order, __Args... __args) {
        if (inorout == "input") {
            data_input(node, param, order);
        } else if (inorout == "output") {
            data_output(node, param, order);
        }
        run(__args...);
    };

}; // end of class

}; // end of namespace

#endif
