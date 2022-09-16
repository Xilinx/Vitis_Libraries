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

#ifndef _XHPP_TASKTRANSFER_
#define _XHPP_TASKTRANSFER_

#include <iostream>
#include <string>
#include <functional>
#include <memory>
#include <tuple>

#include "xhpp_event.hpp"
#include "xhpp_context.hpp"
#include "xhpp_bufferhost.hpp"
#include "xhpp_bufferdevice.hpp"
#include "xhpp_taskbase.hpp"
#include "xhpp_enums.hpp"

namespace xhpp {

namespace task {

//! base class to save data of setup()
class tran_impl_base {
   public:
    ~tran_impl_base(){};

    virtual int setupbody() = 0;

    virtual int setupbodyshadow(unsigned int) = 0;

    virtual int releasebody() = 0;
    virtual int releasebodyshadow() = 0;

    //! non-blocking data transfer
    virtual int tran_impl_run(
        const DataTransMode tmode, xhpp::event* waitevt, xhpp::event* outevt, const int rcin, const int rcout) = 0;

    //! blocking data transfer
    virtual int tran_impl_run(const DataTransMode tmode, const int rcin, const int rcout) = 0;

    virtual int updateparam(const int n, void* param) = 0;

    virtual int release() = 0;

    // virtual void watch() = 0;
};

//! tran_impl calss to save data of setparam
template <typename _BUF_H, typename _BUF_D>
class tran_impl : public tran_impl_base {
   private:
    _BUF_H* hbptr;
    _BUF_D* dbptr;

    //! execute non-blocking data transfer
    void tran_impl_func(_BUF_H* hbuf,
                        _BUF_D* dbuf,
                        const DataTransMode tmode,
                        xhpp::event* waitevt,
                        xhpp::event* outevt,
                        const int rcin,
                        const int rcout) {
        if (tmode == host2dev) {
            dbuf->copy_from_host(hbuf, waitevt, outevt, rcin, rcout);
        } else if (tmode == dev2host) {
            dbuf->copy_to_host(hbuf, waitevt, outevt, rcin, rcout);
        };
    };

    //! execute blocking data transfer
    void tran_impl_func(_BUF_H* hbuf, _BUF_D* dbuf, const DataTransMode tmode, const int rcin, const int rcout) {
        if (tmode == host2dev) {
            dbuf->copy_from_host(hbuf, rcin, rcout);
        } else if (tmode == dev2host) {
            dbuf->copy_to_host(hbuf, rcout, rcin);
        };
    };

   public:
    //! constructor
    tran_impl(_BUF_H* _hbuf, _BUF_D* _dbuf) : hbptr(_hbuf), dbptr(_dbuf){};

    //! deconstructor
    ~tran_impl(){};

    //! setup/allocate (body) buffer
    inline int setupbody() {
        hbptr->bodyallocate();
        dbptr->bodyallocate();
        return 0;
    };

    //! setup/allocate (shadow) buffer
    inline int setupbodyshadow(unsigned int nsize) {
        hbptr->bodyshadowallocate(nsize);
        dbptr->bodyshadowallocate(nsize);
        return 0;
    };

    //! release (body) buffer
    inline int releasebody() {
        hbptr->bodyrelease();
        dbptr->bodyrelease();
        return 0;
    };

    //! setup/allocate (body and shadow) buffer
    inline int releasebodyshadow() {
        hbptr->bodyshadowrelease();
        dbptr->bodyshadowrelease();
        return 0;
    };

    //! execute non-blocking data transfer
    inline int tran_impl_run(
        const DataTransMode tmode, xhpp::event* waitevt, xhpp::event* outevt, const int rcin, const int rcout) {
        tran_impl_func(hbptr, dbptr, tmode, waitevt, outevt, rcin, rcout);
    };

    //! execute blocking data transfer
    inline int tran_impl_run(const DataTransMode tmode, const int rcin, const int rcout) {
        tran_impl_func(hbptr, dbptr, tmode, rcin, rcout);
    };

