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
#ifndef _DSPLIB_WIDGET_API_CAST_HPP_
#define _DSPLIB_WIDGET_API_CAST_HPP_

/*
Widget API Cast Kernel.
This file exists to capture the definition of the widget api cast kernel class.
The class definition holds defensive checks on parameter range and other
legality.
The constructor definition is held in this class because this class must be
accessible to graph level aie compilation.
The main runtime filter function is captured elsewhere as it contains aie
intrinsics which are not included in aie graph level
compilation.
*/

/* Coding conventions
   TT_      template type suffix
   TP_      template parameter suffix
*/

/* Design Notes

*/

#include <adf.h>
#include <vector>

using namespace adf;

#include "widget_api_cast_traits.hpp"

//#define _DSPLIB_WIDGET_API_CAST_HPP_DEBUG_

#include "device_defs.h"

namespace xf {
namespace dsp {
namespace aie {
namespace widget {
namespace api_cast {

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          unsigned int TP_IN_API,
          unsigned int TP_OUT_API,
          unsigned int TP_NUM_INPUTS,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES,
          unsigned int TP_PATTERN,
          unsigned int TP_HEADER_BYTES = 0>
class kernelClass {
   private:
    // Parameter value defensive and legality checks
    static_assert(TP_IN_API <= 2, "ERROR: Unsupported TP_IN_API value set. ");
    static_assert(TP_WINDOW_VSIZE * sizeof(TT_DATA) % 16 == 0, "ERROR: TP_WINDOW_VSIZE must be a multiple of 128bits");

   public:
    // Constructor
    kernelClass() {}

