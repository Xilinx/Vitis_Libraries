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

#ifndef GQE_CFG_H
#define GQE_CFG_H
#include "ap_int.h"
#include "xf_database/dynamic_alu_host.hpp"
#include "xf_database/enums.hpp"
#include <fstream>
#endif
#ifndef _GQE_API_
#define _GQE_API_

#include <CL/cl_ext_xilinx.h>
#include <xcl2.hpp>

#define XCL_BANK(n) (((unsigned int)(n)) | XCL_MEM_TOPOLOGY)

#define XCL_BANK0 XCL_BANK(0)
#define XCL_BANK1 XCL_BANK(1)
#define XCL_BANK2 XCL_BANK(2)
#define XCL_BANK3 XCL_BANK(3)
#define XCL_BANK4 XCL_BANK(4)
#define XCL_BANK5 XCL_BANK(5)
#define XCL_BANK6 XCL_BANK(6)
#define XCL_BANK7 XCL_BANK(7)
#define XCL_BANK8 XCL_BANK(8)
#define XCL_BANK9 XCL_BANK(9)
#define XCL_BANK10 XCL_BANK(10)
#define XCL_BANK11 XCL_BANK(11)
#define XCL_BANK12 XCL_BANK(12)
#define XCL_BANK13 XCL_BANK(13)
#define XCL_BANK14 XCL_BANK(14)
#define XCL_BANK15 XCL_BANK(15)

