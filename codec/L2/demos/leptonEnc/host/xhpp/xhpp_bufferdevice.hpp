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

#ifndef _XHPP_DEVICEBUFFER_
#define _XHPP_DEVICEBUFFER_

// #define _XHPP_DEBUG_

#include "xhpp_event.hpp"
#include "xhpp_bufferbase.hpp"
#include "xhpp_bufferhost.hpp"

#include "CL/cl.h"
#include "CL/cl_ext_xilinx.h"

#include <iostream>

namespace xhpp {

namespace task { // pre-declaration
template <typename _BUF_H, typename _BUF_D>
class tran_impl;
};
namespace task {
class dev_func;
}

namespace vbuffer {

template <class T>
class host;

template <typename _BUF_H, typename _BUF_D>
class tran_impl;

//! virtual device buffer objects
template <class T>
class device : public buffer::base {
    // friend class
    friend class xhpp::vbuffer::host<T>;

    template <typename _BUF_H, typename _BUF_D>
    friend class xhpp::task::tran_impl;

    friend class xhpp::task::dev_func;

   private:
    unsigned int xsize = 0;    //! buffer size
    std::vector<cl_mem> xmems; //! cl_mem objs

    bool _sizeset = false;
    bool _bodyallocated = false;
    bool _shadowallocated = false;
    unsigned int _shadowsize = 0;

    bool _vbuffer_startorend = false;
    // xhpp::host_buffer<T>* hbmap;

   public:
    std::vector<int> bankinfo;

    //! constructors
    device(xhpp::context* ctx) : buffer::base(ctx) { xmems.resize(1); };

    device(xhpp::context* ctx, bool vbuf_startorend) : buffer::base(ctx) {
        xmems.resize(1);
        _vbuffer_startorend = vbuf_startorend;
    };

    device(xhpp::context* ctx, unsigned int num, cl_mem mem) : buffer::base(ctx) {
        xmems.resize(1);
        setsize(num);
        xmems[0] = mem;
    };

    //! set buffer size
    int setsize(unsigned int num) {
        if (num <= 0) {
            throw xhpp::error("ERROR: xhpp::buffer::device::setsize(), input parameter should be larger than zero.\n");
        }
        xsize = num;
        _sizeset = true;
        return 0;
    };

    //    protected:

    //! allocate buffer
    int allocate(const unsigned int n, const int bank = 0) {
        cl_mem_ext_ptr_t ptr;
        if (bank == 0) {
            ptr.flags = XCL_MEM_DDR_BANK0;
        } else if (bank == 1) {
            ptr.flags = XCL_MEM_DDR_BANK1;
        } else if (bank == 2) {
            ptr.flags = XCL_MEM_DDR_BANK2;
        } else if (bank == 3) {
            ptr.flags = XCL_MEM_DDR_BANK3;
        } else {
            throw xhpp::error("ERROR: xhpp::buffer::device::allocate(), bank should be 0 ~ 4.\n");
        };
        ptr.obj = 0;
        ptr.param = 0;

        cl_mem_flags flags = CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX;
        cl_int err;
        xmems[n] = clCreateBuffer(xctx->xcontext, flags, xsize * sizeof(T), &ptr, &err);
        if (err != CL_SUCCESS) {
            throw xhpp::error("ERROR: xhpp::buffer::device::allocate(), allocation error.\n");
        };

        //! let buffer be resident
        err = clEnqueueMigrateMemObjects(xctx->xqueue, 1, &xmems[n], CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0, NULL,
                                         NULL);
        if (err != CL_SUCCESS) {
            throw xhpp::error("ERROR: xhpp::buffer::device::allocate(), allocation (being resident) error.\n");
        };

        xctx->wait();

        return 0;
    };

    //! buffer (body) allocation
    int bodyallocate(const int bank = 0) {
        if (_bodyallocated == false) {
            // std::cout <<"allocate bankinfo[0] "<<bankinfo[0] <<std::endl;
            allocate(0, bank);
            _bodyallocated = true;
        };
        return 0;
    };

