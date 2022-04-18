/*
 * Copyright 2020 Xilinx, Inc.
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

#pragma once

#ifndef _XF_GRAPH_L3_TASK_HPP_
#define _XF_GRAPH_L3_TASK_HPP_

#define DEBUG_WORKER true

#include <chrono>
#include <cstring>
#include <future>
#include <iomanip>
#include <iostream>
#include <queue>

#include <assert.h>
#include <bitset>
#include <sstream>
#include <xrm.h>

#include <thread>
#include <future>
#include <unistd.h>
#include <functional>

#include <openclHandle.hpp>

namespace xf {
namespace graph {
namespace L3 {

class openXRM {
   public:
    // xrmCuGroupResource** resR;
    char** udfCuGroupName;
    openXRM() {
        ctx = (xrmContext*)xrmCreateContext(XRM_API_VERSION_1);
        assert(!(ctx == NULL));
    };

    void freeCuGroup(unsigned int deviceNm) {
        int ret = 0;
        for (int i = 0; i < 1; ++i) {
            ret = xrmUdfCuGroupUndeclare(ctx, udfCuGroupName[i]);
            if (ret == XRM_SUCCESS) {
                printf("INFO: User defined cu group from same device undeclaration success\n");
            } else {
                printf("ERROR: User defined cu group from same device undeclaration fail\n");
                exit(1);
            }
            delete[] udfCuGroupName[i];
        }
        delete[] udfCuGroupName;
    }
    void setUpCuGroup(unsigned int deviceNm,
                      unsigned int cuNm,
                      std::string kernelName,
                      std::string kernelAlias,
                      unsigned int requestLoad) {
        udfCuGroupName = new char*[1];
        xrmUdfCuGroupProperty* udfCuGroupProp[1];
        std::string baseName = "udfCuGroupSameDevice";
        xrmUdfCuListProperty* udfCuListProp;
        xrmUdfCuProperty* udfCuProp;
        udfCuGroupName[0] = new char[XRM_MAX_NAME_LEN];
        udfCuGroupProp[0] = (xrmUdfCuGroupProperty*)malloc(sizeof(xrmUdfCuGroupProperty));
        memset(udfCuGroupProp[0], 0, sizeof(xrmUdfCuGroupProperty));
        strcpy(udfCuGroupName[0], baseName.c_str());
        udfCuGroupProp[0]->optionUdfCuListNum = 1;
        udfCuListProp = &udfCuGroupProp[0]->optionUdfCuListProps[0];
        udfCuListProp->cuNum = cuNm * deviceNm;
        udfCuListProp->sameDevice = false;
        for (int32_t cuIdx = 0; cuIdx < udfCuListProp->cuNum; cuIdx++) {
            std::string cuName0 = kernelName + ":" + kernelName + "_" + std::to_string(cuIdx);
            udfCuProp = &udfCuListProp->udfCuProps[cuIdx];
            strcpy(udfCuProp->cuName, cuName0.c_str());
            udfCuProp->devExcl = false;
            udfCuProp->requestLoad = requestLoad;
        }
        int ret = xrmUdfCuGroupDeclare(ctx, udfCuGroupProp[0], udfCuGroupName[0]);
        if (ret == XRM_SUCCESS) {
            printf("INFO: User defined cu group from same device undeclaration success\n");
        } else {
            printf("ERROR: User defined cu group from same device undeclaration fail\n");
            exit(1);
        }
        free(udfCuGroupProp[0]);
    }

    void unloadXclbin(unsigned int deviceId) { xrmUnloadOneDevice(ctx, deviceId); }
    void loadXclbin(unsigned int deviceId, std::string& xclbinName) {
        unloadXclbin(deviceId);
        xrmLoadOneDevice(ctx, deviceId, (char*)xclbinName.c_str());
    }
    std::thread unloadXclbinNonBlock(unsigned int deviceId) { return std::thread(xrmUnloadOneDevice, ctx, deviceId); }
    std::thread loadXclbinNonBlock(unsigned int deviceId, std::string& xclbinName) {
        return std::thread(xrmLoadOneDevice, ctx, deviceId, (char*)xclbinName.c_str());
    }
    std::future<int> loadXclbinAsync(unsigned int deviceId, std::string& xclbinPath) {
        std::cout << "DEBUG: " << __FUNCTION__ << " xclbinPath=" << xclbinPath << std::endl;
        std::future<int> ret = std::async(&xrmLoadOneDevice, ctx, deviceId, (char*)xclbinPath.c_str());

        return ret;
    }

    uint32_t allocCU(xrmCuResource* resR, const char* kernelName, const char* kernelAlias, int requestLoad) {
        xrmCuProperty propR;
        memset(&propR, 0, sizeof(xrmCuProperty));
        strcpy(propR.kernelName, kernelName);
        strcpy(propR.kernelAlias, kernelAlias);
        propR.devExcl = false;
        propR.requestLoad = requestLoad;
        propR.poolId = 0;

        uint32_t ret = xrmCuAlloc(ctx, &propR, resR);

        if (ret != 0) {
            printf("INFO: All CUs are busy~\n");
        };
        return ret;
    }

    void allocGroupCU(xrmCuGroupResource* resR, std::string groupName) {
        xrmCuGroupProperty cuGroupProp;
        memset(&cuGroupProp, 0, sizeof(xrmCuGroupProperty));
        strcpy(cuGroupProp.udfCuGroupName, groupName.c_str());
        cuGroupProp.poolId = 0;

        xrmCuGroupAlloc(ctx, &cuGroupProp, resR);
    }

    xrmCuListResource allocMultiCU(
        const char* kernelName, const char* kernelAlias, int requestLoad, int deviceNumber, int cuNumber) {
        xrmCuListProperty CuListProp;
        xrmCuListResource CuListRes;

        memset(&CuListProp, 0, sizeof(xrmCuListProperty));
        memset(&CuListRes, 0, sizeof(xrmCuListResource));

        std::cout << "request cu number = " << cuNumber << std::endl;
        CuListProp.cuNum = cuNumber * deviceNumber;
        for (int i = 0; i < CuListProp.cuNum; ++i) {
            strcpy(CuListProp.cuProps[i].kernelName, kernelName);
            strcpy(CuListProp.cuProps[i].kernelAlias, kernelAlias);
            CuListProp.cuProps[i].devExcl = false;
            CuListProp.cuProps[i].requestLoad = requestLoad;
            CuListProp.cuProps[i].poolId = 0;
        }
        uint32_t ret = xrmCuListAlloc(ctx, &CuListProp, &CuListRes);
        if (ret != 0) {
            printf("Error: Fail to alloc cu (xrmCuListBlockingAlloc) \n");
            memset(&CuListRes, 0, sizeof(xrmCuListResource));
        };

        return CuListRes;
    }

    int32_t fetchCuInfo(const char* kernelName,
                        const char* kernelAlias,
                        int requestLoad,
                        unsigned int& numDevices,
                        unsigned int& cuPerBoard,
                        uint64_t& maxChannelSize,
                        unsigned int& maxCU,
                        unsigned int** deviceID,
                        unsigned int** cuID) {
        xrmCuProperty propR;
        memset(&propR, 0, sizeof(xrmCuProperty));
        strcpy(propR.kernelName, kernelName);
        // strcpy(propR.kernelAlias, kernelAlias);
        propR.devExcl = false;
        propR.requestLoad = requestLoad;
        propR.poolId = 0;
        unsigned int availMaxCU = xrmCheckCuAvailableNum(ctx, &propR);
        xrmCuListProperty cuListPropR;
        xrmCuListResource cuListResR;
        cuListPropR.cuNum = availMaxCU;
        unsigned int* devices;
        unsigned int* cus;

        for (int i = 0; i < cuListPropR.cuNum; i++) {
            strcpy(cuListPropR.cuProps[i].kernelName, kernelName);
            // strcpy(cuListPropR.cuProps[i].kernelAlias, kernelAlias);
            cuListPropR.cuProps[i].devExcl = false;
            cuListPropR.cuProps[i].requestLoad = requestLoad;
            cuListPropR.cuProps[i].poolId = 0;
        }
        int32_t alloc0 = xrmCuListAlloc(ctx, &cuListPropR, &cuListResR);

        uint32_t availNumDevices = 0;
#if (SW_EMU_TEST || HW_EMU_TEST)
        maxCU = numDevices * cuPerBoard;
        devices = new unsigned int[maxCU];
        cus = new unsigned int[maxCU];
        for (int i = 0; i < maxCU; i++) {
            cus[i] = i % cuPerBoard;
            ;
            devices[i] = i / cuPerBoard;
        }
        *cuID = cus;
        *deviceID = devices;
#else
        if (alloc0 != 0) {
            std::cout << "ERROR: " << __FUNCTION__ << "Fail to alloc cu list (xrmCuListAlloc): alloc0=" << alloc0
                      << std::endl;
            return alloc0;
        } else {
            devices = new unsigned int[availMaxCU];
            cus = new unsigned int[availMaxCU];
            memBankSizeTransfer(cuListResR.cuResources[0].membankSize, maxChannelSize);
            for (int i = 0; i < cuListResR.cuNum; i++) {
#ifndef NDEBUG
                printf("INFO: Allocated cu list: cu %d\n", i);
                printf("   xclbinFileName is:  %s\n", cuListResR.cuResources[i].xclbinFileName);
                printf("   kernelPluginFileName is:  %s\n", cuListResR.cuResources[i].kernelPluginFileName);
                printf("   kernelName is:  %s\n", cuListResR.cuResources[i].kernelName);
                printf("   kernelAlias is:  %s\n", cuListResR.cuResources[i].kernelAlias);
                printf("   instanceName is:  %s\n", cuListResR.cuResources[i].instanceName);
                printf("   cuName is:  %s\n", cuListResR.cuResources[i].cuName);
                printf("   deviceId is:  %d\n", cuListResR.cuResources[i].deviceId);
                printf("   cuId is:  %d\n", cuListResR.cuResources[i].cuId);
                printf("   channelId is:  %d\n", cuListResR.cuResources[i].channelId);
                printf("   cuType is:  %d\n", cuListResR.cuResources[i].cuType);
                printf("   baseAddr is:  0x%lx\n", cuListResR.cuResources[i].baseAddr);
                printf("   membankId is:  %d\n", cuListResR.cuResources[i].membankId);
                printf("   membankType is:  %d\n", cuListResR.cuResources[i].membankType);
                printf("   membankSize is:  %ld Byte\n", maxChannelSize);
                printf("   membankBaseAddr is:  0x%lx\n", cuListResR.cuResources[i].membankBaseAddr);
                printf("   allocServiceId is:  %lu\n", cuListResR.cuResources[i].allocServiceId);
                printf("   poolId is:  %lu\n", cuListResR.cuResources[i].poolId);
                printf("   channelLoad is:  %d\n", cuListResR.cuResources[i].channelLoad);
#endif
                cus[i] = cuListResR.cuResources[i].cuId;
                devices[i] = cuListResR.cuResources[i].deviceId;
                bool flag = false;
                for (int j = 0; j < i; ++j) {
                    if (devices[j] == (unsigned int)cuListResR.cuResources[i].deviceId) {
                        flag = true;
                    }
                }
                if (flag == false) {
                    availNumDevices += 1;
                }
            }
            *cuID = cus;
            *deviceID = devices;
            assert(availNumDevices != 0);
            if (xrmCuListRelease(ctx, &cuListResR))
                std::cout << "INFO: Success to release cu list\n" << std::endl;
            else
                std::cout << "Error: Fail to release cu list\n" << std::endl;

            std::cout << "INFO: Total device number = " << numDevices << std::endl;
            std::cout << "INFO: Available device number = " << availNumDevices << std::endl;
            std::cout << "INFO: Available CU number = " << availMaxCU << std::endl;
            if (availNumDevices >= numDevices) {
                // has sufficient devices. adjust maxCU based on requested numDevices
                maxCU = numDevices * availMaxCU / availNumDevices;
            }
        }
#endif
        return 0;
    }

    void freeXRM() {
        if (xrmDestroyContext(ctx) != XRM_SUCCESS)
            std::cout << "INFO: Destroy context failed\n" << std::endl;
        else
            std::cout << "INFO: Destroy context success\n" << std::endl;
    };

    xrmContext* ctx;

   private:
    void memBankSizeTransfer(uint64_t input, uint64_t& output) {
        std::stringstream str;
        std::string pre = std::bitset<64>(input).to_string();
        str << pre;
        str >> std::hex >> output;
    };
};

/**
 * Type erased std::packaged_task<RT()>
 *
 * Wraps a std::packaged_task of any return type, such that the task's
 * return value can be captured in a future.
 *
 * Objects of this task class can be stored in any STL container even
 * when the underlying std::packaged_tasks are of different types.
 */