long getkrltime(cl::Event e1, cl::Event e2) {
    cl_ulong start, end;
    e1.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
    e2.getProfilingInfo(CL_PROFILING_COMMAND_START, &end);
    long kerneltime = (end - start) / 1000000;
    return kerneltime;
}
long getkrltime(cl::Event e1_0, cl::Event e1_1, cl::Event e2) {
    cl_ulong start_0, start_1, start, end;
    e1_0.getProfilingInfo(CL_PROFILING_COMMAND_START, &start_0);
    e1_1.getProfilingInfo(CL_PROFILING_COMMAND_START, &start_1);
    e2.getProfilingInfo(CL_PROFILING_COMMAND_START, &end);
    start = start_0 > start_1 ? start_1 : start_0;
    long kerneltime = (end - start) / 1000000;
    return kerneltime;
}
long getkrltime(std::vector<cl::Event> e1, std::vector<cl::Event> e2) {
    cl_ulong start, end;
    e1[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
    e2[0].getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
    for (unsigned int i = 1; i < e1.size(); i++) {
        cl_ulong start1;
        e1[i].getProfilingInfo(CL_PROFILING_COMMAND_START, &start1);
        if (start1 < start) start = start1;
    }
    for (unsigned int i = 1; i < e2.size(); i++) {
        cl_ulong end1;
        e2[i].getProfilingInfo(CL_PROFILING_COMMAND_END, &end1);
        if (end1 > end) end = end1;
    }
    long kerneltime = (end - start) / 1000000;
    return kerneltime;
}
void printkrlTime(cl::Event e1, cl::Event e2, const std::string kinfo = "kernel") {
    std::cout << std::dec << kinfo << " execution time " << getkrltime(e1, e2) << " ms" << std::endl;
}
void printkrlTime(cl::Event e1, cl::Event e2, cl::Event e3, const std::string kinfo = "kernel") {
    std::cout << std::dec << kinfo << " execution time " << getkrltime(e1, e2, e3) << " ms" << std::endl;
}
void printkrlTime(std::vector<cl::Event> e1, std::vector<cl::Event> e2, std::string kinfo = "kernel") {
    std::cout << std::dec << kinfo << " execution time " << getkrltime(e1, e2) << " ms" << std::endl;
}
int64_t gettimestamp() {
    //  std::chrono::steady_clock::duration d = std::chrono::steady_clock::now().time_since_epoch();
    //  std::chrono::microseconds mic = std::chrono::duration_cast<std::chrono::microseconds>(d);
    //  return mic.count();
    std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tp =
        std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    auto tmp = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
    int64_t timestamp = tmp.count();
    return timestamp;
}
void print_h_time(struct timeval tv_r_s, struct timeval tv_r_0, struct timeval tv_r_1, const std::string hinfo) {
    std::cout << hinfo << " start time of Host " << tvdiff(&tv_r_s, &tv_r_0) / 1000 << " ms" << std::endl;
    std::cout << hinfo << " end time of Host " << tvdiff(&tv_r_s, &tv_r_1) / 1000 << " ms" << std::endl;
    std::cout << hinfo << " duration time of Host " << tvdiff(&tv_r_0, &tv_r_1) / 1000 << " ms" << std::endl;
}
void print_d_time(cl::Event s, cl::Event e, int64_t base, const std::string kinfo = "kernel", int64_t offset = 0) {
    cl_ulong start, end;
    s.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
    e.getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
    long stime = (start - base) / 1000000 + offset;
    long etime = (end - base) / 1000000 + offset;
    long duration = (end - start) / 1000000;
    std::cout << std::dec << kinfo << " start time of Device " << stime << " ms" << std::endl;
    std::cout << std::dec << kinfo << " end time of Device " << etime << " ms" << std::endl;
    std::cout << std::dec << kinfo << " duration time of Device " << duration << " ms" << std::endl;
}
template <typename T>
int load_dat(T* data, const std::string& name, const std::string& dir, size_t n, size_t sizeT) {
    if (!data) {
        return -1;
    }

    std::string fn = dir + "/" + name + ".dat";
    FILE* f = fopen(fn.c_str(), "rb");
    if (!f) {
        std::cerr << "ERROR: " << fn << " cannot be opened for binary read." << std::endl;
    }
    // size_t cnt = fread(data, sizeof(T), n, f);
    size_t cnt = fread((void*)data, sizeT, n, f);
    fclose(f);
    if (cnt != n) {
        std::cerr << "ERROR: " << cnt << " entries read from " << fn << ", " << n << " entries required." << std::endl;
        return -1;
    }
    return 0;
}

template <typename T>

void load_dat(T* data, const std::string& name, const std::string& dir, size_t n) {
    load_dat(data, name, dir, n, sizeof(T));
};

class Table {
   public:
    std::string name;
    size_t nrow;
    size_t ncol;
    int npart;
    std::vector<std::string> colsname;
    std::vector<size_t> colswidth;
    std::vector<size_t> isrowid;
    std::vector<size_t> iskdata;

    std::string dir;

    std::vector<size_t> size512;
    cl_mem_ext_ptr_t mext;

    ap_uint<512>* data;
    ap_uint<512>* datak;
    cl::Buffer buffer;

    Table(){};

    Table(std::string name_, size_t nrow_, size_t ncol_, std::string dir_) {
        name = name_;
        nrow = nrow_;
        ncol = ncol_;
        npart = 1;
        dir = dir_;
        size512.push_back(0);
        mode = 2;
    };

    Table(size_t size) {
        size512.push_back(size / 64);
        mode = 3;
    };

    //! Add column
    void addCol(std::string columname, size_t width, int isrid = 0, int iskda = 1) {
        size_t depth = nrow + VEC_LEN * 2 - 1;
        size_t sizeonecol = size_t((width * depth + 64 - 1) / 64);
        size512.push_back(size512.back() + sizeonecol);
        colsname.push_back(columname);
        isrowid.push_back(isrid);
        iskdata.push_back(iskda);
        colswidth.push_back(width);
        mode = 1;
    };

    //! Load table to CPU memory
    void loadHost() {
        // std::cout<<"load host"<<std::endl;
        for (size_t i = 0; i < ncol; i++) {
            // std::cout<<isrowid[i]<<std::endl;
            if (isrowid[i] == 0) {
                // std::cout<<colsname[i]<<dir<<nrow<<colswidth[i]<<std::endl;
                int err = load_dat(data + size512[i] + 1, colsname[i], dir, nrow, colswidth[i]);
                if (err) {
                    std::cout << "ERROR loading table from disk" << std::endl;
                };
            } else {
                for (size_t j = 0; j < nrow; j++) {
                    setInt32(j, i, j);
                }
            }
            memcpy(data + size512[i], &nrow, 4);
        };
    };

    //! CPU memory allocation
    void allocateHost() { // col added manually
        if (mode == 1) {
            data = aligned_alloc<ap_uint<512> >(size512.back());
            data[0] = get_table_header(size512[1], nrow); // TO CHECK
            for (size_t j = 1; j < ncol; j++) {
                data[size512[j]] = 0;
            };
        } else if (mode == 2) { // col added automatically
            size_t depth = nrow + VEC_LEN;
            size_t sizeonecol = size_t((4 * depth + 64 - 1) / 64);
            for (size_t i = 0; i < ncol; i++) {
                size512.push_back(size512.back() + sizeonecol);
                iskdata.push_back(1);
            };
            data = aligned_alloc<ap_uint<512> >(size512.back());
            data[0] = get_table_header(size512[1], 0); // TO CHECK
        } else if (mode == 3) {
            data = aligned_alloc<ap_uint<512> >(size512.back());
        } else {
            std::cout << "ERROR: Table mode not supported" << std::endl;
        }
    };

    void allocateHost(float f, int p_num) { // col added manually
        if ((f == 0) || (p_num == 0))
            std::cout << "ERROR: p_num (" << p_num << ")should be bigger than 1,"
                      << "f(" << f << ")should be bigger than 1" << std::endl;
        if (mode == 2) { // col added automatically
            npart = p_num;
            size_t depth = nrow + VEC_LEN;
            size_t sizeonecol = size_t((4 * depth + 64 - 1) / 64);
            size_t alignedSizeOneCol = (f * sizeonecol + p_num - 1) / p_num;
            for (int j = 0; j < p_num; j++) {
                for (size_t i = 0; i < ncol; i++) {
                    size512.push_back(size512.back() + alignedSizeOneCol);
                    iskdata.push_back(1);
                };
            }
            data = aligned_alloc<ap_uint<512> >(size512.back());
            data[0] = get_table_header(size512[ncol], alignedSizeOneCol, 0); // TO CHECK
            for (int j = 1; j < p_num; j++) data[size512[ncol] * j] = get_table_header(alignedSizeOneCol, 0);
        } else {
            std::cout << "ERROR: Table mode not supported" << std::endl;
        }
    };

    int getNumRow() {
        int nm = *(data);
        return nm;
    };

    void setNumRow(int n) {
        data[0].range(31, 0) = n; // TO CHECK
    };

    template <class T, int N>
    void setcharN(int r, int l, std::array<T, N> array_) {
        long offset = (long)r * N * sizeof(T);
        memcpy((char*)(data + size512[l] + 1) + offset, array_.data(), N * sizeof(T));
    }
    void setInt32(int r, int l, int d) { memcpy((char*)(data + size512[l] + 1) + r * sizeof(int), &d, sizeof(int)); };
    void setInt64(int r, int l, int64_t d) {
        long offset = (long)r * sizeof(int64_t);
        memcpy((char*)(data + size512[l] + 1) + offset, &d, sizeof(int64_t));
    };
    void setInt64_l(int r, int l, int64_t d) { setInt32(r, l, d & 0x000000007fffffff); };
    void setInt64_h(int r, int l, int64_t d) { setInt32(r, l, (int32_t)(d >> 31)); };
    template <class T, int N>
    std::array<T, N> getcharN(int r, int l) {
        std::array<T, N> arr;
        long offset = (long)r * N * sizeof(T);
        memcpy(arr.data(), (char*)(data + size512[l] + 1) + offset, N * sizeof(T));
        return arr;
    };
    int getInt32K(int r, int l) {
        int d;
        memcpy(&d, (char*)(datak + size512[l] + 1) + r * sizeof(int), sizeof(int));
        return d;
    };
    int getInt32(int r, int l) {
        int d;
        memcpy(&d, (char*)(data + size512[l] + 1) + r * sizeof(int), sizeof(int));
        return d;
    };
    int64_t getInt64(int r, int l) {
        int64_t d;
        long offset = (long)r * sizeof(int64_t);
        memcpy(&d, (char*)(data + size512[l] + 1) + offset, sizeof(int64_t));
        return d;
    };
    uint32_t getUint32(int r, int l) {
        uint32_t d;
        memcpy(&d, (char*)(data + size512[l] + 1) + r * sizeof(uint32_t), sizeof(uint32_t));
        return d;
    };
    int64_t combineInt64(int r, int l0, int l1) {
        ap_uint<32> h; // h
        ap_uint<32> l; // l
        int64_t d;

        memcpy(&h, (char*)(data + size512[l0] + 1) + r * sizeof(uint32_t), sizeof(uint32_t));
        memcpy(&l, (char*)(data + size512[l1] + 1) + r * sizeof(uint32_t), sizeof(uint32_t));
        d = (h, l);

        return d;
    };

    int64_t mergeInt64(int32_t l, int32_t h) {
        int64_t h_ = (int64_t)h;
        int64_t h_l = (h_ << 31) + l;
        return h_l;
    }

    bool getKdata() {
        bool datacp = 0;
        int cpNum = 0;
        for (size_t i = 0; i < ncol; i++) {
            if (iskdata[i] == 0) {
                datacp = 1;
            } else {
                cpNum++;
            }
        }
        if (datacp) {
            size_t depth = nrow + VEC_LEN * 2 - 1;
            size_t sizeonecol = size_t((4 * depth + 64 - 1) / 64);
            datak = aligned_alloc<ap_uint<512> >(sizeonecol * cpNum);
            int ki = 0;
            for (size_t i = 0; i < ncol; i++) {
                if (iskdata[i] == 1) {
                    memcpy(datak + ki, data + size512[i], sizeonecol * 64);
                    ki += sizeonecol;
                }
            }
            datak[0] = get_table_header(sizeonecol, nrow); // TO CHECK
        }

        return datacp;
    }

    //! Buffer allocation
    void allocateDevBuffer(cl::Context& context, int bank) {
        // getKdata();
        int cpNum = 0;
        for (size_t i = 0; i < ncol; i++) {
            if (iskdata[i] == 1) {
                cpNum++;
            }
        }
        size_t depth = nrow + VEC_LEN * 2 - 1;
        size_t sizeonecol = size_t((4 * depth + 64 - 1) / 64);

        if (getKdata()) {
            mext = {XCL_MEM_TOPOLOGY | (unsigned int)(bank), datak, 0};
            buffer = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                (size_t)(64 * sizeonecol * cpNum), &mext);
            std::cout << name << " Buffer size: " << (64 * sizeonecol * cpNum) / (1024 * 1024) << " MByte "
                      << std::endl;
        } else {
            mext = {XCL_MEM_TOPOLOGY | (unsigned int)(bank), data, 0};
            buffer = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                (size_t)(64 * size512.back()), &mext);
            std::cout << name << " XBuffer size: " << (64 * size512.back() / (1024 * 1024)) << " MByte " << std::endl;
        }
    };

    void initBuffer(cl::CommandQueue clq) {
        std::vector<cl::Memory> tb;
        tb.push_back(buffer);
        clq.enqueueMigrateMemObjects(tb, CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, nullptr, nullptr);
        clq.enqueueWriteBuffer(buffer, CL_TRUE, 0, 64, data, nullptr, nullptr);
    }

    void getPartDevBuffer(cl::Buffer* subBuf, int p_num, size_t size) {
        cl_buffer_region sub_region[2];
        for (int i = 0; i < p_num; i++) {
            sub_region[i] = {i * size, size};
            subBuf[i] = buffer.createSubBuffer(CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX, CL_BUFFER_CREATE_TYPE_REGION,
                                               &sub_region[i]);
        }
    };

    Table createSubTable(int index) {
        Table tout;
        if (index < npart) {
            cl_buffer_region region{64 * index * size512[ncol], 64 * size512[ncol]};
            tout.buffer = buffer.createSubBuffer(CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX,
                                                 CL_BUFFER_CREATE_TYPE_REGION, &region);
            tout.data = data + index * size512[ncol];
            tout.nrow = tout.data[0].range(31, 0);
            int size512Base = size512[index * ncol];
            for (size_t i = 0; i < ncol; i++) {
                tout.size512.push_back(size512[index * ncol + i] - size512Base);
            }
            // return tout;
        } else {
            std::cout << "index (" << index << ") out of part range (" << npart << ")." << std::endl;
            // return NULL;
        }
        return tout;
    }
    void mergeSubTable(Table* tables, int num) {
        ap_uint<512>* datam = aligned_alloc<ap_uint<512> >(size512.back());
        int rownum = 0;
        for (int i = 0; i < num; i++) {
            rownum += tables[i].data[0].range(31, 0).to_int();
        }
        size_t depth = rownum + VEC_LEN;
        size_t sizeonecol = size_t((4 * depth + 64 - 1) / 64);
        std::cout << "rownum in data" << rownum << std::endl;
        std::cout << "sizeonecol in data" << sizeonecol << std::endl;
        int offsets0 = 0;

        for (size_t k = 0; k < ncol; k++) {
            int offsets1 = 0;
            size512[k + 1] = (size512[k] + sizeonecol);
            for (int i = 0; i < num; i++) {
                ap_uint<512>* subdata = tables[i].data;
                int nnrow = subdata[0].range(31, 0).to_int();
                int size512_one_col = subdata[0].range(63, 32).to_int();
                // int size512_one_tb = subdata[0].range(95, 64).to_int();
                if (nnrow > 0) {
                    // if(size512_one_col!=24) std::cout<<i<<" : "<<size512_one_col<<std::endl<<std::endl;
                    ap_uint<512>* subdata_k = subdata + size512_one_col * k + 1;
                    memcpy(((char*)(datam + offsets0 + 1)) + offsets1, (char*)subdata_k, 4 * nnrow);
                    offsets1 += nnrow * 4;
                }
            }
            offsets0 += sizeonecol;
        }
        // data = datam;
        int64_t sizeofbytes = 64 * size512.back();
        memcpy(data, datam, sizeofbytes);
        data[0] = get_table_header(sizeonecol, rownum); // TO CHECK
        std::cout << "The check" << data[0].range(63, 32).to_int() << " " << data[0].range(31, 0).to_int() << std::endl;
        std::cout << "The check" << data[1].range(95, 64).to_int() << " " << data[1].range(63, 32).to_int() << " "
                  << data[1].range(31, 0).to_int() << std::endl;
        std::cout << "The check" << data[1 + sizeonecol * 7].range(95, 64).to_int() << " "
                  << data[1 + sizeonecol * 7].range(63, 32).to_int() << " "
                  << data[1 + sizeonecol * 7].range(31, 0).to_int() << std::endl;
        std::cout << "The check" << datam[1].range(95, 64).to_int() << " " << datam[1].range(63, 32).to_int() << " "
                  << datam[1].range(31, 0).to_int() << std::endl;
        std::cout << "The check" << datam[1 + sizeonecol * 7].range(95, 64).to_int() << " "
                  << datam[1 + sizeonecol * 7].range(63, 32).to_int() << " "
                  << datam[1 + sizeonecol * 7].range(31, 0).to_int() << std::endl;
    }

   private:
    ap_uint<512> get_table_header(int n512b, int nrow) {
        ap_uint<512> th = 0;
        th.range(31, 0) = nrow;
        th.range(63, 32) = n512b;
        return th;
    };

    ap_uint<512> get_table_header(int hp_size, int blk_size, int nrow) {
        ap_uint<512> th = 0;
        th.range(31, 0) = nrow;
        th.range(63, 32) = blk_size;
        th.range(95, 64) = hp_size;
        return th;
    }

    int mode;
};

