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

#ifndef _XHPP_BASETASK_
#define _XHPP_BASETASK_

#include "xhpp_event.hpp"
#include "xhpp_context.hpp"
#include "xhpp_enums.hpp"

namespace xhpp {
namespace task {

//! base class of task objects
class base {
   public:
    xhpp::context* xctx;

    //! constructor
    base(xhpp::context* ctx) { xctx = ctx; }

    //! deconstructor
    ~base(){};

    //! number of CUs
    virtual int numofcu() = 0;

    //! setup body and shadow
    virtual int setupbodyshadow(unsigned int) = 0;

    //! release
    virtual int release() = 0;

    //! submit (non-blocking)
    virtual int submit(
        xhpp::event* waitevt, xhpp::event* outevt, const int rcin = 0, const int rcout = 0, const int rcrun = 0) = 0;

    //! run (blocking submit)
    virtual int run(const int rcin = 0, const int rcout = 0, const int rcrun = 0) = 0;
};

}; // end of namespace task
}; // end of namespace xhpp

#endif
