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

#ifndef _XHPP_TASK_
#define _XHPP_TASK_
#include <iostream>
#include <functional>
#include <memory>
#include <tuple>
#include "taskbase.hpp"
namespace xhpp {
namespace task {
//! base class of task_impl class
class task_impl_base {
   public:
    ~task_impl_base(){};
    virtual void task_impl_run() = 0;
};

//! task_impl class, which is used to achieve running the loaded host tasks.
template <typename _Callable>
class task_impl : public task_impl_base {
   public:
    _Callable task_impl_func;
    task_impl(_Callable&& __f) : task_impl_func(std::forward<_Callable>(__f)){};
    void task_impl_run() { task_impl_func(); }
};
class host_func : public base {
   private:
    typedef std::shared_ptr<task_impl_base> task_shared_ptr;
    task_shared_ptr tptr;
    cl_int err;

    //! task pointer creation
    template <typename _Callable>
    std::shared_ptr<task_impl<_Callable> > taskptr(_Callable&& __f) {
        return std::make_shared<task_impl<_Callable> >(std::forward<_Callable>(__f));
    }
    cl_event evt_new;

   public:
    std::string type = "host_task";

    //! number of CUs
    int _numcu = 1;
    int numofcu() { return _numcu; }

    /*      unsigned int _shadowsize = 0;
          //! update _shadowsize for kernel task
          int updateshadowsize(const Pattern pattern){
    //TODO
              return _shadowsize;
          }
          */

    //! constructor
    host_func(xhpp::context* ctx) : base(ctx){};
    //! assign/copy host task tsk1==> tsk2
    //...
    //! load host function to host_task
    template <typename _Callable, typename... _Args>
    int setup(_Callable&& __f, _Args&&... __args) {
        this->tptr = taskptr(std::__bind_simple(std::forward<_Callable>(__f), std::forward<_Args>(__args)...));
        std::cout << "assinged new host_task" << std::endl;
        return 0;
    }
    int setarg() {}
    //! lauch the task
    int run() {
        std::cout << std::endl << "------running the callback task-------" << std::endl;
        tptr->task_impl_run();
        cl_int err = clSetUserEventStatus(evt_new, CL_COMPLETE);
        if (err != CL_SUCCESS) {
            std::cout << "set user event failed" << std::endl;
        }
        return 0;
    }
    //! release
    int release() {
        std::cout << "releasing the task" << std::endl;
        return 0;
    }
    ~host_func() {}

    static void callback_run(cl_event event, cl_int status, void* data) {
        auto self = reinterpret_cast<host_func*>(data);
        self->run();
    }

    int setupshadow(unsigned int){};

    /*      cl_event submit_e (xhpp::context* ctx, cl_event event){
            cl_int err = clSetEventCallback(event, CL_COMPLETE, callback_run, this);
            if(err == CL_SUCCESS){
              std::cout << "task submitted successfully ..." << std::endl;
            }
            evt_new = clCreateUserEvent(ctx->xcontext, &err);
            return evt_new;
            }
    */
    int submit(xhpp::context* ctx, xhpp::event* waitevt, xhpp::event* evt, const int rcin, const int rcout) {
        evt_new = clCreateUserEvent(ctx->xcontext, &err);
        if (err != CL_SUCCESS) {
            std::cout << "failed to create host task event" << std::endl;
        }
        evt->data()[0] = evt_new;
        err = clSetEventCallback(waitevt->data()[0], CL_COMPLETE, callback_run, this);
        if (err == CL_SUCCESS) {
            std::cout << "task submitted successfully ..." << std::endl;
            return 0;
        } else {
            return 1;
        }
    };
};

} // task
} // xhpp

#endif
