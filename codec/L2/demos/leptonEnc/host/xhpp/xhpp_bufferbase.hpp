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

#ifndef _XHPP_BASEBUFFER_
#define _XHPP_BASEBUFFER_

#include "xhpp_context.hpp"

namespace xhpp {

namespace buffer {

//! base class of buffer objects
class base {
   protected:
    xhpp::context* xctx = nullptr;

   public:
    //! constructor
    base(xhpp::context* ctx) { xctx = ctx; }

    // //! allocation
    // virtual int allocate(unsigned int) = 0;

    //! buffer (body) allocation
    virtual int bodyallocate(const int = 0) = 0;

    //! buffer (shadow) allocation
    virtual int bodyshadowallocate(const unsigned int) = 0;

    //! buffer (body) free
    virtual int bodyrelease() = 0;

    //! buffer (shadow) free
    virtual int shadowrelease() = 0;

    //! buffer (body and shadow) free
    virtual int bodyshadowrelease() = 0;

    //! starting/ending vbuffer, do not allocate
    virtual bool startingendingallocate() = 0;
};

}; // end of namespace buffer

}; // end of namespace xhpp
#endif
