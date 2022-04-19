/*
 * Copyright 2022 Xilinx, Inc.
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
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <algorithm>
#include <vector>
#include "knn_acc.hpp"
#include "types.hpp"
using namespace std;
const int nm_card = 1;

inline int tvdiff(struct timeval* tv0, struct timeval* tv1) {
    return (tv1->tv_sec - tv0->tv_sec) * 1000000 + (tv1->tv_usec - tv0->tv_usec);
}

struct idx_dist_t {
    int32_t index;
    float distance;
};

bool compare_func(idx_dist_t i, idx_dist_t j) {
    return (i.distance < j.distance);
}

struct csv_blk_t {
    char* csv_blk;
    int csv_size;
};

// for string delimiter
vector<string> split(string s, string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    string token;
    vector<string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != string::npos) {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(s.substr(pos_start));
    return res;
}

int golden_generate(
    const char* csv_file, int col_x, int col_y, float base_x, float base_y, std::vector<idx_dist_t>& golden) {
    // open tbl file
    ifstream ifile(csv_file, std::ifstream::binary);
    if (!ifile.is_open()) {
        cout << "Error opening file " << csv_file << endl;
        return -1;
    }
    string s_line = "";
    int32_t idx = 0;
    getline(ifile, s_line); // skip line 1
    while (getline(ifile, s_line)) {
        vector<string> cols = split(s_line, ",");
        // std::cout << cols[col_x] << " " << cols[col_y] << std::endl;
        float x = stof(cols[col_x]);
        float y = stof(cols[col_y]);
        float dist = std::sqrt((x - base_x) * (x - base_x) + (y - base_y) * (y - base_y));
        idx_dist_t tmp;
        tmp.distance = dist;
        tmp.index = idx++;
        golden.push_back(tmp);
    }

    std::sort(golden.begin(), golden.end(), compare_func);
    ifile.close();
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 7) {
        printf("ERROR: not enough arguments. need 7; given %d\n", argc);
        printf("Usage: ./host.exe schema_file csv_file point_x point_y top_k nm_line_split\n");
        return 1;
    }
    const char* schema_file = argv[1];
    const char* csv_file = argv[2];
    const float base_x = atof(argv[3]);
    const float base_y = atof(argv[4]);
    const int top_k = atoi(argv[5]);
    const int nm_lines = atoi(argv[6]);
    if (top_k > MAX_SORT_NM) {
        printf("ERROR: exceed limit for top-k. %d > %d\n", top_k, MAX_SORT_NM);
        return 1;
    }

    struct timeval tr0;
    struct timeval ts, te;
    gettimeofday(&tr0, 0);
    /***************** Read in Schema *****************/
    // schema2mem
    gettimeofday(&ts, 0);
    std::ifstream sfile(schema_file, std::ios::in);
    if (!sfile.is_open()) {
        printf("ERROR: failed to open file %s\n", schema_file);
        return 1;
    }

    char line[256];
    std::string schema_lines;
    while (sfile.getline(line, sizeof(line))) {
        std::stringstream tmp(line);
        schema_lines.append(tmp.str());
        schema_lines.push_back('\n');
    }
    sfile.close();

    const int schema_sz = schema_lines.size() + 2;
    ap_uint<8>* schema_in = new ap_uint<8>[ schema_sz ];
    schema_in[0] = schema_lines.size() % 256;
    schema_in[1] = schema_lines.size() / 256;
    memcpy(&schema_in[2], &schema_lines.at(0), schema_lines.size());

    gettimeofday(&te, 0);
    printf("INFO: pre execution time - schema2mem time: %.3f ms\n", tvdiff(&ts, &te) / 1000.0);
    // end schema2mem

    /***************** Read in CSV *****************/
    squeue<csv_blk_t*> block_q;
    block_q.init_size(4, 0);
    std::vector<int> line_cnt;
    std::thread prepare_data = std::thread([&]() {
        struct timeval tps, tpe;
        std::ifstream cfile(csv_file, std::ios::in);
        if (!cfile.is_open()) {
            printf("ERROR: failed to open file %s\n", csv_file);
            return 1;
        }

        std::string tmp;
        std::getline(cfile, tmp); // skip line 1
        int cnt = 0;
        int blk_cnt = 0;
        uint64_t blk_start_pos = cfile.tellg();
        uint64_t blk_end_pos = cfile.tellg();

        gettimeofday(&tps, 0);
        while (std::getline(cfile, tmp)) {
            cnt++;
            if (cnt == nm_lines) {
                blk_end_pos = cfile.tellg();
                csv_blk_t* csv_p = new csv_blk_t;
                csv_p->csv_blk = new char[blk_end_pos - blk_start_pos];
                csv_p->csv_size = blk_end_pos - blk_start_pos;

                cfile.seekg(blk_start_pos);
                cfile.read(csv_p->csv_blk, csv_p->csv_size);

                block_q.put(csv_p);
                line_cnt.push_back(cnt);

                cnt = 0;
                blk_start_pos = blk_end_pos;

                gettimeofday(&tpe, 0);
                printf("INFO: pre execution time -- prepare block %d: %.3f ms\n", blk_cnt++,
                       tvdiff(&tps, &tpe) / 1000.0);
                gettimeofday(&tps, 0);
            }
        }

        if (cnt != 0) {
            gettimeofday(&tps, 0);
            cfile.clear();
            cfile.seekg(0, cfile.end);
            blk_end_pos = cfile.tellg();
            csv_blk_t* csv_p = new csv_blk_t;
            csv_p->csv_blk = new char[blk_end_pos - blk_start_pos];
            csv_p->csv_size = blk_end_pos - blk_start_pos;
            cfile.seekg(blk_start_pos);
            cfile.read(csv_p->csv_blk, csv_p->csv_size);
            block_q.put(csv_p);
            gettimeofday(&tpe, 0);
            printf("INFO: pre execution time -- prepare block %d: %.3f ms\n", blk_cnt++, tvdiff(&tps, &tpe) / 1000.0);

            csv_blk_t* null_p = new csv_blk_t;
            null_p->csv_size = 0;
            block_q.put(null_p);
            line_cnt.push_back(cnt);
        } else {
            csv_blk_t* null_p = new csv_blk_t;
            null_p->csv_size = 0;
            block_q.put(null_p);
        }
        cfile.close();
        return 0;
    });

    /***************** Settings for SC *****************/
    gettimeofday(&ts, 0);
    VPP_CC cuCluster[nm_card];
    for (int i = 0; i < nm_card; ++i) {
        knn_acc::add_card(cuCluster[i], i);
    }

    auto schema_buff_pool = knn_acc::create_bufpool(vpp::input);
    auto csv_buff_pool = knn_acc::create_bufpool(vpp::input);
    auto sorted_dist_pool = knn_acc::create_bufpool(vpp::output);
    auto sorted_idx_pool = knn_acc::create_bufpool(vpp::output);

    gettimeofday(&te, 0);
    printf("INFO: sc setup time: %.3f ms\n", tvdiff(&ts, &te) / 1000.0);

    /***************** Send and Receive Data *****************/
    std::vector<idx_dist_t> index_distance_vec;
    std::thread process_data = std::thread([&]() {
        gettimeofday(&ts, 0);
        int round = 0;
        auto send_fn = [&]() -> bool {
            knn_acc::set_handle(round);
            // Move data
            csv_blk_t* csv_block = block_q.get();
            if (csv_block->csv_size == 0) {
                delete csv_block;
                return false;
            }

            int csv_sz = (csv_block->csv_size + 15) / 16;
            const int NM = CSV_PU_NM;
            int avg = csv_sz / NM;
            int left = csv_sz - avg * NM;
            int last = avg + left;
            ap_uint<128> head = 0;
            head.range(31, 0) = avg;
            head.range(63, 32) = last;

            ap_uint<8>* schema_buff_p = (ap_uint<8>*)knn_acc::alloc_buf(schema_buff_pool, schema_sz);
            ap_uint<128>* csv_buff_p =
                (ap_uint<128>*)knn_acc::alloc_buf(csv_buff_pool, (csv_sz + 1) * sizeof(ap_uint<128>));
            float* sorted_dist_p = (float*)knn_acc::alloc_buf(sorted_dist_pool, top_k * sizeof(float));
            uint32_t* sorted_idx_p = (uint32_t*)knn_acc::alloc_buf(sorted_idx_pool, top_k * sizeof(uint32_t));

            struct timeval tms, tme;
            gettimeofday(&tms, 0);

            memcpy(schema_buff_p, schema_in, schema_sz);
            memcpy(csv_buff_p, &head, 16);
            memcpy(&csv_buff_p[1], csv_block->csv_blk, csv_block->csv_size);

            gettimeofday(&tme, 0);
            printf("INFO: send round: %d; size of csv: %.3f MB; memcpy time: %.3f ms\n", round,
                   csv_sz * 16.0 / 1024 / 1024, tvdiff(&tms, &tme) / 1000.0);

            knn_acc::compute(csv_buff_p, schema_buff_p, base_x, base_y, top_k, sorted_dist_p, sorted_idx_p);
            round++;
            delete[] csv_block->csv_blk;
            delete csv_block;
            return true;
        };

        auto receive_fn = [&]() {
            int idx = knn_acc::get_handle();
            printf("INFO: receive round %d\n", idx);
            float* ret_sorted_dist = (float*)knn_acc::get_buf(sorted_dist_pool);
            uint32_t* ret_sorted_idx = (uint32_t*)knn_acc::get_buf(sorted_idx_pool);
            idx_dist_t tmp;

            for (int i = 0; i < top_k; ++i) {
                tmp.distance = ret_sorted_dist[i];
                tmp.index = ret_sorted_idx[i];
                for (int j = 0; j < idx; ++j) {
                    tmp.index += line_cnt[j];
                }
                // printf("INFO: receive round %d: TOP-%d distance-%f index-%d\n", idx, i + 1, ret_sorted_dist[i],
                //        ret_sorted_idx[i]);
                index_distance_vec.push_back(tmp);
            }
        };

        for (int i = 0; i < nm_card; ++i) {
            knn_acc::send_while([&]() -> bool { return send_fn(); }, cuCluster[i]);
            knn_acc::receive_all_in_order([&]() { receive_fn(); }, cuCluster[i]);
        }
        for (int i = 0; i < nm_card; ++i) {
            knn_acc::join(cuCluster[i]);
        }

        gettimeofday(&te, 0);
        printf("INFO: kernel execution time: %.3f ms\n", tvdiff(&ts, &te) / 1000.0);
    });

    prepare_data.join();
    process_data.join();

    // sort on cpu
    gettimeofday(&ts, 0);
    std::sort(index_distance_vec.begin(), index_distance_vec.end(), compare_func);
    for (int i = 0; i < top_k; ++i) {
        printf("INFO: TOP-%d distance-%f index-%d\n", i + 1, index_distance_vec[i].distance,
               index_distance_vec[i].index);
    }

    gettimeofday(&te, 0);
    printf("INFO: post execution time(cpu sort): %.3f ms\n", tvdiff(&ts, &te) / 1000.0);
    printf("INFO: e2e execution time: %.3f ms\n", tvdiff(&tr0, &te) / 1000.0);

    // result checker
    vector<idx_dist_t> golden;
    golden_generate(csv_file, 5, 6, base_x, base_y, golden);

    int err = 0;
    for (int i = 0; i < top_k; ++i) {
        if (fabs(index_distance_vec[i].distance - golden[i].distance) > 1e-6 ||
            index_distance_vec[i].index != golden[i].index)
            err++;
    }

    if (!err) {
        printf("INFO: Test PASS.\n");
    } else {
        printf("INFO: Test FAIL.\n");
    }

    delete[] schema_in;
    return err;
}