void gatherTable_col(Table& tin1, Table& tin2, Table& tout) {
    int nrow = tin1.getNumRow();
    if (nrow != tin2.getNumRow()) {
        std::cout << "colums gathering must tin1.row == tin2.row" << std::endl;
        return;
    }
    for (int r = 0; r < nrow; r++) {
        int tin1_0 = tin1.getInt32(r, 0);
        int tin1_1 = tin1.getInt32(r, 1);
        int tin2_0 = tin2.getInt32(r, 2);
        int tin2_1 = tin2.getInt32(r, 3);
        tout.setInt32(r, 0, tin1_0);
        tout.setInt32(r, 1, tin1_1);
        tout.setInt32(r, 2, tin2_0);
        tout.setInt32(r, 3, tin2_1);
    }
    tout.setNumRow(nrow);
}
void gatherTable_row(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    /*for (int r = 0; r < nrow1; r++) {
        int tin1_0 = tin1.getInt32(r, 0);
        int tin1_1 = tin1.getInt32(r, 1);
    }
    for (int r = nrow1; r < nrow1 + nrow2; r++) {
        int tin2_0 = tin2.getInt32(r, 0);
        int tin2_1 = tin2.getInt32(r, 1);
    }*/
    tout.setNumRow(nrow1 + nrow2);
}