class task {
    struct task_iholder {
        virtual ~task_iholder(){};
        virtual void execute(unsigned int ID,
                             unsigned int ID2,
                             unsigned int ID3,
                             class openXRM* xrm,
                             xrmCuResource* resR,
                             std::string instance) = 0;
    };

    template <typename Callable>
    struct task_holder : public task_iholder {
        Callable held;
        task_holder(Callable&& t) : held(std::move(t)) {}
        void execute(unsigned int ID,
                     unsigned int ID2,
                     unsigned int ID3,
                     class openXRM* xrm,
                     xrmCuResource* resR,
                     std::string instance) {
            std::thread w1(std::move(held), ID, ID2, ID3, xrm->ctx, resR, instance);
            w1.detach();
        }
    };

    std::unique_ptr<task_iholder> content;

   public:
    task() : content(nullptr) {}

    task(task&& rhs) : content(std::move(rhs.content)) {}

    template <typename Callable>
    task(Callable&& c) : content(new task_holder<Callable>(std::forward<Callable>(c))) {}

    task& operator=(task&& rhs) {
        content = std::move(rhs.content);
        return *this;
    }

    bool valid() const { return content != nullptr; }

    void execute(unsigned int ID,
                 unsigned int ID2,
                 unsigned int ID3,
                 class openXRM* xrm,
                 xrmCuResource* resR,
                 std::string instance) {
        content->execute(ID, ID2, ID3, xrm, resR, instance);
    }
};

