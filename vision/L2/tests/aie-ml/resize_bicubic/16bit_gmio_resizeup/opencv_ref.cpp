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
//#include <common/xfcvDataMovers.h>
#include <sstream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <string.h>
#include <vector>
#include "config.h"
int main(int argc, char** argv) {
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
    cv::Mat srcImageR1;
    srcImageR1 = cv::imread(argv[1], -1);
    int width = srcImageR1.cols;
    if (argc >= 4) width = atoi(argv[2]);
    int height = srcImageR1.rows;
    if (argc >= 5) height = atoi(argv[3]);

    if ((width != srcImageR1.cols) || (height != srcImageR1.rows))
        cv::resize(srcImageR1, srcImageR1, cv::Size(width, height));

    // srcImageR1.convertTo(srcImageR, CV_16UC3);

    cv::Mat srcImageR(srcImageR1.rows, srcImageR1.cols, CV_16UC4);
    if (srcImageR1.channels() == 1) {
        cvtColor(srcImageR1, srcImageR1, cv::COLOR_GRAY2RGBA);
    }
    if (srcImageR1.channels() == 3) {
        cvtColor(srcImageR1, srcImageR1, cv::COLOR_RGB2RGBA);
        srcImageR1.convertTo(srcImageR, CV_16UC4, 4);
    }

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
    // Run opencv reference test (resize_bicubic design)
    //////////////////////////////////////////
    cv::Mat srcImageRresize(op_width, op_height, CV_16UC4);
    cv::Mat resize_2ndpass(op_width2, op_height2, CV_16UC4);

    // 1stpass
    cv::resize(srcImageR, srcImageRresize, cv::Size(op_width, op_height), 0, 0, cv::INTER_CUBIC);
    cv::Mat dstRefImage(op_width, op_height, CV_16UC4);
    cv::transpose(srcImageRresize, dstRefImage);
    std::cout << "first pass reference done\n" << std::endl;
    {
        // comparing 1st pass aie vs ref
        cv::Mat dst = cv::imread(argv[4], cv::IMREAD_UNCHANGED);
        // cvtColor(dst, dst, cv::COLOR_RGB2RGBA);
        cv::Mat diff(op_width, op_height, CV_16UC4);
        std::cout << "dst.rows" << dst.rows << "dst.cols" << dst.cols << std::endl;
        std::cout << "dst.channels" << dst.channels() << "dst.elemSize" << dst.elemSize() << std::endl;
        int ref = 0, aie = 0;
        cv::imwrite("ref.png", dstRefImage);
        cv::imwrite("aie.png", dst);
        cv::absdiff(dstRefImage, dst, diff);
        cv::imwrite("diff.png", diff);
        FILE* fp = fopen("cv.txt", "w");
        FILE* fp1 = fopen("aie.txt", "w");
        FILE* fp2 = fopen("diff.txt", "w");
        std::cout << "dst.rows=" << dst.rows << std::endl;
        std::cout << "dst.cols=" << dst.cols << std::endl;
        std::cout << "dstRefImage.cols=" << dstRefImage.cols << std::endl;
        std::cout << "dstRefImage.channels()=" << dstRefImage.channels() << std::endl;

        for (int ii = 0; ii < dst.rows; ii++) {
            for (int jj = 0; jj < dst.cols; jj++) {
                for (int kk = 0; kk < 4; kk++) {
                    uint16_t r_r = dstRefImage.at<cv::Vec4w>(ii, jj)[kk];
                    uint16_t a_r = dst.at<cv::Vec4w>(ii, jj)[kk];
                    fprintf(fp, "%d ", r_r);
                    fprintf(fp1, "%d ", a_r);
                    uint16_t d_p = abs(r_r - a_r);
                    if (abs(r_r - a_r) > 1) {
                        fprintf(fp2, "diff= %d  row=%d col=%d ch=%d\n", d_p, ii, jj, kk);
                    }
                }
                fprintf(fp, "\n");
                fprintf(fp1, "\n");
            }
        }

        /*        std::ofstream ofsin("absdiff.txt",std::fstream::out);

                for (int ii = 0; ii < diff.rows; ii++){

                    for (int jj = 0; jj < diff.cols; jj++) {

                        for(int k=0;k<diff.channels();k++)

                    {

                            int val = diff.at<cv::Vec4w>(ii, jj)[k];

                        ofsin << (int)val;

                        if(k == 3) ofsin << std::endl;

                        else       ofsin << " ";

                    }

                    }

                }

                ofsin.close();*/

        /*         fclose(fp);
                fclose(fp1);
                fclose(fp2); */
        float err_per;
        analyzeDiff(diff, 2, err_per);
        if (err_per > 0) {
            std::cerr << "Test failed" << std::endl;
            //            exit(-1);
        }
        //}
        std::cout << "Test passed" << std::endl;
    }
    {
        // 2ndpass
        cv::resize(dstRefImage, resize_2ndpass, cv::Size(op_width2, op_height2), 0, 0, cv::INTER_CUBIC);
        cv::Mat dstRefImage2(op_width2, op_height2, CV_16UC4);
        cv::transpose(resize_2ndpass, dstRefImage2);

        // compare aie_out vs ref
        cv::Mat dst2 = cv::imread(argv[5], cv::IMREAD_UNCHANGED);
        cvtColor(dst2, dst2, cv::COLOR_RGB2RGBA);
        std::cout << "Analyzing diff\n";
        cv::Mat diff2(op_height2, op_width2, CV_16UC4);
        int ref = 0, aie = 0;
        // cv::imwrite("ref.png", dstRefImage2);
        // cv::imwrite("aie.png", dst2);
        cv::absdiff(dstRefImage2, dst2, diff2);
        // cv::imwrite("diff2.png", diff2);
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
                            //fprintf(fpx, "%d ",r_r);
                            //fprintf(fpy, "%d ",a_r);
                            int d_p= abs(r_r - a_r);
                            if(abs(r_r - a_r) > 1)
                            {
                                fprintf(fpz, "diff= %d  row=%d col=%d ch=%d\n",d_p,ii,jj, kk);
                            }
                        }
                        //fprintf(fpx, "\n");
                        //fprintf(fpy, "\n");
                        fprintf(fpz, "\n");
                    }
                }

                //std::ofstream ofsinx("absdiff2.txt",std::fstream::out);
                for (int ii = 0; ii < diff2.rows; ii++){
                    for (int jj = 0; jj < diff2.cols; jj++) {
                        for(int k=0;k<diff2.channels();k++)
                    {
                            int val = diff2.at<cv::Vec4b>(ii, jj)[k];
                       // ofsinx << (int)val;
                       // if(k == 3) ofsinx << std::endl;
                       // else       ofsinx << " ";
                    }
                    }
                }

                //ofsinx.close();
                //fclose(fpx);
                //fclose(fpy);
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
    }
    return 0;
}
