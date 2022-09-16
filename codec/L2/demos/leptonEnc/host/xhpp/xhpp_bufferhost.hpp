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

#ifndef _XHPP_HOSTBUFFER_
#define _XHPP_HOSTBUFFER_

#include "xhpp_error.hpp"
#include "xhpp_context.hpp"
#include "xhpp_bufferbase.hpp"
#include "xhpp_bufferdevice.hpp"
#include "xhpp_event.hpp"

#include <stdlib.h>
#include <iostream>
#include <vector>

namespace xhpp {

namespace task { // pre-declaration
template <typename _BUF_H, typename _BUF_D>
class tran_impl;
};

namespace vbuffer {

template <class T>
class device;

//! virtual host buffer objects
template <class T>
class host : public buffer::base {
    // friend class
    friend class xhpp::vbuffer::device<T>;

    template <typename _BUF_H, typename _BUF_D>
    friend class xhpp::task::tran_impl;

   private:
    unsigned int xsize = 0;
    std::vector<T*> xmems; //! data

    bool _sizeset = false;
    bool _bodyallocated = false;
    bool _shadowallocated = false;
    unsigned int _shadowsize = 0;

   public:
    //! constructors
    host(xhpp::context* ctx) : buffer::base(ctx) { xmems.resize(1); };

    host(xhpp::context* ctx, unsigned int num, T* data) : buffer::base(ctx) {
        xmems.resize(1);
        setsize(num);
        xmems[0] = data;
    };

    //! set buffer size
    int setsize(unsigned int num) {
        if (num <= 0) {
            throw xhpp::error("ERROR: xhpp::buffer::host::setsize(), input parameter should be larger than zero.\n");
        }
        xsize = num;
        _sizeset = true;
        return 0;
    };

    //    protected:

    //! buffer allocation (4K alignment)
    int bodyallocate(const int bank = 0) {
        void* ptr = nullptr;
        if (posix_memalign(&ptr, 4096, xsize * sizeof(T)))
            throw xhpp::error("ERROR: xhpp::buffer::host::bodyallocate(), allocation error.\n");
        xmems[0] = reinterpret_cast<T*>(ptr);

        _bodyallocated = true;
        return 0;
    };

    //! buffer (body and shadow) allocation
    //      int bodyshadowallocate(unsigned int nsize, const int* banks=NULL){
    int bodyshadowallocate(unsigned int nsize) {
        if (nsize > 0) {
            _shadowsize = nsize - 1;
            xmems.resize(nsize);

            bodyallocate(); // body

            if (_shadowallocated == false) { // shadow
                void* ptr = nullptr;
                // std::cout <<"nsize is "<<nsize<< std::endl;
                for (int i = 1; i < nsize; i++) {
                    if (posix_memalign(&ptr, 4096, xsize * sizeof(T)))
                        throw xhpp::error(
                            "ERROR: xhpp::buffer::host::bodyshadowallocate(), shadow allocation error.\n");
                    xmems[i] = reinterpret_cast<T*>(ptr);
                };
                _shadowallocated = true;
            };
        };
        return 0;
    };

    bool startingendingallocate() { return false; }

    //! buffer (body) free
    int bodyrelease() {
        if (_bodyallocated) {
            free(xmems[0]);
            _bodyallocated = false;
        }

        if (_shadowallocated == false) {
            xsize = 0;
        }
        return 0;
    };

    //! buffer (shadow) free
    int shadowrelease() {
        if (_shadowallocated) {
            free(xmems[1]);
            _shadowallocated = false;
        };

        if (_bodyallocated == false) {
            xsize = 0;
        }
        return 0;
    };

    //! buffer (body and shadow) free
    int bodyshadowrelease() {
        bodyrelease();
        shadowrelease();
        return 0;
    };

    //! body buffer allocated or not
    inline bool bodyallocated() { return _bodyallocated; };

    //! shadow buffer allocated or not
    inline bool shadowallocated() { return _shadowallocated; };

    //! operator []
    inline T& operator[](const unsigned int num) {
        if (num >= xsize) {
            throw xhpp::error("ERROR: xhpp::buffer::host operator[], access out of range.\n");
        };
        return xmems[0][num];
    };

    //! return size of buffer
    inline unsigned int size() { return xsize; };

    //! return pointer of underlying array
    inline T* dataptr(const int rc = 0) { return xmems[rc]; };

    //! set values
    inline int set(const T x, const int rc = 0) {
        for (int i = 0; i < xsize; i++) {
            xmems[rc][i] = x;
        };
        return 0;
    };

    //! set values
    inline int set(const T* vecx, const int rc = 0) {
        memcpy(xmems[rc], vecx, xsize * sizeof(T));
        return 0;
    };