/**
 * Multiple producer / multiple consumer queue of task objects
 *
 * This code is not specifically tied to task::task, but we keep
 * the defintion here to make task.h stand-alone
 */
template <typename Task>
class mpmcqueue {
    std::queue<Task> m_tasks;
    mutable std::mutex m_mutex;
    std::condition_variable m_work;
    bool m_stop = false;
    unsigned long tp = 0;       // time point when last task consumed
    unsigned long waittime = 0; // wait time from tp to next task avail
    bool debug = false;

   public:
    mpmcqueue() {}

    explicit mpmcqueue(bool dbg) : debug(dbg) {}

    void addWork(Task&& t) {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_tasks.push(std::move(t));
        m_work.notify_one();
    }
    bool empty() { return m_tasks.empty(); }

    Task getWork() {
        std::unique_lock<std::mutex> lk(m_mutex);
        while (!m_stop && m_tasks.empty()) {
            m_work.wait(lk);
        }

        Task task;
        if (!m_stop) {
            task = std::move(m_tasks.front());
            m_tasks.pop();
        }
        return task;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lk(m_mutex);
        return m_tasks.size();
    }

    void stop() {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_stop = true;
        m_work.notify_all();
    }
};

using queue = mpmcqueue<task>;

/**
 * event class wraps std::future<RT>
 *
 * Adds a ready() function that can be used to poll if event is ready.
 * Otherwise, currently adds no value compared to bare std::future
 */