    void kernelClassMain(T_inputIF<TT_DATA, TP_IN_API> inInterface, T_outputIF<TT_DATA, TP_OUT_API> outInterface);
};

template <typename TT_DATA,
          unsigned int TP_NUM_INPUTS,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES,
          unsigned int TP_PATTERN,
          unsigned int TP_HEADER_BYTES>
class kernelClass<TT_DATA,
                  kStreamAPI,
                  kWindowAPI,
                  TP_NUM_INPUTS,
                  TP_WINDOW_VSIZE,
                  TP_NUM_OUTPUT_CLONES,
                  TP_PATTERN,
                  TP_HEADER_BYTES> {
   private:
   public:
    // Constructor
    kernelClass() {}

    void kernelClassMain(T_inputIF<TT_DATA, kStreamAPI> inInterface, T_outputIF<TT_DATA, kWindowAPI> outInterface);
};

template <typename TT_DATA,
          unsigned int TP_NUM_INPUTS,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES,
          unsigned int TP_PATTERN,
          unsigned int TP_HEADER_BYTES>
class kernelClass<TT_DATA,
                  kWindowAPI,
                  kStreamAPI,
                  TP_NUM_INPUTS,
                  TP_WINDOW_VSIZE,
                  TP_NUM_OUTPUT_CLONES,
                  TP_PATTERN,
                  TP_HEADER_BYTES> {
   private:
   public:
    // Constructor
    kernelClass() {}

    void kernelClassMain(T_inputIF<TT_DATA, kWindowAPI> inInterface, T_outputIF<TT_DATA, kStreamAPI> outInterface);
    // Multiple Window 2 Single Stream
    void kernelClassMW2SS(T_inputIF<TT_DATA, kWindowAPI> inInterface, T_outputIF<TT_DATA, kStreamAPI> outInterface);
};

template <typename TT_DATA,
          unsigned int TP_NUM_INPUTS,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES,
          unsigned int TP_PATTERN,
          unsigned int TP_HEADER_BYTES>
class kernelClass<TT_DATA,
                  kWindowAPI,
                  kWindowAPI,
                  TP_NUM_INPUTS,
                  TP_WINDOW_VSIZE,
                  TP_NUM_OUTPUT_CLONES,
                  TP_PATTERN,
                  TP_HEADER_BYTES> {
   private:
   public:
    // Constructor
    kernelClass() {}

    // Single Window 2 Single Window
    void kernelClassMain(T_inputIF<TT_DATA, kWindowAPI> inInterface, T_outputIF<TT_DATA, kWindowAPI> outInterface);
    // Multiple Window 2 Single Window
    void kernelClassMW2SW(T_inputIF<TT_DATA, kWindowAPI> inInterface, T_outputIF<TT_DATA, kWindowAPI> outInterface);
};

template <typename TT_DATA,
          unsigned int TP_NUM_INPUTS,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES,
          unsigned int TP_PATTERN,
          unsigned int TP_HEADER_BYTES>
class kernelClass<TT_DATA,
                  kCascStreamAPI,
                  kWindowAPI,
                  TP_NUM_INPUTS,
                  TP_WINDOW_VSIZE,
                  TP_NUM_OUTPUT_CLONES,
                  TP_PATTERN,
                  TP_HEADER_BYTES> {
   private:
   public:
    // Constructor
    kernelClass() {}

    void kernelClassMain(T_inputIF<TT_DATA, kCascStreamAPI> inInterface, T_outputIF<TT_DATA, kWindowAPI> outInterface);
};

template <typename TT_DATA,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES,
          unsigned int TP_PATTERN,
          unsigned int TP_HEADER_BYTES>
class kernelClass<TT_DATA,
                  kWindowAPI,
                  kCascStreamAPI,
                  1,
                  TP_WINDOW_VSIZE,
                  TP_NUM_OUTPUT_CLONES,
                  TP_PATTERN,
                  TP_HEADER_BYTES> {
   private:
   public:
    // Constructor
    kernelClass() {}

    void kernelClassMain(T_inputIF<TT_DATA, kWindowAPI> inInterface, T_outputIF<TT_DATA, kCascStreamAPI> outInterface);
};

template <typename TT_DATA,
          unsigned int TP_NUM_INPUTS,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES,
          unsigned int TP_PATTERN,
          unsigned int TP_HEADER_BYTES>
class kernelClass<TT_DATA,
                  kStreamCascAPI,
                  kWindowAPI,
                  TP_NUM_INPUTS,
                  TP_WINDOW_VSIZE,
                  TP_NUM_OUTPUT_CLONES,
                  TP_PATTERN,
                  TP_HEADER_BYTES> {
   private:
   public:
    // Constructor
    kernelClass() {}

    void kernelClassMain(T_inputIF<TT_DATA, kStreamCascAPI> inInterface, T_outputIF<TT_DATA, kWindowAPI> outInterface);
};

template <typename TT_DATA,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES,
          unsigned int TP_PATTERN,
          unsigned int TP_HEADER_BYTES>
class kernelClass<TT_DATA,
                  kWindowAPI,
                  kStreamCascAPI,
                  1,
                  TP_WINDOW_VSIZE,
                  TP_NUM_OUTPUT_CLONES,
                  TP_PATTERN,
                  TP_HEADER_BYTES> {
   private:
   public:
    // Constructor
    kernelClass() {}

    void kernelClassMain(T_inputIF<TT_DATA, kWindowAPI> inInterface, T_outputIF<TT_DATA, kStreamCascAPI> outInterface);
};

//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
// Single kernel base specialization. Used for single window to single window copy
template <typename TT_DATA,
          unsigned int TP_IN_API,
          unsigned int TP_OUT_API,
          unsigned int TP_NUM_INPUTS,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES,
          unsigned int TP_PATTERN,
          unsigned int TP_HEADER_BYTES = 0>
class widget_api_cast : public kernelClass<TT_DATA,
                                           TP_IN_API,
                                           TP_OUT_API,
                                           TP_NUM_INPUTS,
                                           TP_WINDOW_VSIZE,
                                           TP_NUM_OUTPUT_CLONES,
                                           TP_PATTERN,
                                           TP_HEADER_BYTES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA,
                      TP_IN_API,
                      TP_OUT_API,
                      TP_NUM_INPUTS,
                      TP_WINDOW_VSIZE,
                      TP_NUM_OUTPUT_CLONES,
                      TP_PATTERN,
                      TP_HEADER_BYTES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_buffer<TT_DATA>& __restrict inWindow0, output_buffer<TT_DATA>& __restrict outWindow0);
};

// Specialization for single window in, dual window out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
class widget_api_cast<TT_DATA, kWindowAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>
    : public kernelClass<TT_DATA, kWindowAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA, kWindowAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_buffer<TT_DATA>& __restrict inWindow0,
                      output_buffer<TT_DATA>& __restrict outWindow0,
                      output_buffer<TT_DATA>& __restrict outWindow1);
};

// Specialization for single window in, triple window out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
class widget_api_cast<TT_DATA, kWindowAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3, TP_PATTERN, TP_HEADER_BYTES>
    : public kernelClass<TT_DATA, kWindowAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3, TP_PATTERN, TP_HEADER_BYTES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA, kWindowAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3, TP_PATTERN, TP_HEADER_BYTES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_buffer<TT_DATA>& __restrict inWindow0,
                      output_buffer<TT_DATA>& __restrict outWindow0,
                      output_buffer<TT_DATA>& __restrict outWindow1,
                      output_buffer<TT_DATA>& __restrict outWindow2);
};