    //! buffer (body and shadow) allocation
    //      int bodyshadowallocate(unsigned int nsize, const int* banks=NULL){
    int bodyshadowallocate(unsigned int nsize) {
        if (nsize > 0) {
            _shadowsize = nsize - 1;
            xmems.resize(nsize);

            bodyallocate(bankinfo[0]); // body

            if (_shadowallocated == false) { // shadow
                for (int i = 1; i < nsize; i++) {
                    if (xctx->pattern == xhpp::linear) {
                        allocate(i, bankinfo[i]);
#ifdef _XHPP_DEBUG_
                        std::cout << "allocating bankinfo[" << i << "] " << bankinfo[i] << std::endl;
#endif
                    } else { // pipeline mode, the 12, 34, 56 used bankinfo are the same.
                        allocate(i, bankinfo[i / 2]);
#ifdef _XHPP_DEBUG_
                        std::cout << "allocating bankinfo[" << i << "] " << bankinfo[i / 2] << std::endl;
#endif
                    }
                };
                _shadowallocated = true;
            };
        };
        return 0;
    };

    //! if dev vbuffer is starting or ending point, do not allocate to real buffer.
    bool startingendingallocate() { return _vbuffer_startorend; }

    //! buffer (body) free
    int bodyrelease() {
        if (_bodyallocated) {
            clReleaseMemObject(xmems[0]);
            _bodyallocated = false;
        };

        if (_shadowallocated == false) {
            xsize = 0;
        };
        return 0;
    };

