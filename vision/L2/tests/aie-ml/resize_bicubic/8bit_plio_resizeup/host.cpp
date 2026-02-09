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

#define PROFILE

#include <fstream>
#include <chrono>
#include <common/xf_aie_sw_utils.hpp>
#include <common/xfcvDataMovers.h>
#include <sstream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <xrt/experimental/xrt_kernel.h>
#include <xrt/experimental/xrt_graph.h>
#include <xrt/experimental/xrt_aie.h>
#include <cmath>
#include <string.h>
#include <vector>

#include "config.h"
template <int FBITS>
uint32_t compute_scalefactor(int M, int N) {
    float x_scale = (float)M / (float)N;
    float scale = x_scale * (1 << FBITS);
    return (uint32_t)(std::roundf(scale));
}

template <int FBITS>
float compute_scalefactor_f(int M, int N) {
    float x_scale = (float)M / (float)N;
    return (x_scale);
}

/*
 ******************************************************************************
 * Top level executable
 ******************************************************************************
 */

int main(int argc, char** argv) {
    try {
        if (argc < 3) {
            std::stringstream errorMessage;
            errorMessage << argv[0] << " <xclbin> <inputImage> [width] [height] [iterations]";
            std::cerr << errorMessage.str();
            throw std::invalid_argument(errorMessage.str());
        }
        const char* xclBinName = argv[1];
        //////////////////////////////////////////
        // Read image from file and resize
        //////////////////////////////////////////
        cv::Mat srcImageR;
        srcImageR = cv::imread(argv[2], -1);

        int width = srcImageR.cols;
        if (argc >= 4) width = atoi(argv[3]);
        int height = srcImageR.rows;
        if (argc >= 5) height = atoi(argv[4]);

        int iterations = 1;
        if (argc >= 6) iterations = atoi(argv[5]);

        if ((width != srcImageR.cols) || (height != srcImageR.rows))
            cv::resize(srcImageR, srcImageR, cv::Size(width, height));

        if (srcImageR.elemSize() == 1) cvtColor(srcImageR, srcImageR, cv::COLOR_GRAY2RGBA);

        if (srcImageR.elemSize() == 3) cvtColor(srcImageR, srcImageR, cv::COLOR_RGB2RGBA);

        std::cout << "Image size" << std::endl;
        std::cout << srcImageR.rows << std::endl;
        std::cout << srcImageR.cols << std::endl;
        std::cout << srcImageR.elemSize() << std::endl;
        std::cout << srcImageR.type() << std::endl;
        std::cout << srcImageR.total() << std::endl;
        std::cout << srcImageR.channels() << std::endl;
        std::cout << srcImageR.size() << std::endl;
        std::cout << "Image size (end)" << std::endl;
        int op_width = IMAGE_WIDTH_OUT;
        int op_height = IMAGE_HEIGHT_OUT;
        int op_width2 = IMAGE_WIDTH_OUT2;
        int op_height2 = IMAGE_HEIGHT_OUT2;

        //////////////////////////////////////////
        // Run opencv reference test (filter2D design)
        //////////////////////////////////////////
        cv::Mat srcImageRresize, resize_2ndpass;
        // 1stpass
        cv::resize(srcImageR, srcImageRresize, cv::Size(op_width, op_height), 0, 0, cv::INTER_CUBIC);
        cv::Mat resize_1DV_trans(op_width, op_height, CV_8UC4);
        cv::transpose(srcImageRresize, resize_1DV_trans);
        cv::Mat dstRefImage = resize_1DV_trans;
        // 2ndpass
        cv::resize(resize_1DV_trans, resize_2ndpass, cv::Size(op_width2, op_height2), 0, 0, cv::INTER_CUBIC);
        cv::Mat resize2_1DV_trans(op_width2, op_height2, CV_8UC4);
        cv::transpose(resize_2ndpass, resize2_1DV_trans);
        cv::Mat dstRefImage2 = resize2_1DV_trans;

        // Initializa device
        xF::deviceInit(xclBinName);

        // Allocate output buffer
        void* dstData = nullptr;
        xrt::bo dst_hndl = xrt::bo(xF::gpDhdl, (op_height * op_width * srcImageR.elemSize()), 0, 0);
        dstData = dst_hndl.map();
        cv::Mat dst(op_width, op_height, dstRefImage.type(), dstData);
        // cv::Mat dst(op_height, op_width, dstRefImage.type(), dstData);

        // resize 1st pass
        {
            uint32_t scale_y_fix = compute_scalefactor<16>(IMAGE_HEIGHT_IN, IMAGE_HEIGHT_OUT);
            float scale_y_f = compute_scalefactor_f<16>(IMAGE_HEIGHT_IN, IMAGE_HEIGHT_OUT);

            // Load image
            void* srcData = nullptr;
            std::cout << "src_hndl size" << (srcImageR.total() * srcImageR.elemSize()) << std::endl;
            xrt::bo src_hndl = xrt::bo(xF::gpDhdl, (srcImageR.total() * srcImageR.elemSize()), 0, 0);
            srcData = src_hndl.map();
            memcpy(srcData, srcImageR.data, (srcImageR.total() * srcImageR.elemSize()));
            std::cout << "dst_hndl size()=" << (op_height * op_width * srcImageR.elemSize()) << std::endl;
            xF::xfcvDataMoverParams params(srcImageR.size(), cv::Size(op_width, op_height));
            xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT_IN, TILE_WIDTH_IN, 8, NO_INSTANCES, 16, false, true>
                tiler(0, 0, false, 4);
            xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_WIDTH_OUT, TILE_HEIGHT_OUT, 8, NO_INSTANCES, 16, false, true>
                stitcher(false);
            // xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_HEIGHT_OUT, TILE_WIDTH_OUT, 8> stitcher(false);
            std::cout << "Graph init. This does nothing because CDO in boot PDI "
                         "already configures AIE.\n";

#if !__X86_DEVICE__
            std::vector<xrt::graph> gHndl;
            std::string graph_name_RTP[6];
            for (int k = 0; k < NO_INSTANCES; k++) {
                std::string graph_name = "resize[" + std::to_string(k) + "]";
                std::cout << graph_name << std::endl;
                gHndl.push_back(xrt::graph(xF::gpDhdl, xF::xclbin_uuid, graph_name));
                std::cout << "XRT graph opened" << std::endl;
                gHndl.back().reset();
                std::cout << "Graph reset done" << std::endl;
                for (int i = 0; i < NO_CORES; i++) {
                    for (int j = 1; j < 6; j++) {
                        graph_name_RTP[j] = graph_name + ".k[" + std::to_string(i) + "].in[" + std::to_string(j) + "]";
                        std::cout << graph_name_RTP[j] << std::endl;
                    }
                    gHndl[k].update(graph_name_RTP[1], CHANNELS);
                    gHndl[k].update(graph_name_RTP[2], scale_y_fix);
                    gHndl[k].update(graph_name_RTP[3], IMAGE_HEIGHT_IN);
                    gHndl[k].update(graph_name_RTP[4], IMAGE_HEIGHT_OUT);
                    gHndl[k].update(graph_name_RTP[5], scale_y_f);
                }
            }
#endif

            START_TIMER
            tiler.compute_metadata(srcImageR.size(), cv::Size(op_width, op_height), false, TILE_WIDTH_OUT,
                                   TILE_HEIGHT_OUT, true);
            STOP_TIMER("Meta data compute time")

            std::chrono::microseconds tt(0);
            for (int iter = 0; iter < iterations; iter++) {
                //@{
                //            std::cout << "Iteration : " << (i + 1) << std::endl;

                auto tiles_sz = tiler.host2aie_nb(&src_hndl, srcImageR.size(), params);
                stitcher.aie2host_nb(&dst_hndl, dst.size(), tiles_sz);

#if !__X86_DEVICE__
                //            std::cout << "Graph run(" << tiles_sz[0] * tiles_sz[1] << ")\n";
                START_TIMER
                for (int i = 0; i < NO_INSTANCES; i++) {
                    gHndl[i].run(tiler.tilesPerCore(i) / NO_CORES);
                }
                for (int i = 0; i < NO_INSTANCES; i++) {
                    gHndl[i].wait();
                }
                STOP_TIMER("resize function")
/*             for(int r=0; r<((tiles_sz[0] * tiles_sz[1])/NO_CORES);r++){
                //std::cout << "before Graph run(" << r << ")\n";
                gHndl.run(1);
                gHndl.wait();
                //std::cout << "Graph run(" << r << ")\n";
            }
 */
#endif

                tiler.wait();
                stitcher.wait();

                std::cout << "Data transfer complete (Stitcher)\n";
                tt += tdiff;
                //@}
            }
#if !__X86_DEVICE__
            for (int i = 0; i < NO_INSTANCES; i++) {
                gHndl[i].end(0);
            }

#endif
            // Analyze output {
            std::cout << "Analyzing diff\n";
            cv::Mat diff(op_height, op_width, CV_8UC4);
            int ref = 0, aie = 0;
            cv::imwrite("ref.png", dstRefImage);
            cv::imwrite("aie.png", dst);
            cv::absdiff(dstRefImage, dst, diff);
            cv::imwrite("diff.png", diff);
            /*FILE *fp=fopen("aie.txt","w");
            FILE *fp1=fopen("cv.txt","w");
            FILE *fp2=fopen("diff.txt","w");

            std::cout<<"dst.rows="<< dst.rows << std::endl;
            std::cout<<"dst.cols="<< dst.cols << std::endl;
            std::cout<<"dstRefImage.cols="<<dstRefImage.cols << std::endl;
            std::cout<<"dstRefImage.channels()="<<dstRefImage.channels() << std::endl;

            for (int ii = 0; ii < dst.rows; ii++) {
                for (int jj = 0; jj < dst.cols; jj++) {
                    for (int kk = 0; kk < 4; kk++) {

                          uint8_t r_r=dstRefImage.at<cv::Vec4b>(ii, jj)[kk];
                          uint8_t a_r=dst.at<cv::Vec4b>(ii, jj)[kk];
                        fprintf(fp, "%d ",r_r);
                        fprintf(fp1, "%d ",a_r);
                        uint d_p= abs(r_r - a_r);
                        if(abs(r_r - a_r) > 1)
                        {
                            fprintf(fp2, "diff= %d  row=%d col=%d ch=%d\n",d_p,ii,jj, kk);
                        }

                    }
                    fprintf(fp, "\n");
                    fprintf(fp1, "\n");
                }
            }
            std::ofstream ofsin("absdiff.txt",std::fstream::out);
            for (int ii = 0; ii < diff.rows; ii++){
                for (int jj = 0; jj < diff.cols; jj++) {
                    for(int k=0;k<diff.channels();k++)
                {
                        int val = diff.at<cv::Vec4b>(ii, jj)[k];
                    ofsin << (int)val;
                    if(k == 3) ofsin << std::endl;
                    else       ofsin << " ";
                }
                }
            }
            ofsin.close();
            fclose(fp);
            fclose(fp1);
            fclose(fp2);
    */
            float err_per;
            analyzeDiff(diff, 2, err_per);
            if (err_per > 0) {
                std::cerr << "Test failed" << std::endl;
                exit(-1);
            }
            //}
            std::cout << "Test passed" << std::endl;
            std::cout << "Average time to process frame : " << (((float)tt.count() * 0.001) / (float)iterations)
                      << " ms" << std::endl;
            std::cout << "Average frames per second : " << (((float)1000000 / (float)tt.count()) * (float)iterations)
                      << " fps" << std::endl;
        }
        // resize 2nd pass
        {
            uint32_t scale_y_fix2 = compute_scalefactor<16>(IMAGE_HEIGHT_IN2, IMAGE_HEIGHT_OUT2);
            float scale_y_f2 = compute_scalefactor_f<16>(IMAGE_HEIGHT_IN2, IMAGE_HEIGHT_OUT2);

            // Load image
            void* srcData2 = nullptr;
            std::cout << "src_hndl2 size" << (dst.total() * dst.elemSize()) << std::endl;
            xrt::bo src_hndl2 = xrt::bo(xF::gpDhdl, (dst.total() * dst.elemSize()), 0, 0);
            srcData2 = src_hndl2.map();
            memcpy(srcData2, dst.data, (dst.total() * dst.elemSize()));
            // Allocate output buffer
            void* dstData2 = nullptr;
            xrt::bo dst_hndl2 = xrt::bo(xF::gpDhdl, (op_height2 * op_width2 * dst.elemSize()), 0, 0);
            dstData2 = dst_hndl2.map();
            cv::Mat dst2(op_width2, op_height2, dstRefImage2.type(), dstData2);
            // cv::Mat dst(op_height, op_width, dstRefImage.type(), dstData);
            std::cout << "dst_hndl2 size()=" << (op_height2 * op_width2 * dst2.elemSize()) << std::endl;
            xF::xfcvDataMoverParams params2(dst.size(), cv::Size(op_width2, op_height2));
            xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT_IN2, TILE_WIDTH_IN2, 8, NO_INSTANCES, 16, false, true>
                tiler2(0, 0, false, 4);
            xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_WIDTH_OUT2, TILE_HEIGHT_OUT2, 8, NO_INSTANCES, 16, false,
                               true>
                stitcher2(false);
            // xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_HEIGHT_OUT, TILE_WIDTH_OUT, 8> stitcher(false);
            std::cout << "Graph init. This does nothing because CDO in boot PDI "
                         "already configures AIE.\n";

#if !__X86_DEVICE__
            std::vector<xrt::graph> gHndl2;
            std::string graph_name_RTP[6];
            for (int k = 0; k < NO_INSTANCES; k++) {
                std::string graph_name = "resize2[" + std::to_string(k) + "]";
                // std::cout << graph_name << std::endl;
                gHndl2.push_back(xrt::graph(xF::gpDhdl, xF::xclbin_uuid, graph_name));
                std::cout << "XRT graph opened" << std::endl;
                gHndl2.back().reset();
                std::cout << "Graph reset done" << std::endl;
                for (int i = 0; i < NO_CORES; i++) {
                    for (int j = 1; j < 6; j++) {
                        graph_name_RTP[j] = graph_name + ".k[" + std::to_string(i) + "].in[" + std::to_string(j) + "]";
                        // std::cout << graph_name_RTP[j] << std::endl;
                    }
                    gHndl2[k].update(graph_name_RTP[1], CHANNELS);
                    gHndl2[k].update(graph_name_RTP[2], scale_y_fix2);
                    gHndl2[k].update(graph_name_RTP[3], IMAGE_HEIGHT_IN2);
                    gHndl2[k].update(graph_name_RTP[4], IMAGE_HEIGHT_OUT2);
                    gHndl2[k].update(graph_name_RTP[5], scale_y_f2);
                }
            }
#endif

            START_TIMER
            tiler2.compute_metadata(dst.size(), cv::Size(op_width2, op_height2), false, TILE_WIDTH_OUT2,
                                    TILE_HEIGHT_OUT2, true);
            STOP_TIMER("Meta data compute time")

            std::chrono::microseconds tt(0);
            for (int i = 0; i < iterations; i++) {
                //@{
                std::cout << "Iteration : " << (i + 1) << std::endl;
                START_TIMER
                auto tiles_sz2 = tiler2.host2aie_nb(&src_hndl2, dst.size(), params2);
                stitcher2.aie2host_nb(&dst_hndl2, dst2.size(), tiles_sz2);

#if !__X86_DEVICE__
                //            gHndl2.run(tiles_sz2[0] * tiles_sz2[1]);
                //            gHndl2.wait();
                //            std::cout << "Graph run(" << tiles_sz2[0] * tiles_sz2[1] << ")\n";
                for (int i = 0; i < NO_INSTANCES; i++) {
                    gHndl2[i].run(tiler2.tilesPerCore(i) / NO_CORES);
                }
                for (int i = 0; i < NO_INSTANCES; i++) {
                    gHndl2[i].wait();
                }

#endif

                tiler2.wait();
                stitcher2.wait();

                STOP_TIMER("resize2 function")
                std::cout << "Data transfer complete (Stitcher)\n";
                tt += tdiff;
                //@}
            }
#if !__X86_DEVICE__
            for (int i = 0; i < NO_INSTANCES; i++) {
                gHndl2[i].end(0);
            }
#endif
            // Analyze output {
            std::cout << "Analyzing diff\n";
            cv::Mat diff2(op_height2, op_width2, CV_8UC4);
            int ref = 0, aie = 0;
            cv::imwrite("ref.png", dstRefImage2);
            cv::imwrite("aie.png", dst2);
            cv::absdiff(dstRefImage2, dst2, diff2);
            cv::imwrite("diff2.png", diff2);
            /*        FILE *fpx=fopen("aie2.txt","w");
                    FILE *fpy=fopen("cv2.txt","w");
                    FILE *fpz=fopen("diff2.txt","w");

                    std::cout<<"dst2.rows="<< dst2.rows << std::endl;
                    std::cout<<"dst2.cols="<< dst2.cols << std::endl;
                    std::cout<<"dstRefImage2.cols="<<dstRefImage2.cols << std::endl;
                    std::cout<<"dstRefImage2.channels()="<<dstRefImage2.channels() << std::endl;

                    for (int ii = 0; ii < dst2.rows; ii++) {
                        for (int jj = 0; jj < dst2.cols; jj++) {
                            for (int kk = 0; kk < 4; kk++) {

                                int   r_r=dstRefImage2.at<cv::Vec4b>(ii, jj)[kk];
                                int   a_r=dst2.at<cv::Vec4b>(ii, jj)[kk];
                                fprintf(fpx, "%d ",r_r);
                                fprintf(fpy, "%d ",a_r);
                                int d_p= abs(r_r - a_r);
                                if(abs(r_r - a_r) > 1)
                                {
                                    fprintf(fpz, "diff= %d  row=%d col=%d ch=%d\n",d_p,ii,jj, kk);
                                }

                            }
                            fprintf(fpx, "\n");
                            fprintf(fpy, "\n");
                            //fprintf(fpz, "\n");
                        }
                    }
                    std::ofstream ofsinx("absdiff2.txt",std::fstream::out);
                    for (int ii = 0; ii < diff2.rows; ii++){
                        for (int jj = 0; jj < diff2.cols; jj++) {
                            for(int k=0;k<diff2.channels();k++)
                        {
                                int val = diff2.at<cv::Vec4b>(ii, jj)[k];
                            ofsinx << (int)val;
                            if(k == 3) ofsinx << std::endl;
                            else       ofsinx << " ";
                        }
                        }
                    }
                    ofsinx.close();
                    fclose(fpx);
                    fclose(fpy);
                    fclose(fpz);
            */
            float err_per;
            analyzeDiff(diff2, 4, err_per);
            if (err_per > 0) {
                std::cerr << "Test failed" << std::endl;
                exit(-1);
            }
            //}
            std::cout << "Test passed" << std::endl;
            std::cout << "Average time to process frame : " << (((float)tt.count() * 0.001) / (float)iterations)
                      << " ms" << std::endl;
            std::cout << "Average frames per second : " << (((float)1000000 / (float)tt.count()) * (float)iterations)
                      << " fps" << std::endl;
        }
        return 0;

    } catch (std::exception& e) {
        const char* errorMessage = e.what();
        std::cerr << "Exception caught: " << errorMessage << std::endl;
        exit(-1);
    }
}
