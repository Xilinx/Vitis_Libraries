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

#include "xf_security/crc32c_sc.hpp"

int crc32c_run(std::vector<std::string> in_file, std::vector<uint32_t> in_size, std::vector<uint32_t>& out_result) {
    // get the number of input files
    uint32_t num = in_file.size();
    uint32_t num2 = in_size.size();
    if (num != num2) {
        std::cout << "ERROR: size of in_file have to be equal to size of in_size.\n";
        return 1;
    }

    int* fd = (int*)malloc(sizeof(int) * num);
    for (int i = 0; i < num; i++) {
        fd[i] = open(in_file[i].c_str(), O_RDONLY | O_DIRECT, S_IRWXU);
        if (fd[i] < 0) {
            std::cout << "ERROR: Open file \"" << in_file[i] << "\" failed.\n";
        }
    }

    // total number of bytes of input
    uint32_t total_size = 0;
#ifdef USE_P2P
    auto in_buff_pool = crc32c_acc::create_bufpool(vpp::input, vpp::p2p);
#else
    auto in_buff_pool = crc32c_acc::create_bufpool(vpp::input);
#endif
    auto len_buff_pool = crc32c_acc::create_bufpool(vpp::input);
    auto out_buff_pool = crc32c_acc::create_bufpool(vpp::output);
    uint32_t index = 0;
    // send task requests
    crc32c_acc::send_while([&]() -> bool {
        struct stat sb;
        fstat(fd[index], &sb);
        uint32_t file_size = sb.st_size;
        total_size += file_size;
#ifdef USE_P2P
        ap_uint<512>* acc_in_buff = (ap_uint<512>*)crc32c_acc::file_buf(in_buff_pool, fd[index], file_size, 0, 0);
#else
        ap_uint<512>* acc_in_buff = (ap_uint<512>*)crc32c_acc::alloc_buf(in_buff_pool, file_size);
        pread(fd[index], acc_in_buff, file_size, 0);
#endif
        ap_uint<32>* acc_len_buff = (ap_uint<32>*)crc32c_acc::alloc_buf(len_buff_pool, sizeof(ap_uint<32>) * 2);
        acc_len_buff[0] = 1;
        acc_len_buff[1] = file_size;
        ap_uint<32>* acc_out_buff = (ap_uint<32>*)crc32c_acc::alloc_buf(out_buff_pool, sizeof(ap_uint<32>));

        crc32c_acc::compute(acc_in_buff, acc_len_buff, acc_out_buff);

        index++;
        return index < num;
    });

    // send result receiving requests
    uint32_t order = 0;
    ap_uint<32>* out_buff = aligned_alloc<ap_uint<32> >(num);
    crc32c_acc::receive_all_in_order([&]() {
        ap_uint<32>* acc_ret_out_buff = (ap_uint<32>*)crc32c_acc::get_buf(out_buff_pool);
        memcpy(&out_buff[order++], acc_ret_out_buff, sizeof(ap_uint<32>));
    });

    std::cout << "Starting crc32c_acc::join()\n";
    struct timeval start_time, end_time;
    gettimeofday(&start_time, 0);
    crc32c_acc::join();
    gettimeofday(&end_time, 0);
    for (int i = 0; i < num; i++) {
        out_result.push_back((uint32_t)out_buff[i]);
    }

    std::cout << "Finishing crc32c_acc::join()\n";
    std::cout << "Total execution time " << tvdiff(&start_time, &end_time) / 1000 << "ms" << std::endl;
    std::cout << "End-to-end throughput "
              << ((double)total_size / 1024.0 / 1024.0 / 1024.0) / (tvdiff(&start_time, &end_time) / 1000000.0)
              << " GB/s.\n";

    return 0;
}