// stream to  window, 1 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
class widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>
    : public kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_stream<TT_DATA>* __restrict inStream0, output_buffer<TT_DATA>& __restrict outWindow0);
};

// stream to  window, 2 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
class widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>
    : public kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_stream<TT_DATA>* __restrict inStream0,
                      output_buffer<TT_DATA>& __restrict outWindow0,
                      output_buffer<TT_DATA>& __restrict outWindow1);
};

// stream to  window, 3 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
class widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3, TP_PATTERN, TP_HEADER_BYTES>
    : public kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3, TP_PATTERN, TP_HEADER_BYTES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3, TP_PATTERN, TP_HEADER_BYTES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_stream<TT_DATA>* __restrict inStream0,
                      output_buffer<TT_DATA>& __restrict outWindow0,
                      output_buffer<TT_DATA>& __restrict outWindow1,
                      output_buffer<TT_DATA>& __restrict outWindow2);
};

// stream to  window, 4 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
class widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 4, TP_PATTERN, TP_HEADER_BYTES>
    : public kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 4, TP_PATTERN, TP_HEADER_BYTES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 4, TP_PATTERN, TP_HEADER_BYTES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_stream<TT_DATA>* __restrict inStream0,
                      output_buffer<TT_DATA>& __restrict outWindow0,
                      output_buffer<TT_DATA>& __restrict outWindow1,
                      output_buffer<TT_DATA>& __restrict outWindow2,
                      output_buffer<TT_DATA>& __restrict outWindow3);
};

// dual stream input
// stream to  window, 2 in 1 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
class widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>
    : public kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_stream<TT_DATA>* __restrict inStream0,
                      input_stream<TT_DATA>* __restrict inStream1,
                      output_circular_buffer<TT_DATA>& __restrict outWindow0);
};

// stream to  window, 2 in 2 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
class widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>
    : public kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_stream<TT_DATA>* __restrict inStream0,
                      input_stream<TT_DATA>* __restrict inStream1,
                      output_buffer<TT_DATA>& __restrict outWindow0,
                      output_buffer<TT_DATA>& __restrict outWindow1);
};

// stream to  window, 2 in 3 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
class widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 3, TP_PATTERN, TP_HEADER_BYTES>
    : public kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 3, TP_PATTERN, TP_HEADER_BYTES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 3, TP_PATTERN, TP_HEADER_BYTES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_stream<TT_DATA>* __restrict inStream0,
                      input_stream<TT_DATA>* __restrict inStream1,
                      output_buffer<TT_DATA>& __restrict outWindow0,
                      output_buffer<TT_DATA>& __restrict outWindow1,
                      output_buffer<TT_DATA>& __restrict outWindow2);
};

// stream to  window, 2 in 4 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
class widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 4, TP_PATTERN, TP_HEADER_BYTES>
    : public kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 4, TP_PATTERN, TP_HEADER_BYTES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 4, TP_PATTERN, TP_HEADER_BYTES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_stream<TT_DATA>* __restrict inStream0,
                      input_stream<TT_DATA>* __restrict inStream1,
                      output_buffer<TT_DATA>& __restrict outWindow0,
                      output_buffer<TT_DATA>& __restrict outWindow1,
                      output_buffer<TT_DATA>& __restrict outWindow2,
                      output_buffer<TT_DATA>& __restrict outWindow3);
};

// Window to stream 1 to 1
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
class widget_api_cast<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>
    : public kernelClass<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_buffer<TT_DATA>& __restrict inWindow0, output_stream<TT_DATA>* __restrict outStream0);
};

