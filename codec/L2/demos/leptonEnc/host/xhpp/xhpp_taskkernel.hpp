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

#ifndef _XHPP_KERNELTASK_
#define _XHPP_KERNELTASK_

#include <iostream>
#include <string>
#include <functional>
#include <memory>
#include <tuple>

#include "xhpp_taskbase.hpp"
#include "xhpp_context.hpp"
#include "xhpp_bufferdevice.hpp"
#include "xhpp_enums.hpp"

// #define _XHPP_DEBUG_

namespace xhpp {
namespace task {

//! kernel function class
class dev_func : public base {
   private:
    // cl_kernel mkernel;
    std::vector<cl_kernel> mkernels;

    //! param list
    class dev_func_arglist {
       public:
        std::vector<size_t> argsize; // sizeof(arg)
        std::vector<void*> argvalue;
        std::vector<xhpp::buffer::base*> devbuffer;
        std::vector<int> startendmark; // when the input arg is starting or ending device vbuffer, mark as 1
    } arglist;

    //! number of CUs
    int _numcu = 0;
    int numofcu() { return _numcu; }

    //! kernel CU name
    std::vector<std::string> _cuname;
    std::vector<int> _cubank;
    //! kernel params: number of device buffer type
    int db_n = 0; // vadd: db_n = 3

    bool _cuadded = false;
    bool _paramset = false;

    bool _bodyallocated = false;
    bool _bodyshadowallocated = false;

    unsigned int _shadowsize = 0;

    //! update _shadowsize for kernel task
    // for lienar mode, shadowsize=0,
    // pipeline mode, shadowsize=1
    int updateshadowsize(const Pattern pattern) {
        if (pattern == pipeline) {
            _shadowsize = 1;
        }
        return _shadowsize;
    }

    //! device buffer type arg index
    int argidx_db = 0;

    //! setup one kernel arg with dataype T, the params other than dbuf args.
    template <typename T>
    int set_single_arg(T& value) {
        // push to arg list
        arglist.argsize.push_back(sizeof(T));
        arglist.startendmark.push_back(0);
        for (int i = 0; i < _numcu * (_shadowsize + 1); i++) {
            arglist.argvalue.push_back((void*)(&value));
        }
    };

    //! setup one kernel arg with dataype cl_mem, the device buffer params
    template <typename T>
    int set_single_arg(xhpp::vbuffer::device<T>& value) {
        // push to arg list
        arglist.argsize.push_back(sizeof(cl_mem));
        if (value._vbuffer_startorend == false) {
            arglist.startendmark.push_back(0);
        } else {
            arglist.startendmark.push_back(1);
        }
        value.xmems.resize(_numcu * (_shadowsize + 1));
        for (int i = 0; i < _numcu * (_shadowsize + 1); i++) {
            arglist.argvalue.push_back((void*)(&(value.xmems[i])));
        }
        arglist.devbuffer.push_back(&value);

        // bankinfo is added to the devicebuffer
        for (int i = 0; i < _numcu; i++) {
            value.bankinfo.push_back(_cubank[argidx_db + i * db_n]);
#ifdef _XHPP_DEBUG_
            std::cout << "bank info is " << _cubank[argidx_db + i * db_n] << std::endl;
#endif
        }
        argidx_db++;
        return 0;
    };

    //! setup the last kernel arg
    template <typename T>
    int setargs(T& arg1) {
        set_single_arg(arg1);
        return 0;
    };

    //! setup kernel args recursively
    template <typename T, typename... U>
    int setargs(T& arg1, U&... __args) {
        set_single_arg(arg1);
        setargs(__args...);
        return 0;
    };

    //! allocate cl_kernel and set cl args
    int allocatesetclarg(int ncu, int nsh) {
        cl_int err;
        int n = ncu * (_shadowsize + 1) + nsh;

        std::cout << "n= " << n << std::endl;
        // create
        mkernels[n] = clCreateKernel(xctx->xprogram, _cuname[ncu].c_str(), &err);
        if (err != CL_SUCCESS) {
            throw xhpp::error("ERROR: xhpp::task::dev_func::allocatesetclarg(), clCreateKernel error.\n");
        };

        // set args
        int argid = 0;
        int nparam = arglist.argsize.size();
        for (int s = 0; s < nparam; s++) {
            size_t ss = (arglist.argsize)[s];
            int nv = s * (_numcu * (_shadowsize + 1)) + n;
            void* vv = (arglist.argvalue)[nv];
            if ((arglist.startendmark)[s] == 0) {
                int err = clSetKernelArg(mkernels[n], argid++, ss, vv);
                if (err != CL_SUCCESS) {
                    throw xhpp::error("ERROR: xhpp::task::dev_func::allocatesetclarg(), clSetKernelArg error.\n");
                };
            } else {
                std::cout << "not setting arg for " << s << std::endl;
                argid++;
            }
        };
        std::cout << "---------" << std::endl;
        return 0;
    };

   public:
    //! constructor
    dev_func(xhpp::context* ctx) : base(ctx){};

    //! deconstructor
    ~dev_func(){};

    //! add CU
    int addcu(std::string cuname, int param_num, int* bank) {
        _numcu += 1;
        _cuname.push_back(cuname);
        db_n = param_num;
        for (int i = 0; i < param_num; i++) {
            _cubank.push_back(bank[i]);
        }
        _cuadded = true;
        return 0;
    };

    //! set parameters
    template <typename... _Args>
    int setparam(_Args&... __args) {
        updateshadowsize(xctx->pattern);
        mkernels.resize(_numcu * (_shadowsize + 1));
        std::cout << "_shadowsize = " << _shadowsize << std::endl;
        int res = setargs(__args...); // set args
        _paramset = true;
        return res;
    };