template <typename RT>
class event {
   public:
    typedef RT value_type;
    typedef std::future<value_type> FutureType;

   private:
    mutable FutureType m_future;

   public:
    event() = delete;
    event(const event& rhs) = delete;

    event(const event&& rhs) : m_future(std::move(rhs.m_future)) {}

    event(FutureType&& f) : m_future(std::forward<FutureType>(f)) {}

    event& operator=(event&& rhs) {
        m_future = std::move(rhs.m_future);
        return *this;
    }

    RT wait() const { return m_future.get(); }

    RT get() const { return m_future.get(); }

    bool ready() const { return (m_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready); }
};

/**
 * Functions for adding work (tasks) to a task queue.
 *
 * All functions return a task::event that encapsulates the
 * return type of the task.
 *
 * Variants of the functions supports adding both free functions
 * and member functions associated with some class object.
 */
// Free function, lambda, functor

template <typename Q, typename F, typename... Args>
auto createL3(Q& q, F&& f, Args&&... args) -> event<int> {
    typedef std::packaged_task<int(int, int, int, xrmContext*, xrmCuResource*, std::string)> task_type;
    task_type t(std::bind(std::forward<F>(f), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                          std::placeholders::_4, std::placeholders::_5, std::placeholders::_6,
                          std::forward<Args>(args)...));
    std::cout << "INFO: Creating L3" << std::endl;
    event<int> e(t.get_future());
    q.addWork(std::move(t));
    return e;
}

