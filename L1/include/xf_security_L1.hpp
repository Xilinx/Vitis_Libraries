/*
 * Copyright 2019 Xilinx, Inc.
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

/**
 * @file xf_security.hpp
 * @brief Top-level header for HLS Security Library.
 */

#ifndef _XF_SECURITY_L1_HPP_
#define _XF_SECURITY_L1_HPP_

#include "xf_security/aes.hpp"
#include "xf_security/cbc.hpp"
#include "xf_security/cfb.hpp"
#include "xf_security/ctr.hpp"
#include "xf_security/ecb.hpp"
#include "xf_security/gcm.hpp"
#include "xf_security/ofb.hpp"
#include "xf_security/xts.hpp"

#include "xf_security/md4.hpp"
#include "xf_security/md5.hpp"

#include "xf_security/sha1.hpp"
#include "xf_security/sha224_256.hpp"
#include "xf_security/sha512_t.hpp"

#include "xf_security/blake2b.hpp"

#endif