    //! update parameters
    inline int updateparam(const int n, void* buf) {
        if (n == 0) {
            hbptr = (_BUF_H*)buf;
        } else if (n == 1) {
            dbptr = (_BUF_D*)buf;
        } else {
            throw xhpp::error("ERROR: xhpp::task::data_transfer::updateparam(), first parameter is out of range.\n");
        };
    };

    //! release
    int release(){};

}; // end of class

//! data transfer task class
class data_transfer : public base {
   private:
    DataTransMode tmode; //! mode
    tran_impl_base* tranptr;

    //! setup data transfer task
    template <typename BUF_H, typename BUF_D>
    inline int setup(BUF_H* hb, BUF_D* db) {
        tranptr = new tran_impl<BUF_H, BUF_D>(hb, db);
        return 0;
    };

   public:
    //! constructor
    data_transfer(xhpp::context* ctx, DataTransMode mode) : base(ctx) { tmode = mode; };

    //! deconstructor
    ~data_transfer(){};

    //! number of CUs
    int _numcu = 1;
    int numofcu() { return _numcu; }

    // TODO: event/dep is not added

    //! set parameters of transfer task
    template <typename BUF_H, typename BUF_D>
    inline int setparam(BUF_H* hb, BUF_D* db) {
        return setup(hb, db);
    };

    //! update parameters, error out scalar
    template <class T>
    int updateparam(const int nparam, T& param, const int rcrun) {
        throw xhpp::error("ERROR: xhpp::task::data_transfer::updateparam, input should not be scalar.\n");
        return 1;
    }

    //! update virtual host buffer parameters
    // For data transfer, the process is hb==>db[0]/db[1].
    // updateparam() only responds for update the param for the user side. aka. replace hb by in[i].
    // No hb[0] hb[1] is required. And for the replacement, no idxin/idxout/idxrun is required.
    // The db[0]/db[1]/db[mcu] index is updated in the node submit part.
    // PS: in reality, only host buffer are updated by the user.
    template <class T>
    int updateparam(const int rcrun, const int nparam, xhpp::vbuffer::host<T>* param) {
        return tranptr->updateparam(0, param);
    };

    //! update host buffer parameters
    template <class T>
    int updateparam(const int rcrun, const int nparam, xhpp::buffer::host<T>* param) {
        return tranptr->updateparam(0, param);
    };

    //! update virtual device buffer parameters
    template <class T>
    int updateparam(const int rcrun, const int nparam, xhpp::vbuffer::device<T>* param) {
        return tranptr->updateparam(1, param);
    };

    //! update device buffer parameters
    template <class T>
    int updateparam(const int rcrun, const int nparam, xhpp::buffer::device<T>* param) {
        return tranptr->updateparam(1, param);
    };

    //! setup body
    int setupbody() { return tranptr->setupbody(); };

    //! setup body and shadow
    int setupbodyshadow(unsigned int nsize) { return tranptr->setupbodyshadow(nsize); };

    //! release body
    int releasebody() { return tranptr->releasebody(); };

    //! release body and shadow
    int releasebodyshadow() { return tranptr->releasebodyshadow(); };

    //! submit non-blocking data transfer task
    int submit(
        xhpp::event* waitevt, xhpp::event* outevt, const int rcin = 0, const int rcout = 0, const int rcrun = 0) {
        // std::cout <<"data transfer using rcin, rcout "<<rcin<<", " <<rcout <<std::endl;
        return tranptr->tran_impl_run(tmode, waitevt, outevt, rcin, rcout); // TODO: check rcin/rcout with rcfrom/rcto
    };

    //! run blocking data transfer task
    int run(const int rcin = 0, const int rcout = 0, const int rcrun = 0) { // TODO: check rcin/rcout with rcfrom/rcto
        return tranptr->tran_impl_run(tmode, rcin, rcout);
    };

    //! release task
    int release() {
        tranptr->release();
        free(tranptr);
        return 0;
    };
}; // end of class

}; // end of namespace task
}; // end of namespace xhpp

#endif
