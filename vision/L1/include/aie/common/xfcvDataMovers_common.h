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

namespace xF {

enum DataMoverKind { TILER, STITCHER };

template <int BITWIDTH>
class EmulAxiData {
    static constexpr int BYTEWIDTH = BITWIDTH / 8;

   public:
    char data[BYTEWIDTH];
    template <typename T>
    EmulAxiData(T m) {
        assert(sizeof(T) <= BYTEWIDTH);
        char* tmp = (char*)&m;
        for (unsigned int i = 0; i < BYTEWIDTH; i++) {
            data[i] = (i < sizeof(T)) ? tmp[i] : 0;
        }
    }
    template <typename T>
    EmulAxiData& operator=(const EmulAxiData& mc) {
        if (this != &mc) {
            for (unsigned int i = 0; i < BYTEWIDTH; i++) {
                data[i] = mc.data[i];
            }
        }
        return *this;
    }
};

template <typename T>
class CtypeToCVMatType {
   public:
    static constexpr uchar type =
        (std::is_same<T, float>::value)
            ? CV_32F
            : (std::is_same<T, double>::value)
                  ? CV_64F
                  : (std::is_same<T, int32_t>::value)
                        ? CV_32S
                        : (std::is_same<T, int16_t>::value)
                              ? CV_16S
                              : (std::is_same<T, uint16_t>::value)
                                    ? CV_16U
                                    : (std::is_same<T, int8_t>::value)
                                          ? CV_8S
                                          : (std::is_same<T, uint8_t>::value)
                                                ? CV_8U
                                                : (std::is_same<T, signed char>::value) ? CV_8S : CV_8U;
};

static xrt::device gpDhdl(nullptr);
static std::vector<char> gHeader;
static const axlf* gpTop = nullptr;
static xrt::uuid xclbin_uuid;

static uint16_t gnTilerInstCount = 0;
static uint16_t gnStitcherInstCount = 0;

void deviceInit(std::string xclBin) {
    if (!(xclBin.empty())) {
        if (!bool(gpDhdl)) {
            assert(gpTop == nullptr);

            gpDhdl = xrt::device(0);
            if (!bool(gpDhdl)) {
                throw std::runtime_error("No valid device handle found. Make sure using right xclOpen index.");
            }

            xclbin_uuid = gpDhdl.load_xclbin(xclBin);
            std::cout << xclbin_uuid.to_string().c_str() << std::endl;
            // adf::registerXRT(dhdl, xclbin_uuid.get());//(const unsigned char*)xclbin_uuid.to_string().c_str());
        }
    }

    if (!bool(gpDhdl)) {
        throw std::runtime_error("No valid device handle found. Make sure using right xclOpen index.");
    }
}

struct xfcvDataMoverParams {
    cv::Size mInputImgSize;
    cv::Size mOutputImgSize;

    xfcvDataMoverParams() : mInputImgSize(0, 0), mOutputImgSize(0, 0) {}

    xfcvDataMoverParams(const cv::Size& inImgSize) : mInputImgSize(inImgSize), mOutputImgSize(inImgSize) {}

    xfcvDataMoverParams(const cv::Size& inImgSize, const cv::Size& outImgSize)
        : mInputImgSize(inImgSize), mOutputImgSize(outImgSize) {}
};
}