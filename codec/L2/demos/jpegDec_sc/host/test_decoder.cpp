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

//#define _HLS_TEST_ 1

#ifndef _HLS_TEST_
//#include "xcl2.hpp"
#endif

#include "kernelJpegDecoder.hpp"
#include "utils_XAcc_jpeg.hpp"
#include "xf_utils_sw/logger.hpp"

#include "utils.hpp"

#ifndef __SYNTHESIS__

// ------------------------------------------------------------
// for tmp application and reorder
int16_t* hls_block = (int16_t*)malloc(sizeof(int16_t) * MAX_NUM_COLOR * MAXCMP_BC * 64);
xf::codec::idct_out_t* yuv_row_pointer = (uint8_t*)malloc(sizeof(uint8_t) * MAX_NUM_COLOR * MAXCMP_BC * 64);

// ------------------------------------------------------------
// input strm_iDCT_x8[8] is the row of block yuv in mcu order of sample
// output image_height*image_width*Y ... image_height*image_width*U ... image_height*image_width*V 0a to form a file to
// show the picture
void rebuild_raw_yuv(std::string file_name,
                     xf::codec::bas_info* bas_info,
                     int hls_bc[MAX_NUM_COLOR],
                     // hls::stream<xf::codec::idct_out_t >   strm_iDCT_x8[8],
                     ap_uint<64>* yuv_mcu_pointer) {
    std::string file = file_name.substr(file_name.find_last_of('/') + 1);
    std::string fn = file.substr(0, file.find_last_of(".")) + ".raw";
    FILE* f = fopen(fn.c_str(), "wb");
    std::cout << "WARNING: " << fn << " will be opened for binary write." << std::endl;
    if (!f) {
        std::cerr << "ERROR: " << fn << " cannot be opened for binary write." << std::endl;
    }

    xf::codec::idct_out_t* yuv_mcu_pointer_pix = (uint8_t*)malloc(sizeof(uint8_t) * bas_info->all_blocks * 64);

    int cnt = 0;
    int cnt_row = 0;
    for (int b = 0; b < (int)(bas_info->all_blocks); b++) {
        for (int i = 0; i < 8; i++) { // write one block of Y or U or V
            for (int j = 0; j < 8; j++) {
                yuv_mcu_pointer_pix[cnt] = yuv_mcu_pointer[cnt_row](8 * (j + 1) - 1, 8 * j); // strm_iDCT_x8[j].read();
                cnt++;
            }
            cnt_row++;
        }
    }

write_mcu_raw_data:
    fwrite(yuv_mcu_pointer, sizeof(char), bas_info->all_blocks * 64, f);

    // fwrite(&end_file, 1, 1, f);//write 0x0a
    fclose(f);

    file = file_name.substr(file_name.find_last_of('/') + 1);
    fn = file.substr(0, file.find_last_of(".")) + ".yuv";
    f = fopen(fn.c_str(), "wb");
    std::cout << "WARNING: " << fn << " will be opened for binary write." << std::endl;
    if (!f) {
        std::cerr << "ERROR: " << fn << " cannot be opened for binary write." << std::endl;
    }

    xf::codec::COLOR_FORMAT fmt = bas_info->format;

    int dpos[MAX_NUM_COLOR]; // the dc position of the pointer
    for (int cmp = 0; cmp < MAX_NUM_COLOR; cmp++) {
        dpos[cmp] = 0;
    }

    uint16_t block_width = bas_info->axi_width[0];
    int n_mcu = 0;

    printf("INFO: fmt %d, bas_info->mcu_cmp = %d \n", fmt, (int)(bas_info->mcu_cmp));
    printf("INFO: bas_info->hls_mbs[cmp] %d, %d, %d \n", bas_info->hls_mbs[0], bas_info->hls_mbs[1],
           bas_info->hls_mbs[2]);

LOOP_write_yuv_buffer:
    while (n_mcu < (int)(bas_info->hls_mcuc)) {
        for (int cmp = 0; cmp < MAX_NUM_COLOR; cmp++) {              // 0,1,2
            for (int mbs = 0; mbs < bas_info->hls_mbs[cmp]; mbs++) { // 0,1,2,3, 0, 0,

                for (int i = 0; i < 8; i++) { // write one block of Y or U or V
                    for (int j = 0; j < 8; j++) {
                        yuv_row_pointer[(cmp)*bas_info->axi_height[0] * bas_info->axi_width[0] * 64 + (dpos[cmp]) * 8 +
                                        j * bas_info->axi_width[cmp] * 8 + i] = *yuv_mcu_pointer_pix;
                        yuv_mcu_pointer_pix++;
                    }
                } // end block

                if (fmt == xf::codec::C420) { // 420 mbs= 0 1 2 3 0 0

                    if (mbs == 0) {
                        if (cmp != 0 && (dpos[cmp] % bas_info->axi_width[1] == bas_info->axi_width[1] - 1)) {
                            dpos[cmp] += 1 + bas_info->axi_width[1] * (8 - 1);
                        } else {
                            dpos[cmp] += 1;
                        }
                    } else if (mbs == 1) {
                        dpos[cmp] += block_width * 8 - 1;
                    } else if (mbs == 2) {
                        dpos[cmp] += 1;
                    } else {
                        if (dpos[cmp] % (block_width * (8) * 2) == (8 + 1) * block_width - 1) {
                            dpos[cmp] += 1 + block_width * (8 - 1);
                        } else {
                            dpos[cmp] -= block_width * 8 - 1;
                        }
                    }
                } else if (fmt == xf::codec::C422) { // 422 mbs 0 1 0 0
                    if (mbs == 0) {
                        if (cmp != 0 && (dpos[cmp] % bas_info->axi_width[1] == bas_info->axi_width[1] - 1)) {
                            dpos[cmp] += 1 + bas_info->axi_width[1] * (8 - 1);
                        } else {
                            dpos[cmp] += 1;
                        }
                    } else { // cmp=0, mbs=1
                        if (dpos[cmp] % (block_width) == block_width - 1) {
                            dpos[cmp] += 1 + block_width * (8 - 1);
                        } else {
                            dpos[cmp] += 1;
                        }
                    }
                } else {
                    if (dpos[cmp] % block_width == block_width - 1) {
                        dpos[cmp] += 1 + block_width * (8 - 1);
                    } else {
                        dpos[cmp] += 1;
                    }
                }
            }
        } // end one mcu
        n_mcu++;
    }

    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 8; j++) {
            printf("%02X, ", (uint8_t)(yuv_row_pointer[8 * i + j]));
        }
        printf("\n");
    }

    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 8; j++) {
            printf("%d, ", (uint8_t)(yuv_row_pointer[8 * i + j]));
        }
        printf("\n");
    }