    //! buffer (shadow) free
    int shadowrelease() {
        if (_shadowallocated) {
            for (int i = 0; i < _shadowsize; i++) {
                clReleaseMemObject(xmems[i + 1]);
            }
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

    //! non-blocking copy from host to device
    int copy_from_host(xhpp::vbuffer::host<T>* hbuf,
                       xhpp::event* waitevt,
                       xhpp::event* evt,
                       const int rcfrom = 0,
                       const int rcto = 0) {
        cl_int err = clEnqueueWriteBuffer(xctx->xqueue, xmems[rcto], CL_FALSE, 0, xsize * sizeof(T),
                                          (hbuf->xmems)[rcfrom], waitevt->size(), waitevt->data(), evt->data());
        if (CL_SUCCESS != err) {
            throw xhpp::error(err,
                              "ERROR: xhpp::buffer::device::copy_from_device(), non-blocking data transfer from host "
                              "buffer to device buffer error.\n");
        };
//   clFinish(xctx->xqueue);
#ifdef _XHPP_DEBUG_
        std::cout << "buffer from_host \n";
#endif
        return 0;
    };

    //! non-blocking copy from deivce to host
    int copy_to_host(xhpp::vbuffer::host<T>* hbuf,
                     xhpp::event* waitevt,
                     xhpp::event* evt,
                     const int rcfrom = 0,
                     const int rcto = 0) {
        cl_int err = clEnqueueReadBuffer(xctx->xqueue, xmems[rcfrom], CL_FALSE, 0, xsize * sizeof(T),
                                         (hbuf->xmems)[rcto], waitevt->size(), waitevt->data(), evt->data());
        if (CL_SUCCESS != err) {
            throw xhpp::error(err,
                              "ERROR: xhpp::buffer::device::copy_to_host(), non-blocking data transfer from device "
                              "buffer to host buffer error.\n");
        };
#ifdef _XHPP_DEBUG_
        std::cout << "Buffer to_host \n";
#endif
        return 0;
    };

    //! blocking copy from host to device
    inline int copy_from_host(xhpp::vbuffer::host<T>* hbuf, const int rcfrom = 0, const int rcto = 0) {
        cl_int err = clEnqueueWriteBuffer(xctx->xqueue, xmems[rcto], CL_TRUE, 0, xsize * sizeof(T),
                                          (hbuf->xmems)[rcfrom], 0, NULL, NULL);
        if (CL_SUCCESS != err) {
            throw xhpp::error(err,
                              "ERROR: xhpp::buffer::device::copy_from_host(), blocking data transfer from host buffer "
                              "to device buffer error.\n");
        };
#ifdef _XHPP_DEBUG_
        std::cout << "buffer from_host \n";
#endif
        return 0;
    };

    //! blocking copy from device to host
    inline int copy_to_host(xhpp::vbuffer::host<T>* hbuf, const int rcfrom = 0, const int rcto = 0) {
        cl_int err = clEnqueueReadBuffer(xctx->xqueue, xmems[rcfrom], CL_TRUE, 0, xsize * sizeof(T),
                                         (hbuf->xmems)[rcto], 0, NULL, NULL);
        if (CL_SUCCESS != err) {
            throw xhpp::error(err,
                              "ERROR: xhpp::buffer::device::copy_to_host(), blocking data transfer from device buffer "
                              "to host buffer error.\n");
        };
#ifdef _XHPP_DEBUG_
        std::cout << "Buffer to_host \n";
#endif
        return 0;
    };

    //! blocking copy from device to device
    int copy_to_device(xhpp::vbuffer::device<T>* dbuf) {
        cl_int err =
            clEnqueueCopyBuffer(xctx->xqueue, xmems[0], (dbuf->xmems)[0], 0, 0, xsize * sizeof(T), 0, NULL, NULL);
        if (CL_SUCCESS != err) {
            throw xhpp::error(err,
                              "ERROR: xhpp::buffer::device::copy_to_device(), blocking data transfer from device "
                              "buffer to device buffer error.\n");
        };
        //        clFinish(xctx->xqueue);
        return 0;
    };

    //! non-blocking copy from device to device
    int copy_to_device(xhpp::vbuffer::device<T>* dbuf, xhpp::event* waitevt, xhpp::event* evt) {
        cl_int err = clEnqueueCopyBuffer(xctx->xqueue, xmems[0], (dbuf->xmems)[0], 0, 0, xsize * sizeof(T),
                                         waitevt->size(), waitevt->data(), evt->data());
        if (CL_SUCCESS != err) {
            throw xhpp::error(err,
                              "ERROR: xhpp::buffer::device::copy_to_device(), non-blocking data transfer from device "
                              "buffer to device buffer error.\n");
        };
        //        clFinish(xctx->xqueue);
        return 0;
    };

}; // end of class vbuffer::device
}; // end of namespace vbuffer

namespace buffer {

template <class T>
class host;

//! device buffer objects
template <class T>
class device : public vbuffer::device<T> {
   public:
    //! constructors
    device(xhpp::context* ctx) : vbuffer::device<T>(ctx){};
    device(xhpp::context* ctx, unsigned int num, cl_mem mem) : vbuffer::device<T>(ctx, num, mem){};

    //! buffer allocation
    int allocate(unsigned int num, const int bank = 0) {
#ifdef _XHPP_DEBUG_
        std::cout << "in buffer allocate" << std::endl;
#endif
        vbuffer::device<T>::setsize(num);
        vbuffer::device<T>::bodyallocate(bank); // TODO
        return 0;
    };

    //! buffer free
    int release() {
        vbuffer::device<T>::bodyrelease();
        return 0;
    };

    //! blocking copy from device to host
    int copy_from_host(xhpp::buffer::host<T>* hbuf) { return vbuffer::device<T>::copy_from_host(hbuf); };

    //! blocking copy from host to device
    int copy_to_host(xhpp::buffer::host<T>* hbuf) { return vbuffer::device<T>::copy_to_host(hbuf); };

    //! blocking copy from device to device
    int copy_to_device(xhpp::buffer::device<T>* dbuf) { return vbuffer::device<T>::copy_to_device(dbuf); };
    //! non-blocking copy from device to device
    int copy_to_device(xhpp::buffer::device<T>* dbuf, xhpp::event* waitevt, xhpp::event* evt) {
        return vbuffer::device<T>::copy_to_device(dbuf, waitevt, evt);
    };
};

}; // end of namespace buffer
}; // end of namespace xhpp

#endif
