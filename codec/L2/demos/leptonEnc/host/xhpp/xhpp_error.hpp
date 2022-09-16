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

#ifndef _XHPP_ERROR_
#define _XHPP_ERROR_

#include <stdexcept>
#include <string>

namespace xhpp {

class error : public std::runtime_error {
    unsigned int err_code; //! error code

   public:
    error(unsigned int ec, const std::string& what = "") : std::runtime_error(what), err_code(ec){};

    error(const std::string& what) : std::runtime_error(what), err_code(0){};
};
}
#endif