LOOP_write_y:
    fwrite(yuv_row_pointer, sizeof(char), bas_info->axi_height[0] * bas_info->axi_width[0] * 64, f);
LOOP_write_u:
    fwrite(yuv_row_pointer + bas_info->axi_height[0] * bas_info->axi_width[0] * 64, sizeof(char),
           bas_info->axi_height[1] * bas_info->axi_width[1] * 64, f);
LOOP_write_v:
    fwrite(yuv_row_pointer + bas_info->axi_height[0] * bas_info->axi_width[0] * 128, sizeof(char),
           bas_info->axi_height[2] * bas_info->axi_width[2] * 64, f);

    // fwrite(&end_file, 1, 1, f);//write 0x0a
    fclose(f);

    printf("Please open the YUV file with fmt %d and (width, height) = (%d, %d) \n", fmt, bas_info->axi_width[0] * 8,
           bas_info->axi_height[0] * 8);

    // write yuv info to a file
    fn = file.substr(0, file.find_last_of(".")) + ".yuv.h";
    f = fopen(fn.c_str(), "aw");
    std::cout << "WARNING: " << fn << " will be opened for binary write." << std::endl;
    if (!f) {
        std::cerr << "ERROR: " << fn << " cannot be opened for binary write." << std::endl;
    }
    fprintf(f, "INFO: fmt=%d, bas_info->mcu_cmp=%d\n", fmt, (int)(bas_info->mcu_cmp));
    fprintf(f, "INFO: bas_info->hls_mbs[cmp] %d, %d, %d \n", bas_info->hls_mbs[0], bas_info->hls_mbs[1],
            bas_info->hls_mbs[2]);
    fprintf(f, "Please open the YUV file with fmt %d and (width, height) = (%d, %d) \n", fmt,
            bas_info->axi_width[0] * 8, bas_info->axi_height[0] * 8);
    fclose(f);
}

