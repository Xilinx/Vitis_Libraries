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
#ifndef _XILINX_APPS_COMMON_H_
#define _XILINX_APPS_COMMON_H_

/**
 * @brief Simple string class for avoiding ABI problems
 *
 * This class provides string memory management like `std::string` but without ABI compatibility issues.
 * ABI (Application Binary Interface) problems can arise when using standard C++ classes in a coding environment
 * that uses multiple compiler versions.  For example, if you compile your application code using a different
 * version of the g++ compiler from the one used to compile this library, then when using dynamic loading,
 * standard C++ types, such as `std::string`, may not pass correctly from your code into the library.
 * This string class avoids these compatibility issues by using only plain data types internally.
 */
class XString {
   public:
    XString() = default;
    ~XString() { clear(); }
    XString(const XString& other) { copyIn(other.data); }
    XString(XString&& other) { steal(std::forward<XString>(other)); }
    XString(const char* cstr) { copyIn(cstr); }
    XString(const std::string& str) { copyIn(str.c_str()); }
    XString& operator=(const XString& other) {
        copyIn(other.data);
        return *this;
    }
    XString& operator=(XString&& other) {
        steal(std::forward<XString>(other));
        return *this;
    }
    XString& operator=(const char* cstr) {
        copyIn(cstr);
        return *this;
    }
    XString& operator=(const std::string& str) {
        copyIn(str.c_str());
        return *this;
    }
    operator std::string() const { return std::string(c_str()); }
    operator const char*() const { return c_str(); }
    const char* c_str() const { return data == nullptr ? "" : data; }
    bool empty() const { return data == nullptr || std::strlen(data) == 0; }

    bool operator==(const XString& other) const {
        if (data == nullptr && other.data == nullptr) return true;
        if (data == nullptr || other.data == nullptr) return false;
        return std::strcmp(data, other.data) == 0;
    }

    void clear() {
        if (data != nullptr) std::free(data);
        data = nullptr;
    }

   private:
    char* data = nullptr;

    void copyIn(const char* other) {
        clear();
        if (other != nullptr) {
            data = static_cast<char*>(std::malloc(std::strlen(other) + 1));
            std::strcpy(data, other);
        }
    }

    void steal(XString&& other) {
        clear();
        data = other.data;
        other.data = nullptr;
    }
};

// TODO: Move this to internal header after create/load_alveo_partitions are removed
enum { ALVEOAPI_NONE = 0, ALVEOAPI_PARTITION, ALVEOAPI_PARTITION_BFS, ALVEOAPI_LOAD, ALVEOAPI_RUN };

enum { LOUVAINMOD_PRUNING_KERNEL = 1, LOUVAINMOD_2CU_U55C_KERNEL = 2 };

enum {
    ERRORCODE_COMPUTE_LOUVAIN_ALVEO_SEPERATED_LOAD = -3,
    ERRORCODE_CREATESHAREDHANDLE = -4,
    ERRORCODE_GETNUMPARTITIONS_INVALID_ARGC = -5,
    ERRORCODE_GETNUMPARTITIONS_INVALID_NUMPARS = -6,
    ERRORCODE_GETNUMPARTITIONS_FAILED_OPEN_ALVEOPRJ = -7,
};

/**
 * Define this macro to make functions in louvainmod_loader.cpp inline instead of extern.  You would use this macro
 * when including louvainmod_loader.cpp in a header file, as opposed to linking with libXilinxCosineSim_loader.a.
 */
#ifdef XILINX_LOUVAINMOD_INLINE_IMPL
#define XILINX_LOUVAINMOD_IMPL_DECL inline
#else
#define XILINX_LOUVAINMOD_IMPL_DECL extern
#endif

#endif