    // //! setup the kernel
    // template<typename... _Args>
    // int setup(std::string krn_name, _Args & ...  __args){
    //   krnname = krn_name;
    //   int res = setparam(__args...);
    //   return 0;
    // };

    //! setup kernel (body)
    int setupbody() {
        if (_bodyallocated == false) {
            // buffer
            for (int s = 0; s < arglist.devbuffer.size(); s++) {
                (arglist.devbuffer)[s]->bodyallocate(_cubank[0]);
            };
            // kernel obj
            allocatesetclarg(0, 0);
            _bodyallocated = true;
        };
        return 0;
    };

    //! setup kernel (body and shadow)
    int setupbodyshadow(unsigned int _nsize) {
        if (_bodyshadowallocated == false) {
            // buffer
            for (int s = 0; s < arglist.devbuffer.size(); s++) {
                if ((arglist.devbuffer)[s]->startingendingallocate() == false) {
                    (arglist.devbuffer)[s]->bodyshadowallocate(_nsize);
                } else {
                    std::cout << "not allocate buffer for starting or ending vbuffer" << std::endl;
                }
            };
            // kernel obj
            // std::cout <<"_numcu and _shadowsize is " <<_numcu <<"," <<_shadowsize <<std::endl;
            for (int i = 0; i < _numcu; i++) {
                for (int j = 0; j < (_shadowsize + 1); j++) {
                    allocatesetclarg(i, j);
                };
            };
            _bodyshadowallocated = true;
        }
        return 0;
    };

    //! update parameters, scalar
    template <class T>
    int updateparam(const int rcrun, const int nparam, T param) {
        cl_int err = clSetKernelArg(mkernels[rcrun], nparam, sizeof(T), &param);
        if (err != CL_SUCCESS) {
            throw xhpp::error("ERROR: xhpp::task::dev_func::updateparam, clSetKernelArg error.\n");
        };
    }

    //! update parameters, virtual device buffer
    template <class T>
    int updateparam(const int rcrun, const int nparam, xhpp::vbuffer::device<T>*& param) {
        cl_int err = clSetKernelArg(mkernels[rcrun], nparam, sizeof(cl_mem), &(param.xmems[0]));
        if (err != CL_SUCCESS) {
            throw xhpp::error("ERROR: xhpp::task::dev_func::updateparam, clSetKernelArg error.\n");
        };
    };

    //! update parameters, device buffer
    template <class T>
    int updateparam(const int rcrun, const int nparam, xhpp::buffer::device<T>*& param) {
        cl_int err = clSetKernelArg(mkernels[rcrun], nparam, sizeof(cl_mem), &(param->xmems[0]));
        if (err != CL_SUCCESS) {
            throw xhpp::error("ERROR: xhpp::task::dev_func::updateparam, clSetKernelArg error.\n");
        };
    };

    //! update parameters, virtual host buffer error out
    template <class T>
    int updateparam(const int rcrun, const int nparam, xhpp::vbuffer::host<T>*& param) {
        throw xhpp::error("ERROR: xhpp::task::dev_func::updateparam, input should not be virtual host buffer.\n");
    }

    //! update parameters, host buffer error out
    template <class T>
    int updateparam(const int rcrun, const int nparam, xhpp::buffer::host<T>*& param) {
        throw xhpp::error("ERROR: xhpp::task::dev_func::updateparam, input should not be host buffer.\n");
    }

    //! submit a kernel task
    int submit(
        xhpp::event* waitevt, xhpp::event* outevt, const int rcin = 0, const int rcout = 0, const int rcrun = 0) {
        size_t globalsize[] = {1, 1, 1};
        size_t localsize[] = {1, 1, 1};
        cl_int err;
        std::cout << "before kernel" << std::endl;
        err = clEnqueueNDRangeKernel(xctx->xqueue, mkernels[rcrun], 1, NULL, globalsize, localsize, waitevt->size(),
                                     waitevt->data(), outevt->data());
        std::cout << "kernel end" << std::endl;
        if (err != CL_SUCCESS) {
            throw xhpp::error("ERROR: xhpp::task::dev_func::submit(), kernel launch error.\n");
        };
        return 0;
    };

    //! blocking run
    int run(const int rcin = 0, const int rcout = 0, const int rcrun = 0) {
        size_t globalsize[] = {1, 1, 1};
        size_t localsize[] = {1, 1, 1};
        cl_int err;
        err = clEnqueueNDRangeKernel(xctx->xqueue, mkernels[rcrun], 1, NULL, globalsize, localsize, 0, NULL, NULL);
        if (err != CL_SUCCESS) {
            throw xhpp::error("ERROR: xhpp::task::dev_func::run(), kernel launch error.\n");
        };
        xctx->wait();
        return 0;
    };

    //! release task
    int release() {
        _numcu = 0;
        _cuname.resize(0);
        _cuadded = false;
        _paramset = false;
        if (_bodyshadowallocated == true) {
            for (int s = 0; s < arglist.devbuffer.size(); s++) { // buffer
                (arglist.devbuffer)[s]->bodyshadowrelease();
            };

            if (_bodyallocated = true) {
                clReleaseKernel(mkernels[0]);
                _bodyallocated = false;
            }
            for (int i = 1; i < _numcu * (_shadowsize + 1); i++) {
                clReleaseKernel(mkernels[i]);
            };
            _bodyshadowallocated = false;
            _bodyallocated = false;
        } else if (_bodyallocated = true) {
            clReleaseKernel(mkernels[0]);
            for (int s = 0; s < arglist.devbuffer.size(); s++) { // buffer
                (arglist.devbuffer)[s]->bodyrelease();
            };
            _bodyallocated = false;
        }
        return 0;
    };
};

}; // end of namespace task
}; // end of namespace xhpp

#endif