inline void dynamicWorker(queue& q,
                          class openXRM* xrm,
                          clHandle* handles,
                          std::string kernelName,
                          std::string kernelAlias,
                          unsigned int requestLoad,
                          unsigned int numDevices,
                          unsigned int cuNm) {
    int hwNm = numDevices * cuNm;
    int resNm = hwNm > 16 ? hwNm : 16;
    class task t[resNm];

    unsigned int pendingRequests = 0;
    unsigned int curRequestId = 0;
    unsigned int deviceId = 0;
    unsigned int cuId = 0;
    unsigned int handleId = 0;
    bool toStop = false;

    while (true) {
        pendingRequests = 0;
        while (!q.empty()) {
            // q.getWork is blocking until it has a job
            t[pendingRequests] = q.getWork();
            std::cout << "INFO: add taskID=" << pendingRequests << std::endl;
            pendingRequests++;
            if (pendingRequests >= resNm) break; // no more requests because of hardware limit, execute pending ones.
        }

        toStop = false;
        for (int i = 0; i < pendingRequests; ++i) {
            if (!t[i].valid()) {
                toStop = true;
                std::cout << "Worker stop!" << std::endl;
            }
        }
        if (toStop) break;

        while (pendingRequests > 0) {
            while (handles[handleId].isBusy) {
                handleId++;
                handleId = handleId % hwNm;
                // wait for free cu
            }

#ifdef DEBUG_WORKER
            std::cout << "DEBUG: " << __FUNCTION__ << " pendingRequests=" << pendingRequests
                      << " curRequestId=" << curRequestId << std::endl;
#endif

            cuId = handles[handleId].cuID;
            deviceId = handles[handleId].deviceID;
            if (!handles[handleId].isBusy) {
                // cu alloc success, excute op
                pendingRequests--;
#ifdef DEBUG_WORKER
                std::cout << "DEBUG: " << __FUNCTION__ << " taskID=" << pendingRequests << " handleId=" << handleId
                          << " deviceId=" << deviceId << " cuId=" << cuId << std::endl;
#endif
                handles[handleId].isBusy = true;
                t[pendingRequests].execute(deviceId, cuId, handles[handleId].resR->channelId, xrm,
                                           handles[handleId].resR, handles[handleId].resR->instanceName);
                curRequestId++;
            }
        }
    }
}

inline void staticWorker(queue& q,
                         class openXRM* xrm,
                         clHandle* handles,
                         std::string kernelName,
                         std::string kernelAlias,
                         unsigned int requestLoad,
                         unsigned int numDevices,
                         unsigned int cuNm) {
    int hwNm = numDevices * cuNm;
    xrmCuResource* resR[hwNm];
    int pendingRequests;
    int curRequestId = 0;

    while (true) {
        class task t[hwNm];
        pendingRequests = 0;
        for (int i = 0; i < hwNm; ++i) {
            // q.getWork is blocking until it has a job
            t[i] = q.getWork();

            pendingRequests++;
            if (q.empty()) break; // no more requests execute pending ones.
        }

        bool toStop = false;
        for (int i = 0; i < pendingRequests; ++i) {
            if (!t[i].valid()) toStop = true;
        }
        if (toStop) break;

        for (int i = 0; i < pendingRequests; i++) {
            unsigned int deviceID = handles[curRequestId].deviceID;
            unsigned int cuID = handles[curRequestId].cuID;
            unsigned int channelID = 0;

            std::cout << "DEBUG: " << __FUNCTION__ << " pendingRequests" << pendingRequests << " curRequestId "
                      << curRequestId << " handle.busy=" << handles[curRequestId].isBusy << std::endl;

            while (handles[curRequestId].isBusy) {
                sleep(0.001); // wait 1ms for cu free
            }

            handles[curRequestId].isBusy = true;
            t[i].execute(deviceID, cuID, channelID, xrm, handles[curRequestId].resR,
                         handles[curRequestId].resR->instanceName);
            curRequestId++;
            if (curRequestId == hwNm) curRequestId = 0;
        }
    }
}

} // L3
} // graph
} // xf

#endif