class cfgCmd {
   public:
    ap_uint<512>* cmd;
    cl::Buffer buffer;

    cfgCmd(){};

    void allocateHost() {
        cmd = aligned_alloc<ap_uint<512> >(9);
        memset(cmd, 0, 64 * 9);
    };

    void allocateDevBuffer(cl::Context& context, int bank) {
        cl_mem_ext_ptr_t mext = {XCL_MEM_TOPOLOGY | (unsigned int)(32), cmd, 0};
        buffer = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, (size_t)(64 * 9),
                            &mext);
    };

    void setup(){}; // TODO
};

class AggrCfgCmd {
   public:
    ap_uint<32>* cmd;
    cl::Buffer buffer;

    AggrCfgCmd(){};

    void allocateHost() {
        cmd = aligned_alloc<ap_uint<32> >(128);
        memset(cmd, 0, 4 * 128);
    };

    void allocateDevBuffer(cl::Context& context, int bank) {
        cl_mem_ext_ptr_t mext = {XCL_MEM_TOPOLOGY | (unsigned int)(bank), cmd, 0};

        buffer = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, (size_t)(4 * 128),
                            &mext);
    };

    void setup(){}; // TODO
};

class bufferTmp {
   public:
    cl_mem_ext_ptr_t memExt[16];
    ap_uint<64>* tb_buf[16];
    cl::Buffer buffer[16];