// Window to stream 1 to 2
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
class widget_api_cast<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>
    : public kernelClass<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_buffer<TT_DATA>& __restrict inWindow0,
                      output_stream<TT_DATA>* __restrict outStream0,
                      output_stream<TT_DATA>* __restrict outStream1);
};
#ifdef __SUPPORTS_ACC64__
// CascStream to  window, 2 in 1 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
class widget_api_cast<TT_DATA, kCascStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>
    : public kernelClass<TT_DATA, kCascStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA, kCascStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_cascade<cacc64>* __restrict inStream0,
                      input_stream<TT_DATA>* __restrict inStream1,
                      output_circular_buffer<TT_DATA>& __restrict outWindow0);
};
#endif //__SUPPORTS_ACC64__

#ifdef __SUPPORTS_ACC64__
// StreamCasc to  window, 2 in 1 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
class widget_api_cast<TT_DATA, kStreamCascAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>
    : public kernelClass<TT_DATA, kStreamCascAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA, kStreamCascAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_stream<TT_DATA>* __restrict inStream0,
                      input_cascade<cacc64>* __restrict inStream1,
                      output_circular_buffer<TT_DATA>& __restrict outWindow0);
};
#endif //__SUPPORTS_ACC64__

#ifdef __SUPPORTS_ACC64__
// Window to CascStream 1 to 2
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
class widget_api_cast<TT_DATA, kWindowAPI, kCascStreamAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>
    : public kernelClass<TT_DATA, kWindowAPI, kCascStreamAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA, kWindowAPI, kCascStreamAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_buffer<TT_DATA>& __restrict inWindow0,
                      output_cascade<cacc64>* __restrict outStream0,
                      output_stream<TT_DATA>* __restrict outStream1);
};
#endif //__SUPPORTS_ACC64__

#ifdef __SUPPORTS_ACC64__
// Window to StreamCasc 1 to 2
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
class widget_api_cast<TT_DATA, kWindowAPI, kStreamCascAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>
    : public kernelClass<TT_DATA, kWindowAPI, kStreamCascAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA, kWindowAPI, kStreamCascAPI, 1, TP_WINDOW_VSIZE, 2, TP_PATTERN, TP_HEADER_BYTES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_buffer<TT_DATA>& __restrict inWindow0,
                      output_stream<TT_DATA>* __restrict outStream0,
                      output_cascade<cacc64>* __restrict outStream1);
};
#endif //__SUPPORTS_ACC64__

// Window to stream 2 to 1 interleave
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
class widget_api_cast<TT_DATA, kWindowAPI, kStreamAPI, 2, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>
    : public kernelClass<TT_DATA, kWindowAPI, kStreamAPI, 2, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA, kWindowAPI, kStreamAPI, 2, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_buffer<TT_DATA>& __restrict inWindow0,
                      input_buffer<TT_DATA>& __restrict inWindow1,
                      output_stream<TT_DATA>* __restrict outStream0);
};

// Window to stream 3 to 1 interleave
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
class widget_api_cast<TT_DATA, kWindowAPI, kStreamAPI, 3, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>
    : public kernelClass<TT_DATA, kWindowAPI, kStreamAPI, 3, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA, kWindowAPI, kStreamAPI, 3, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_buffer<TT_DATA>& __restrict inWindow0,
                      input_buffer<TT_DATA>& __restrict inWindow1,
                      input_buffer<TT_DATA>& __restrict inWindow2,
                      output_stream<TT_DATA>* __restrict outStream0);
};

// Window to window 2 to 1 interleave
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
class widget_api_cast<TT_DATA, kWindowAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>
    : public kernelClass<TT_DATA, kWindowAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA, kWindowAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_buffer<TT_DATA>& __restrict inWindow0,
                      input_buffer<TT_DATA>& __restrict inWindow1,
                      output_buffer<TT_DATA>& __restrict outWindow0);
};

// Window to window 3 to 1 interleave
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
class widget_api_cast<TT_DATA, kWindowAPI, kWindowAPI, 3, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>
    : public kernelClass<TT_DATA, kWindowAPI, kWindowAPI, 3, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA, kWindowAPI, kWindowAPI, 3, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_buffer<TT_DATA>& __restrict inWindow0,
                      input_buffer<TT_DATA>& __restrict inWindow1,
                      input_buffer<TT_DATA>& __restrict inWindow2,
                      output_buffer<TT_DATA>& __restrict outWindow0);
};