// ------------------------------------------------------------
void rebuild_infos(xf::codec::img_info& img_info,
                   xf::codec::cmp_info cmp_info[MAX_NUM_COLOR],
                   xf::codec::bas_info& bas_info,
                   int& rtn,
                   int& rtn2,
                   ap_uint<32> infos[1024]) {
    img_info.hls_cs_cmpc = *(infos + 0);
    img_info.hls_mcuc = *(infos + 1);
    img_info.hls_mcuh = *(infos + 2);
    img_info.hls_mcuv = *(infos + 3);
    rtn = *(infos + 4);
    rtn2 = *(infos + 5);

    bas_info.all_blocks = *(infos + 10);
    for (int i = 0; i < MAX_NUM_COLOR; i++) {
        bas_info.axi_height[i] = *(infos + 11 + i);
    }
    for (int i = 0; i < 4; i++) {
        bas_info.axi_map_row2cmp[i] = *(infos + 14 + i);
    }
    bas_info.axi_mcuv = *(infos + 18);
    bas_info.axi_num_cmp = *(infos + 19);
    bas_info.axi_num_cmp_mcu = *(infos + 20);
    for (int i = 0; i < MAX_NUM_COLOR; i++) {
        bas_info.axi_width[i] = *(infos + 21 + i);
    }
    int format = *(infos + 24);
    bas_info.format = (xf::codec::COLOR_FORMAT)format;
    for (int i = 0; i < MAX_NUM_COLOR; i++) {
        bas_info.hls_mbs[i] = *(infos + 25 + i);
    }
    bas_info.hls_mcuc = *(infos + 28);
    for (int c = 0; c < MAX_NUM_COLOR; c++) {
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                bas_info.idct_q_table_x[c][i][j] = *(infos + 29 + c * 64 + i * 8 + j);
            }
        }
    }
    for (int c = 0; c < MAX_NUM_COLOR; c++) {
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                bas_info.idct_q_table_y[c][i][j] = *(infos + 221 + c * 64 + i * 8 + j);
            }
        }
    }
    bas_info.mcu_cmp = *(infos + 413);
    for (int c = 0; c < MAX_NUM_COLOR; c++) {
        for (int i = 0; i < 64; i++) {
            bas_info.min_nois_thld_x[c][i] = *(infos + 414 + c * 64 + i);
        }
    }
    for (int c = 0; c < MAX_NUM_COLOR; c++) {
        for (int i = 0; i < 64; i++) {
            bas_info.min_nois_thld_y[c][i] = *(infos + 606 + c * 64 + i);
        }
    }
    for (int c = 0; c < MAX_NUM_COLOR; c++) {
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                bas_info.q_tables[c][i][j] = *(infos + 798 + c * 64 + i * 8 + j);
            }
        }
    }
    for (int c = 0; c < MAX_NUM_COLOR; c++) {
        cmp_info[c].bc = *(infos + 990 + c * 6);
        cmp_info[c].bch = *(infos + 991 + c * 6);
        cmp_info[c].bcv = *(infos + 992 + c * 6);
        cmp_info[c].mbs = *(infos + 993 + c * 6);
        cmp_info[c].sfh = *(infos + 994 + c * 6);
        cmp_info[c].sfv = *(infos + 995 + c * 6);
    }

    printf("test INFO:  bas_info->mcu_cmp = %d \n", (int)(bas_info.mcu_cmp));
    printf("test INFO: bas_info->hls_mbs[cmp] %d, %d, %d \n", bas_info.hls_mbs[0], bas_info.hls_mbs[1],
           bas_info.hls_mbs[2]);
}