    bufferTmp(cl::Context& context) {
        for (int i = 0; i < 8; i++) {
            tb_buf[i] = aligned_alloc<ap_uint<64> >(HT_BUFF_DEPTH);
        };
        for (int i = 8; i < 16; i++) {
            tb_buf[i] = aligned_alloc<ap_uint<64> >(S_BUFF_DEPTH);
        };

#ifdef USE_DDR
        memExt[0].flags = XCL_MEM_DDR_BANK1;
        memExt[1].flags = XCL_MEM_DDR_BANK1;
        memExt[2].flags = XCL_MEM_DDR_BANK1;
        memExt[3].flags = XCL_MEM_DDR_BANK2;
        memExt[4].flags = XCL_MEM_DDR_BANK2;
        memExt[5].flags = XCL_MEM_DDR_BANK2;
        memExt[6].flags = XCL_MEM_DDR_BANK3;
        memExt[7].flags = XCL_MEM_DDR_BANK3;
        memExt[8].flags = XCL_MEM_DDR_BANK1;
        memExt[9].flags = XCL_MEM_DDR_BANK1;
        memExt[10].flags = XCL_MEM_DDR_BANK1;
        memExt[11].flags = XCL_MEM_DDR_BANK2;
        memExt[12].flags = XCL_MEM_DDR_BANK2;
        memExt[13].flags = XCL_MEM_DDR_BANK2;
        memExt[14].flags = XCL_MEM_DDR_BANK3;
        memExt[15].flags = XCL_MEM_DDR_BANK3;
#else
        memExt[0].flags = (unsigned int)(2) | XCL_MEM_TOPOLOGY;
        memExt[1].flags = (unsigned int)(3) | XCL_MEM_TOPOLOGY;
        memExt[2].flags = (unsigned int)(10) | XCL_MEM_TOPOLOGY;
        memExt[3].flags = (unsigned int)(11) | XCL_MEM_TOPOLOGY;
        memExt[4].flags = (unsigned int)(18) | XCL_MEM_TOPOLOGY;
        memExt[5].flags = (unsigned int)(19) | XCL_MEM_TOPOLOGY;
        memExt[6].flags = (unsigned int)(26) | XCL_MEM_TOPOLOGY;
        memExt[7].flags = (unsigned int)(27) | XCL_MEM_TOPOLOGY;
        memExt[8].flags = (unsigned int)(6) | XCL_MEM_TOPOLOGY;
        memExt[9].flags = (unsigned int)(7) | XCL_MEM_TOPOLOGY;
        memExt[10].flags = (unsigned int)(14) | XCL_MEM_TOPOLOGY;
        memExt[11].flags = (unsigned int)(15) | XCL_MEM_TOPOLOGY;
        memExt[12].flags = (unsigned int)(22) | XCL_MEM_TOPOLOGY;
        memExt[13].flags = (unsigned int)(23) | XCL_MEM_TOPOLOGY;
        memExt[14].flags = (unsigned int)(30) | XCL_MEM_TOPOLOGY;
        memExt[15].flags = (unsigned int)(31) | XCL_MEM_TOPOLOGY;
#endif
        for (int i = 0; i < 16; i++) {
            memExt[i].param = 0;
            memExt[i].obj = tb_buf[i];
        };

        // Map buffers
        for (int i = 0; i < 8; i++) {
            buffer[i] = cl::Buffer(context, CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR,
                                   (size_t)(8 * HT_BUFF_DEPTH), &memExt[i]);
        };
        for (int i = 8; i < 16; i++) {
            buffer[i] = cl::Buffer(context, CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR,
                                   (size_t)(8 * S_BUFF_DEPTH), &memExt[i]);
        };
    };

