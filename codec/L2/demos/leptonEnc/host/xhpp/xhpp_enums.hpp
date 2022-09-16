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
#ifndef _XHPP_ENUMS_H_
#define _XHPP_ENUMS_H_

namespace xhpp {

//! graph running modes
enum Pattern { linear = 1, pipeline = 2 };

//! data transfer mode enum
enum DataTransMode { host2dev = 1, dev2host = 2 };
};

#endif