// ------------------------------------------------------------

int main(int argc, const char* argv[]) {
    std::cout << "\n------------ Test for decode image.jpg  -------------\n";
    std::string optValue;
    std::string JPEGFile;
    std::string xclbin_path;

    // cmd arg parser.
    ArgParser parser(argc, argv);

    // Read In paths addresses
    if (parser.getCmdOption("-JPEGFile", optValue)) {
        JPEGFile = optValue;
        std::cout << "COMMOND: host.exe -JPEGFile " << optValue << std::endl;
    } else {
        std::cout << "INFO: JPEG file not specified for this test. use "
                     "'-JPEGFile' to specified it. \n";
    }

    ///// declaration

    // load data to simulate the ddr data
    // size of jpeg_pointer, output of yuv_mcu_pointer, and output image infos
    int size;
    uint8_t* jpeg_pointer;
#ifndef _HLS_TEST_
    ap_uint<64>* yuv_mcu_pointer = aligned_alloc<ap_uint<64> >(sizeof(ap_uint<64>) * MAXCMP_BC * 8);
    ap_uint<32>* infos = aligned_alloc<ap_uint<32> >(sizeof(ap_uint<32>) * 1024);
#else
    ap_uint<64>* yuv_mcu_pointer = (ap_uint<64>*)malloc(sizeof(ap_uint<64>) * MAXCMP_BC * 8);
    ap_uint<32>* infos = (ap_uint<32>*)malloc(sizeof(ap_uint<32>) * 1024);
#endif
    int err = load_dat(jpeg_pointer, JPEGFile, size);
    if (err) {
        printf("Alloc buf failed!, size:%d Bytes\n", size);
        return err;
    } else {
        printf("Alloc buf successfully!, size:%d Bytes\n", size);
    }

    // Variables to measure time

    // To test SYNTHESIS top
    hls::stream<ap_uint<24> > block_strm;
    xf::codec::cmp_info cmp_info[MAX_NUM_COLOR];
    xf::codec::img_info img_info;
    xf::codec::bas_info bas_info;
    img_info.hls_cs_cmpc = 0; // init

    // 0: decode jfif successful
    // 1: marker in jfif is not in expectation
    int rtn = 0;

    // 0: decode huffman successful
    // 1: huffman data is not in expectation
    int rtn2 = false;

#ifdef _HLS_TEST_
    uint32_t hls_mcuc;
    uint16_t hls_mcuh;
    uint16_t hls_mcuv;
    uint8_t hls_cs_cmpc;
    hls::stream<ap_uint<5> > idx_coef;
    hls::stream<xf::codec::idct_out_t> strm_iDCT_x8[8];

    // L2 top
    kernelJpegDecoder((ap_uint<CH_W>*)jpeg_pointer, (int)size,
                      //&img_info, cmp_info, &bas_info,
                      yuv_mcu_pointer, infos);
    // strm_iDCT_x8);//idx_coef,

    rebuild_infos(img_info, cmp_info, bas_info, rtn, rtn2, infos);
    // one shoot test for the IDCT
    printf("INFO: bas_info.q_tables are : \n");
    for (int id = 0; id < 2; id++) {
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                printf("%d, ", (int)(bas_info.q_tables[id][i][j]));
            }
            printf("\n");
        }
    }