    void initBuffer(cl::CommandQueue clq) {
        std::vector<cl::Memory> tb;
        for (int i = 0; i < 16; i++) {
            tb.push_back(buffer[i]);
        }
        clq.enqueueMigrateMemObjects(tb, CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, nullptr, nullptr);
    }

}; // end of class

class AggrBufferTmp {
   public:
    cl_mem_ext_ptr_t memExt[8];
    ap_uint<512>* tb_buf[8];
    cl::Buffer buffer[8];

    AggrBufferTmp(cl::Context& context) {
        for (int i = 0; i < 8; i++) {
            tb_buf[i] = aligned_alloc<ap_uint<512> >(S_BUFF_DEPTH / 8);
        };

#ifdef USE_DDR
        memExt[0].flags = XCL_MEM_DDR_BANK1;
        memExt[1].flags = XCL_MEM_DDR_BANK1;
        memExt[2].flags = XCL_MEM_DDR_BANK1;
        memExt[3].flags = XCL_MEM_DDR_BANK2;
        memExt[4].flags = XCL_MEM_DDR_BANK2;
        memExt[5].flags = XCL_MEM_DDR_BANK2;
        memExt[6].flags = XCL_MEM_DDR_BANK3;
        memExt[7].flags = XCL_MEM_DDR_BANK3;
#else
        memExt[0].flags = (unsigned int)(8) | XCL_MEM_TOPOLOGY;
        memExt[1].flags = (unsigned int)(12) | XCL_MEM_TOPOLOGY;
        memExt[2].flags = (unsigned int)(16) | XCL_MEM_TOPOLOGY;
        memExt[3].flags = (unsigned int)(20) | XCL_MEM_TOPOLOGY;
        memExt[4].flags = (unsigned int)(10) | XCL_MEM_TOPOLOGY;
        memExt[5].flags = (unsigned int)(14) | XCL_MEM_TOPOLOGY;
        memExt[6].flags = (unsigned int)(18) | XCL_MEM_TOPOLOGY;
        memExt[7].flags = (unsigned int)(22) | XCL_MEM_TOPOLOGY;
#endif

        for (int i = 0; i < 8; i++) {
            memExt[i].param = 0;
            memExt[i].obj = tb_buf[i];
        };

        // Map buffers
        for (int i = 0; i < 8; i++) {
            buffer[i] = cl::Buffer(context, CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR,
                                   (size_t)(8 * S_BUFF_DEPTH), &memExt[i]);
        };
    };

