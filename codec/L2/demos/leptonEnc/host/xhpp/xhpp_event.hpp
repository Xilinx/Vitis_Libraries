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
#ifndef _XHPP_EVENT_
#define _XHPP_EVENT_

#include "CL/cl.h"
#include <vector>

namespace xhpp {
//! xhpp event objects
class event {
   private:
   public:
    std::vector<cl_event> evtvec;

    event(size_t n) : evtvec(n){};

    event(){};

    size_t size() { return evtvec.size(); };

    void resize(size_t n) { return evtvec.resize(n); };

    cl_event* data() {
        if (evtvec.size() == 0)
            return nullptr;
        else
            return evtvec.data();
    }

    cl_event& operator[](size_t n) { return evtvec[n]; }
};
};

#endif