#else
    xf::common::utils_sw::Logger logger(std::cout, std::cerr);

    // send task requests
    auto jpeg_pointer_pool = jpegDec_acc::create_bufpool(vpp::input);
    auto yuv_mcu_pointer_pool = jpegDec_acc::create_bufpool(vpp::output);
    auto infos_pool = jpegDec_acc::create_bufpool(vpp::output);

    jpegDec_acc::send_while([&]() -> bool {
        uint8_t* acc_jpeg_pointer = (uint8_t*)jpegDec_acc::alloc_buf(jpeg_pointer_pool, sizeof(uint8_t) * size);
        ap_uint<64>* acc_yuv_mcu_pointer =
            (ap_uint<64>*)jpegDec_acc::alloc_buf(yuv_mcu_pointer_pool, sizeof(ap_uint<64>) * MAXCMP_BC * 8);
        ap_uint<32>* acc_infos = (ap_uint<32>*)jpegDec_acc::alloc_buf(infos_pool, sizeof(ap_uint<32>) * 1024);

        memcpy(acc_jpeg_pointer, jpeg_pointer, sizeof(uint8_t) * size);

        jpegDec_acc::compute((ap_uint<CH_W>*)acc_jpeg_pointer, size, acc_yuv_mcu_pointer, acc_infos);

        return 0;
    });

    // send result receiving requests
    jpegDec_acc::receive_all_in_order([&]() {
        ap_uint<64>* acc_yuv_mcu_pointer = (ap_uint<64>*)jpegDec_acc::get_buf(yuv_mcu_pointer_pool);
        ap_uint<32>* acc_infos = (ap_uint<32>*)jpegDec_acc::get_buf(infos_pool);

        memcpy(yuv_mcu_pointer, acc_yuv_mcu_pointer, sizeof(ap_uint<64>) * MAXCMP_BC * 8);
        memcpy(infos, acc_infos, sizeof(ap_uint<32>) * 1024);

        rebuild_infos(img_info, cmp_info, bas_info, rtn, rtn2, acc_infos);

    });

    struct timeval start_time, end_time;
    gettimeofday(&start_time, 0);
    jpegDec_acc::join();
    gettimeofday(&end_time, 0);

    std::cout << "INFO: Finish kernel execution" << std::endl;
    std::cout << "INFO: Finish E2E execution" << std::endl;
    std::cout << "-------------------------------------------------------" << std::endl;
    unsigned long exec_timeE2E = diff(&end_time, &start_time);
    std::cout << "INFO: Average E2E per run: " << exec_timeE2E << " us\n";
    std::cout << "-------------------------------------------------------" << std::endl;

    rebuild_infos(img_info, cmp_info, bas_info, rtn, rtn2, infos);
#endif
    // for image info
    int hls_bc[MAX_NUM_COLOR];
    for (int i = 0; i < MAX_NUM_COLOR; i++) {
        hls_bc[i] = cmp_info[i].bc;
    }

    // todo merge to syn-code

    if (rtn || rtn2) {
        printf("Warning: Decoding the bad case input file!\n");
        if (rtn == 1) {
            printf("Warning: [code 1] marker in jfif is not in expectation!\n");
        } else if (rtn == 2) {
            printf("ERROR: [code 2] huffman table is not in expectation!\n");
        } else {
            if (rtn2) {
                printf("Warning: [code 3] huffman data is not in expectation!\n");
            }
        }
        return 1;
#ifndef _HLS_TEST_
        logger.error(xf::common::utils_sw::Logger::Message::TEST_FAIL);
    } else {
        logger.info(xf::common::utils_sw::Logger::Message::TEST_PASS);
#endif
    }

    printf("INFO: writing the YUV file!\n");
    rebuild_raw_yuv(JPEGFile, &bas_info, hls_bc, yuv_mcu_pointer);

    free(jpeg_pointer);
    free(hls_block);
    free(infos);
    free(yuv_row_pointer);

    std::cout << "Ready for next image!\n ";

    return 0;
}
#endif

// ************************************************************