    void BufferInitial(cl::CommandQueue& q) {
        std::vector<cl::Memory> tb;
        for (int i = 0; i < 8; i++) {
            tb.push_back(buffer[i]);
        }
        q.enqueueMigrateMemObjects(tb, CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, nullptr, nullptr);
    };

}; // end of class

class krnlEngine {
    Table* in1;
    Table* in2;
    Table* out;
    cfgCmd* cfgcmd;
    bufferTmp* buftmp;

    cl::CommandQueue clq;

   public:
    cl::Kernel krnl;
    krnlEngine(){};

    krnlEngine(cl::Program& program, cl::CommandQueue& q, const std::string krnname) {
        krnl = cl::Kernel(program, krnname.c_str());
        clq = q;
    };

    void setup(Table& tb1, Table& tb2, Table& tbout, cfgCmd& cmd, bufferTmp& buf) {
        in1 = &tb1;
        in2 = &tb2;
        out = &tbout;
        cfgcmd = &cmd;
        buftmp = &buf;

        int j = 0;
        krnl.setArg(j++, (in1->buffer));
        krnl.setArg(j++, (in2->buffer));
        krnl.setArg(j++, (out->buffer));
        krnl.setArg(j++, (cfgcmd->buffer));
        for (int r = 0; r < 16; r++) {
            krnl.setArg(j++, (buftmp->buffer)[r]);
        };
    };

    void setup_buf(Table& tb1, cl::Buffer& tb2, cl::Buffer& tbout, cfgCmd& cmd, bufferTmp& buf) {
        in1 = &tb1;
        cfgcmd = &cmd;
        buftmp = &buf;

        int j = 0;
        krnl.setArg(j++, in1->buffer);
        krnl.setArg(j++, tb2);
        krnl.setArg(j++, tbout);
        krnl.setArg(j++, (cfgcmd->buffer));
        for (int r = 0; r < 16; r++) {
            krnl.setArg(j++, (buftmp->buffer)[r]);
        };
    };