// Window to window 4 to 1 interleave
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
class widget_api_cast<TT_DATA, kWindowAPI, kWindowAPI, 4, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>
    : public kernelClass<TT_DATA, kWindowAPI, kWindowAPI, 4, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA, kWindowAPI, kWindowAPI, 4, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_buffer<TT_DATA>& __restrict inWindow0,
                      input_buffer<TT_DATA>& __restrict inWindow1,
                      input_buffer<TT_DATA>& __restrict inWindow2,
                      input_buffer<TT_DATA>& __restrict inWindow3,
                      output_buffer<TT_DATA>& __restrict outWindow0);
};

// Window to window 5 to 1 interleave
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
class widget_api_cast<TT_DATA, kWindowAPI, kWindowAPI, 5, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>
    : public kernelClass<TT_DATA, kWindowAPI, kWindowAPI, 5, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA, kWindowAPI, kWindowAPI, 5, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_buffer<TT_DATA>& __restrict inWindow0,
                      input_buffer<TT_DATA>& __restrict inWindow1,
                      input_buffer<TT_DATA>& __restrict inWindow2,
                      input_buffer<TT_DATA>& __restrict inWindow3,
                      input_buffer<TT_DATA>& __restrict inWindow4,
                      output_buffer<TT_DATA>& __restrict outWindow0);
};

// Window to window 6 to 1 interleave
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
class widget_api_cast<TT_DATA, kWindowAPI, kWindowAPI, 6, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>
    : public kernelClass<TT_DATA, kWindowAPI, kWindowAPI, 6, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA, kWindowAPI, kWindowAPI, 6, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_buffer<TT_DATA>& __restrict inWindow0,
                      input_buffer<TT_DATA>& __restrict inWindow1,
                      input_buffer<TT_DATA>& __restrict inWindow2,
                      input_buffer<TT_DATA>& __restrict inWindow3,
                      input_buffer<TT_DATA>& __restrict inWindow4,
                      input_buffer<TT_DATA>& __restrict inWindow5,
                      output_buffer<TT_DATA>& __restrict outWindow0);
};

// Window to window 7 to 1 interleave
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
class widget_api_cast<TT_DATA, kWindowAPI, kWindowAPI, 7, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>
    : public kernelClass<TT_DATA, kWindowAPI, kWindowAPI, 7, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA, kWindowAPI, kWindowAPI, 7, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_buffer<TT_DATA>& __restrict inWindow0,
                      input_buffer<TT_DATA>& __restrict inWindow1,
                      input_buffer<TT_DATA>& __restrict inWindow2,
                      input_buffer<TT_DATA>& __restrict inWindow3,
                      input_buffer<TT_DATA>& __restrict inWindow4,
                      input_buffer<TT_DATA>& __restrict inWindow5,
                      input_buffer<TT_DATA>& __restrict inWindow6,
                      output_buffer<TT_DATA>& __restrict outWindow0);
};

// Window to window 7 to 1 interleave
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_PATTERN, unsigned int TP_HEADER_BYTES>
class widget_api_cast<TT_DATA, kWindowAPI, kWindowAPI, 8, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>
    : public kernelClass<TT_DATA, kWindowAPI, kWindowAPI, 8, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA, kWindowAPI, kWindowAPI, 8, TP_WINDOW_VSIZE, 1, TP_PATTERN, TP_HEADER_BYTES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_buffer<TT_DATA>& __restrict inWindow0,
                      input_buffer<TT_DATA>& __restrict inWindow1,
                      input_buffer<TT_DATA>& __restrict inWindow2,
                      input_buffer<TT_DATA>& __restrict inWindow3,
                      input_buffer<TT_DATA>& __restrict inWindow4,
                      input_buffer<TT_DATA>& __restrict inWindow5,
                      input_buffer<TT_DATA>& __restrict inWindow6,
                      input_buffer<TT_DATA>& __restrict inWindow7,
                      output_buffer<TT_DATA>& __restrict outWindow0);
};
}
}
}
}
}

#endif // _DSPLIB_WIDGET_API_CAST_HPP_