    //! non-blocking copy from host to device
    int copy_to_device(xhpp::vbuffer::device<T>* dbuf,
                       xhpp::event* waitevt,
                       xhpp::event* evt,
                       const int rcfrom = 0,
                       const int rcto = 0) {
        cl_int err = clEnqueueWriteBuffer(xctx->xqueue, (dbuf->xmems)[rcto], CL_FALSE, 0, xsize * sizeof(T),
                                          xmems[rcfrom], waitevt->size(), waitevt->data(), evt->data());
        if (CL_SUCCESS != err) {
            throw xhpp::error(err,
                              "ERROR: xhpp::buffer::host::copy_to_device(), non-blocking data transfer from host "
                              "buffer to device buffer error.\n");
        };
#ifdef _XHPP_DEBUG_
        std::cout << "buffer from_host \n";
#endif
        return 0;
    };

    //! non-blocking copy from device to host
    int copy_from_device(xhpp::vbuffer::device<T>* dbuf,
                         xhpp::event* waitevt,
                         xhpp::event* evt,
                         const int rcfrom = 0,
                         const int rcto = 0) {
        cl_int err = clEnqueueReadBuffer(xctx->xqueue, (dbuf->xmems)[rcfrom], CL_FALSE, 0, xsize * sizeof(T),
                                         xmems[rcto], waitevt->size(), waitevt->data(), evt->data());
        if (CL_SUCCESS != err) {
            throw xhpp::error(err,
                              "ERROR: xhpp::buffer::host::copy_from_device(), non-blocking data transfer from device "
                              "buffer to host buffer error.\n");
        };
#ifdef _XHPP_DEBUG_
        std::cout << "Buffer to_host \n";
#endif
        return 0;
    };

    //! blocking copy from device to host
    inline int copy_from_device(xhpp::vbuffer::device<T>* dbuf, const int rcfrom = 0, const int rcto = 0) {
        cl_int err = clEnqueueReadBuffer(xctx->xqueue, (dbuf->xmems)[rcfrom], CL_TRUE, 0, xsize * sizeof(T),
                                         xmems[rcto], 0, NULL, NULL);
        if (CL_SUCCESS != err) {
            throw xhpp::error(err,
                              "ERROR: xhpp::buffer::host::copy_from_device(), blocking data transfer from device "
                              "buffer to host buffer error.\n");
        };
#ifdef _XHPP_DEBUG_
        std::cout << "buffer from_host \n";
#endif
        return 0;
    };

    //! blocking copy from deivce to host
    inline int copy_to_device(xhpp::vbuffer::device<T>* dbuf, const int rcfrom = 0, const int rcto = 0) {
        cl_int err = clEnqueueWriteBuffer(xctx->xqueue, (dbuf->xmems)[rcto], CL_TRUE, 0, xsize * sizeof(T),
                                          xmems[rcfrom], 0, NULL, NULL);
        if (CL_SUCCESS != err) {
            throw xhpp::error(err,
                              "ERROR: xhpp::buffer::host::copy_to_device(), blocking data transfer from host buffer to "
                              "device buffer error.\n");
        };
#ifdef _XHPP_DEBUG_
        std::cout << "Buffer to_host \n";
#endif
        return 0;
    };

}; // end of class vbuffer::host
}; // end of namespace vbuffer

namespace buffer {

template <class T>
class device;

//! host buffer objects
template <class T>
class host : public vbuffer::host<T> {
   public:
    //! constructor
    host(xhpp::context* ctx) : vbuffer::host<T>(ctx){};
    host(xhpp::context* ctx, unsigned int num, T* data) : vbuffer::host<T>(ctx, num, data){};

    //! buffer allocation (4K alignment)
    int allocate(unsigned int num) {
        vbuffer::host<T>::setsize(num);
        vbuffer::host<T>::bodyallocate();
        return 0;
    };

    //! buffer free
    int release() {
        vbuffer::host<T>::bodyrelease();
        vbuffer::host<T>::shadowrelease();
        return 0;
    };

    //! operator []
    inline T& operator[](const unsigned int num) { return vbuffer::host<T>::operator[](num); }

    //! return size of buffer
    inline unsigned int size() { return vbuffer::host<T>::size(); };

    //! pointer of underlying array
    inline T* dataptr(const int rc = 0) { return vbuffer::host<T>::dataptr(); };

    //! buffer allocated or not
    inline bool allocated() { return vbuffer::host<T>::bodyallocated(); };

    //! set value to all the elements
    inline int set(const T x) { return vbuffer::host<T>::set(x); };

    //! set values
    inline int set(const T* vecx) { return vbuffer::host<T>::set(vecx); };

    //! blocking copy from device to host
    int copy_from_device(xhpp::buffer::device<T>* dbuf) { return vbuffer::host<T>::copy_from_device(dbuf); };

    //! blocking copy from host to device
    int copy_to_device(xhpp::buffer::device<T>* dbuf) { return vbuffer::host<T>::copy_to_device(dbuf); };
};

}; // end of namespace buffer
}; // end of namespace xhpp

#endif