    void setup_hp(const int depth, const int index, const int thres, Table& tbin, Table& tbout, cfgCmd& cmd) {
        in1 = &tbin;
        out = &tbout;
        cfgcmd = &cmd;

        int j = 0;
        krnl.setArg(j++, depth);
        krnl.setArg(j++, index);
        krnl.setArg(j++, thres);
        krnl.setArg(j++, (in1->buffer));
        krnl.setArg(j++, (out->buffer));
        krnl.setArg(j++, (cfgcmd->buffer));
    };

    void run(int rc, std::vector<cl::Event>* waitevt, cl::Event* outevt) { clq.enqueueTask(krnl, waitevt, outevt); };
};

class AggrKrnlEngine {
    Table* in;
    Table* out;

    cfgCmd* hpcmd;
    AggrCfgCmd *cfgcmd0, *cfgcmd1;
    AggrBufferTmp* buftmp;

    cl::CommandQueue clq;
    cl::Kernel krnl;

   public:
    AggrKrnlEngine(){};

    AggrKrnlEngine(cl::Program& program, cl::CommandQueue& q, const std::string krnname) {
        krnl = cl::Kernel(program, krnname.c_str());
        clq = q;
    };

    void setup(Table& tb, Table& tbout, AggrCfgCmd& cfg_in, AggrCfgCmd& cfg_out, AggrBufferTmp& buf) {
        in = &tb;
        out = &tbout;
        cfgcmd0 = &cfg_in;
        cfgcmd1 = &cfg_out;
        buftmp = &buf;

        int j = 0;
        krnl.setArg(j++, (in->buffer));
        krnl.setArg(j++, (out->buffer));
        krnl.setArg(j++, (cfgcmd0->buffer));
        krnl.setArg(j++, (cfgcmd1->buffer));
        for (int r = 0; r < 8; r++) {
            krnl.setArg(j++, (buftmp->buffer)[r]);
        };
    };

    void setup_hp(const int depth, const int index, const int thres, Table& tbin, Table& tbout, cfgCmd& cmd) {
        in = &tbin;
        out = &tbout;
        hpcmd = &cmd;

        int j = 0;
        krnl.setArg(j++, depth);
        krnl.setArg(j++, index);
        krnl.setArg(j++, thres);
        krnl.setArg(j++, (in->buffer));
        krnl.setArg(j++, (out->buffer));
        krnl.setArg(j++, (hpcmd->buffer));
    };

    void run(int rc, std::vector<cl::Event>* waitevt, cl::Event* outevt) { clq.enqueueTask(krnl, waitevt, outevt); };
};

class transEngine {
    std::vector<cl::Memory> ib[2];
    cl::CommandQueue clq;

   public:
    transEngine(){};
    transEngine(cl::CommandQueue q) { clq = q; };

    void setq(cl::CommandQueue q) { clq = q; };
    void add(Table* tb) {
        ib[0].push_back((tb->buffer));
        // ib[1].push_back((tb->buffer)[1]);
    };
    void clear_add(Table* tb) {
        ib[0].clear();
        ib[0].push_back((tb->buffer));
        // ib[1].push_back((tb->buffer)[1]);
    };
    void add(cl::Buffer& buf) {
        ib[0].push_back(buf);
        // ib[1].push_back((tb->buffer)[1]);
    };
    void add(cfgCmd* tb) {
        ib[0].push_back((tb->buffer));
        // ib[1].push_back((tb->buffer)[1]);
    };
    void add(AggrCfgCmd* tb) {
        ib[0].push_back((tb->buffer));
        // ib[1].push_back((tb->buffer)[1]);
    };
    void clear() { ib[0].clear(); }
    void host2dev(int rc, std::vector<cl::Event>* waitevt, cl::Event* outevt) {
        clq.enqueueMigrateMemObjects(ib[rc], 0, waitevt, outevt);
    };

    void dev2host(int rc, std::vector<cl::Event>* waitevt, cl::Event* outevt) {
        clq.enqueueMigrateMemObjects(ib[rc], CL_MIGRATE_MEM_OBJECT_HOST, waitevt, outevt);
    };
};

#endif
