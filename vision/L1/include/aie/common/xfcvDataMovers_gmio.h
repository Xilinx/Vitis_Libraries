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

#ifndef _XFCVDATAMOVERS_GMIO
#define _XFCVDATAMOVERS_GMIO

namespace xF {

template <DataMoverKind KIND,
          typename DATA_TYPE,
          int TILE_HEIGHT_MAX,
          int TILE_WIDTH_MAX,
          int AIE_VECTORIZATION_FACTOR,
          int CORES>
class xfcvDataMovers<KIND, DATA_TYPE, TILE_HEIGHT_MAX, TILE_WIDTH_MAX, AIE_VECTORIZATION_FACTOR, CORES, 0, true> {
    // using DataCopyF_t = std::function<int(DATA_TYPE*, DATA_TYPE*,
    // std::vector<int>&, int, int)>;

   private:
    uint16_t mOverlapH;
    uint16_t mOverlapV;
    uint16_t mTileRows;
    uint16_t mTileCols;
    bool mbUserHndl;
    bool mTilerRGBtoRGBA;
    bool mStitcherRGBAtoRGB;
    bool mIsOutputResize;
    bool isDynamicOutputResize;
    cv::Size mOutSize;
    uint8_t mchannels;

    cv::Mat* mpImage;
    DATA_TYPE* mpImgData;
    std::array<uint16_t, 3> mImageSize; // Rows, Cols, Elem Size

    std::vector<smartTileMetaData> mMetaDataList;

    // xrtBufferHandle mImageBOHndl;
    xrt::aie::bo mImageBOHndl;
    xrt::aie::bo::async_handle* mImageBOHndl_run[CORES];

    //    DataCopyF_t mTileDataCopy;

    int imgSize() { return (mImageSize[0] * mImageSize[1] * mImageSize[2]); }

    int tileWindowSize(uint8_t channels) {
        return ((xf::cv::aie::METADATA_SIZE + (channels * TILE_HEIGHT_MAX * TILE_WIDTH_MAX * sizeof(DATA_TYPE))));
    }

    int tileImgSize(uint8_t channels) { return (tileWindowSize(channels) * (mTileRows * mTileCols)); }

    int bufferSizePerCore() { return (tileWindowSize(mchannels) * ((mTileRows * mTileCols) / CORES)); }

    // Helper function for Tiler copy {
    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == TILER)>::type* = nullptr>
    void input_copy(uint16_t startInd, uint16_t endInd) {
        assert(mpImgData);

        char* buffer = (mImageBOHndl.map<char*>());

        int tileSize = tileWindowSize(mchannels);
        for (int t = startInd; t < endInd; t++) {
            xf::cv::aie::metadata_elem_t* meta_data_p = (xf::cv::aie::metadata_elem_t*)(buffer + (t * tileSize));
            memset(meta_data_p, 0, xf::cv::aie::METADATA_SIZE);

            xf::cv::aie::xfSetTileWidth(meta_data_p, mMetaDataList[t].tileWidth());
            xf::cv::aie::xfSetTileHeight(meta_data_p, mMetaDataList[t].tileHeight());
            xf::cv::aie::xfSetTilePosH(meta_data_p, mMetaDataList[t].positionH());
            xf::cv::aie::xfSetTilePosV(meta_data_p, mMetaDataList[t].positionV());
            xf::cv::aie::xfSetTileOVLP_HL(meta_data_p, mMetaDataList[t].overlapSizeH_left());
            xf::cv::aie::xfSetTileOVLP_HR(meta_data_p, mMetaDataList[t].overlapSizeH_right());
            xf::cv::aie::xfSetTileOVLP_VT(meta_data_p, mMetaDataList[t].overlapSizeV_top());
            xf::cv::aie::xfSetTileOVLP_VB(meta_data_p, mMetaDataList[t].overlapSizeV_bottom());

            if (!mIsOutputResize) {
                xf::cv::aie::xfSetTileOutPosH(meta_data_p,
                                              mMetaDataList[t].overlapSizeH_left() + mMetaDataList[t].positionH());
                xf::cv::aie::xfSetTileOutPosV(meta_data_p,
                                              mMetaDataList[t].overlapSizeV_top() + mMetaDataList[t].positionV());
                xf::cv::aie::xfSetTileOutTWidth(
                    meta_data_p, mMetaDataList[t].tileWidth() -
                                     (mMetaDataList[t].overlapSizeH_left() + mMetaDataList[t].overlapSizeH_right()));
                xf::cv::aie::xfSetTileOutTHeight(
                    meta_data_p, mMetaDataList[t].tileHeight() -
                                     (mMetaDataList[t].overlapSizeV_top() + mMetaDataList[t].overlapSizeV_bottom()));
            } else if (isDynamicOutputResize == true) {
                xf::cv::aie::xfSetTileOutPosH(meta_data_p, mMetaDataList[t].outPositionH());
                xf::cv::aie::xfSetTileOutPosV(meta_data_p, mMetaDataList[t].outPositionV());
                xf::cv::aie::xfSetTileOutTWidth(meta_data_p, mOutSize.width);
                xf::cv::aie::xfSetTileOutTHeight(meta_data_p, mOutSize.height);
            } else {
                xf::cv::aie::xfSetTileOutPosH(meta_data_p, 0);
                xf::cv::aie::xfSetTileOutPosV(meta_data_p, t);
                xf::cv::aie::xfSetTileOutTWidth(meta_data_p, mOutSize.width);
                xf::cv::aie::xfSetTileOutTHeight(meta_data_p, 1);
            }
            DATA_TYPE* image_data_p = (DATA_TYPE*)xf::cv::aie::xfGetImgDataPtr(meta_data_p);
            int16_t tileHeight = mMetaDataList[t].tileHeight();
            int16_t tileWidth = mMetaDataList[t].tileWidth();
            int16_t positionV = mMetaDataList[t].positionV();
            int16_t positionH = mMetaDataList[t].positionH();

            for (int ti = 0; ti < tileHeight; ti++) {
                memcpy(image_data_p + (ti * (tileWidth * mchannels)),
                       mpImgData + (((positionV + ti) * (mImageSize[1] * mchannels)) + positionH * mchannels),
                       tileWidth * sizeof(DATA_TYPE) * mchannels);
            }
        }
    }
    // }

    // Tiler copy {
    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == TILER)>::type* = nullptr>
    void copy() {
        assert(mpImgData);

        uint16_t numThreads = std::thread::hardware_concurrency();

        std::thread mCopyThreads[numThreads];

        uint16_t tilesPerThread = (mTileRows * mTileCols) / numThreads;
        for (int i = 0; i < numThreads; i++) {
            uint16_t startInd = i * tilesPerThread;
            uint16_t endInd = (i == numThreads - 1) ? (mTileRows * mTileCols) : ((i + 1) * tilesPerThread);

            mCopyThreads[i] = std::thread(&xfcvDataMovers::input_copy, this, startInd, endInd);
        }
        for (int i = 0; i < numThreads; i++) {
            mCopyThreads[i].join();
        }
    }
    //}

    // Helper function for stitcher copy {
    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    void output_copy(uint16_t startInd, uint16_t endInd) {
        assert(mpImgData != nullptr);
        char* buffer = (mImageBOHndl.map<char*>());
        int tileSize = tileWindowSize(mchannels);
        for (int t = startInd; t < endInd; t++) {
            xf::cv::aie::metadata_elem_t* meta_data_p = (xf::cv::aie::metadata_elem_t*)(buffer + (t * tileSize));

            int16_t tileWidth = xf::cv::aie::xfGetTileWidth(meta_data_p);
            int16_t tileHeight = xf::cv::aie::xfGetTileHeight(meta_data_p);
            int16_t positionH = xf::cv::aie::xfGetTilePosH(meta_data_p);
            int16_t positionV = xf::cv::aie::xfGetTilePosV(meta_data_p);
            int16_t overlapSizeH_left = xf::cv::aie::xfGetTileOVLP_HL(meta_data_p);
            int16_t overlapSizeH_right = xf::cv::aie::xfGetTileOVLP_HR(meta_data_p);
            int16_t overlapSizeV_top = xf::cv::aie::xfGetTileOVLP_VT(meta_data_p);
            int16_t overlapSizeV_bottom = xf::cv::aie::xfGetTileOVLP_VB(meta_data_p);

            int16_t correctedPositionH = xf::cv::aie::xfGetTileOutPosH(meta_data_p);
            int16_t correctedPositionV = xf::cv::aie::xfGetTileOutPosV(meta_data_p);
            int16_t correctedTileWidth = xf::cv::aie::xfGetTileOutTWidth(meta_data_p);
            int16_t correctedTileHeight = xf::cv::aie::xfGetTileOutTHeight(meta_data_p);

            DATA_TYPE* image_data_p = (DATA_TYPE*)xf::cv::aie::xfGetImgDataPtr(meta_data_p);
            for (int ti = 0; ti < correctedTileHeight; ti++) {
                memcpy(mpImgData +
                           (((correctedPositionV + ti) * (mImageSize[1] * mchannels)) + correctedPositionH * mchannels),
                       image_data_p +
                           (((overlapSizeV_top + ti) * (tileWidth * mchannels)) + overlapSizeH_left * mchannels),
                       correctedTileWidth * sizeof(DATA_TYPE) * mchannels);
            }
        }
    }

    //}

    // Stitcher copy {
    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    void copy() {
        assert(mpImgData != nullptr);
        uint16_t numThreads = std::thread::hardware_concurrency();
        std::thread mCopyThreads[numThreads];

        uint16_t tilesPerThread = (mTileRows * mTileCols) / numThreads;
        for (int i = 0; i < numThreads; i++) {
            uint16_t startInd = i * tilesPerThread;
            uint16_t endInd = (i == numThreads - 1) ? (mTileRows * mTileCols) : ((i + 1) * tilesPerThread);
            mCopyThreads[i] = std::thread(&xfcvDataMovers::output_copy, this, startInd, endInd);
        }
        for (int i = 0; i < numThreads; i++) {
            mCopyThreads[i].join();
        }
    }
    //}

    void free_buffer() {
        if (bool(mImageBOHndl)) {
            mImageBOHndl = {};
        }
        mImageBOHndl = {};
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == TILER)>::type* = nullptr>
    void alloc_buffer() {
        if (!bool(mImageBOHndl)) {
            assert(tileImgSize(mchannels) > 0);
            std::cout << "Allocating image device buffer (Tiler), "
                      << " Size : " << tileImgSize(mchannels) << " bytes" << std::endl;
            mImageBOHndl = xrt::aie::bo(gpDhdl, (tileImgSize(mchannels)), xrt::bo::flags::normal, 0);
        }
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    void alloc_buffer() {
        if (!bool(mImageBOHndl)) {
            assert(tileImgSize(mchannels) > 0);
            std::cout << "Allocating image device buffer (Stitcher), "
                      << " Size : " << tileImgSize(mchannels) << " bytes" << std::endl;
            mImageBOHndl = xrt::aie::bo(gpDhdl, tileImgSize(mchannels), xrt::bo::flags::normal, 0);
        }
    }

    void regTilerStitcherCount() {
        if (KIND == TILER)
            ++gnTilerInstCount;
        else
            ++gnStitcherInstCount;
    }

   public:
    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == TILER)>::type* = nullptr>
    void start(std::array<std::string, CORES> portNames) {
        for (int i = 0; i < CORES; i++) {
            auto run = mImageBOHndl.async(portNames[i].c_str(), XCL_BO_SYNC_BO_GMIO_TO_AIE, bufferSizePerCore(),
                                          i * bufferSizePerCore());
            mImageBOHndl_run[i] = new xrt::aie::bo::async_handle(run);
        }
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == TILER)>::type* = nullptr>
    void wait(std::array<std::string, CORES> portNames) {
        for (int i = 0; i < CORES; i++) {
            mImageBOHndl_run[i]->wait();
        }
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    void start(std::array<std::string, CORES> portNames) {
        for (int i = 0; i < CORES; i++) {
            auto run = mImageBOHndl.async(portNames[i].c_str(), XCL_BO_SYNC_BO_AIE_TO_GMIO, bufferSizePerCore(),
                                          i * bufferSizePerCore());
            mImageBOHndl_run[i] = new xrt::aie::bo::async_handle(run);
        }
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    void wait(std::array<std::string, CORES> portNames) {
        for (int i = 0; i < CORES; i++) {
            mImageBOHndl_run[i]->wait();
        }

        // Copy data from device buffer to host
        copy();
        CtypeToCVMatType<DATA_TYPE> type;
        if (mpImage != nullptr) {
            cv::Mat dst(mImageSize[0], mImageSize[1], type.type, mpImgData);

            // TODO: saturation to be done based on the mat type ???
            if (mpImage->type() == CV_8U) {
                // Saturate the output values to [0,255]
                dst = cv::max(dst, 0);
                dst = cv::min(dst, 255);
            }
            dst.convertTo(*mpImage, mpImage->type());
        }
        mpImage = nullptr;
    }

    // Initialization / device buffer allocation / tile header copy / type
    // conversion to be done in constructor {
    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == TILER)>::type* = nullptr>
    xfcvDataMovers(uint16_t overlapH, uint16_t overlapV, uint8_t channels = 1) {
        // if (!bool(gpDhdl)) {
        //  throw std::runtime_error("No valid device handle found. Make sure using xF::deviceInit(...) is called.");
        //}

        mpImgData = nullptr;
        mImageSize = {0, 0, 0};
        mchannels = channels;
        // Initialize overlaps
        mOverlapH = overlapH;
        mOverlapV = overlapV;

        mTileRows = 0;
        mTileCols = 0;
        mbUserHndl = false;
        mTilerRGBtoRGBA = false;
        mStitcherRGBAtoRGB = false;

        mImageBOHndl = {};

        for (int i = 0; i < CORES; i++) {
            mImageBOHndl_run[i] = new xrt::aie::bo::async_handle({});
        }

        // Register the count of tiler/stitcher objects
        regTilerStitcherCount();
    }
    //}

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    xfcvDataMovers(uint8_t channels = 1) {
        // if (!bool(gpDhdl)) {
        //  throw std::runtime_error("No valid device handle found. Make sure using xF::deviceInit(...) is called.");
        //}

        mpImage = nullptr;
        mpImgData = nullptr;
        mImageSize = {0, 0, 0};
        mchannels = channels;

        // Initialize overlaps
        mOverlapH = 0;
        mOverlapV = 0;

        mTileRows = 0;
        mTileCols = 0;
        mbUserHndl = false;
        mTilerRGBtoRGBA = false;
        mStitcherRGBAtoRGB = false;

        mImageBOHndl = {};

        for (int i = 0; i < CORES; i++) {
            mImageBOHndl_run[i] = new xrt::aie::bo::async_handle({});
        }

        // Register the count of tiler/stitcher objects
        regTilerStitcherCount();
    }

    // Non copyable {
    xfcvDataMovers(const xfcvDataMovers&) = delete;
    xfcvDataMovers& operator=(const xfcvDataMovers&) = delete;
    //}

    //    void setTileCopyFn(DataCopyF_t& fn);

    // Close / free operations tp be done here {
    ~xfcvDataMovers() {
        free_buffer();
        // gpDhdl = nullptr;
    }
    //}

    void compute_metadata(const cv::Size& inImgSize,
                          const cv::Size& outImgSize = cv::Size(0, 0),
                          bool YorUV = false,
                          const int OUT_TILE_WIDTH_MAX = TILE_WIDTH_MAX,
                          const int OUT_TILE_HEIGHT_MAX = TILE_HEIGHT_MAX,
                          bool resize_bicubic = false,
                          bool sbm = false);

    // These functions will start the data transfer protocol {
    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == TILER)>::type* = nullptr>
    std::array<uint16_t, 2> host2aie_nb(xrt::bo imgHndl,
                                        const cv::Size& img_size,
                                        std::array<std::string, CORES> portNames,
                                        const xfcvDataMoverParams& params = xfcvDataMoverParams()) {
        int old_img_buffer_size = imgSize();

        if (params.mOutputImgSize == cv::Size(0, 0)) {
            bool bRecompute = false;
            if ((mImageSize[0] != img_size.height) || (mImageSize[1] != img_size.width)) {
                bRecompute = true;
            }

            mpImgData = imgHndl.map<DATA_TYPE*>();
            mImageSize = {(uint16_t)img_size.height, (uint16_t)img_size.width, (uint16_t)sizeof(DATA_TYPE) * mchannels};

            if (bRecompute == true) {
                // Pack metadata
                compute_metadata(img_size);
            }
        } else if (isDynamicOutputResize == true) {
            mpImgData = imgHndl.map<DATA_TYPE*>();
            mImageSize = {(uint16_t)img_size.height, (uint16_t)img_size.width, (uint16_t)sizeof(DATA_TYPE) * mchannels};
            // compute_metadata(img_size, params.mOutputImgSize);
        } else {
            mpImgData = imgHndl.map<DATA_TYPE*>();
            mImageSize = {(uint16_t)img_size.height, (uint16_t)img_size.width, (uint16_t)sizeof(DATA_TYPE) * mchannels};
            // compute_metadata(img_size, params.mOutputImgSize);
        }

        int new_img_buffer_size = imgSize();
        if ((new_img_buffer_size > old_img_buffer_size)) {
            free_buffer();
        }
        mbUserHndl = bool(imgHndl);
        if (mbUserHndl) mImageBOHndl = imgHndl;

        // Allocate buffer
        alloc_buffer();

        // Copy input data to device buffer
        copy();
        // Start the data transfers
        start(portNames);
        std::array<uint16_t, 2> ret = {mTileRows, mTileCols};

        return ret;
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == TILER)>::type* = nullptr>
    std::array<uint16_t, 2> host2aie_nb(xrt::bo* imgHndl,
                                        const cv::Size& img_size,
                                        std::array<std::string, CORES> portNames,
                                        const xfcvDataMoverParams& params = xfcvDataMoverParams()) {
        return host2aie_nb((*imgHndl), img_size, portNames, params);
    }

    // These functions will start the data transfer protocol {
    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == TILER)>::type* = nullptr>
    std::array<uint16_t, 2> host2aie_nb(DATA_TYPE* img_data,
                                        const cv::Size& img_size,
                                        std::array<std::string, CORES> portNames,
                                        const xfcvDataMoverParams& params = xfcvDataMoverParams()) {
        int old_img_buffer_size = imgSize();
        if (params.mOutputImgSize == cv::Size(0, 0)) {
            bool bRecompute = false;
            if ((mImageSize[0] != img_size.height) || (mImageSize[1] != img_size.width)) {
                bRecompute = true;
            }

            mpImgData = (DATA_TYPE*)img_data;
            mImageSize = {(uint16_t)img_size.height, (uint16_t)img_size.width, (uint16_t)sizeof(DATA_TYPE) * mchannels};

            if (bRecompute == true) {
                // Pack metadata
                compute_metadata(img_size);
            }
        } else {
            mpImgData = (DATA_TYPE*)img_data;
            mImageSize = {(uint16_t)img_size.height, (uint16_t)img_size.width, (uint16_t)sizeof(DATA_TYPE) * mchannels};
            // compute_metadata(img_size, params.mOutputImgSize);
        }

        int new_img_buffer_size = imgSize();

        if ((new_img_buffer_size > old_img_buffer_size)) {
            free_buffer();
        }

        // Allocate buffer
        alloc_buffer();

        // Copy input data to device buffer
        copy();

        // Start the data transfers
        start(portNames);

        std::array<uint16_t, 2> ret = {mTileRows, mTileCols};

        return ret;
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == TILER)>::type* = nullptr>
    std::array<uint16_t, 2> host2aie_nb(cv::Mat& img,
                                        std::array<std::string, CORES> portNames,
                                        const xfcvDataMoverParams& params = xfcvDataMoverParams()) {
        CtypeToCVMatType<DATA_TYPE> cType;
        if (cType.type == img.type()) {
            return host2aie_nb((DATA_TYPE*)img.data, img.size(), portNames, params);
        } else if (cType.type < img.type()) {
            cv::Mat temp;
            img.convertTo(temp, cType.type);
            return host2aie_nb((DATA_TYPE*)temp.data, img.size(), portNames, params);
        } else {
            std::vector<DATA_TYPE> imgData;
            imgData.assign(img.data, img.data + img.total());
            return host2aie_nb(imgData.data(), img.size(), portNames, params);
        }
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    void aie2host_nb(xrt::bo imgHndl,
                     const cv::Size& img_size,
                     std::array<uint16_t, 2> tiles,
                     std::array<std::string, CORES> portNames) {
        int old_img_buffer_size = imgSize();

        mpImgData = imgHndl.map<DATA_TYPE*>();
        mImageSize = {(uint16_t)img_size.height, (uint16_t)img_size.width, sizeof(DATA_TYPE) * mchannels};

        mTileRows = tiles[0];
        mTileCols = tiles[1];
        int new_img_buffer_size = imgSize();
        if ((new_img_buffer_size > old_img_buffer_size)) {
            free_buffer();
        }
        mbUserHndl = bool(imgHndl);
        if (mbUserHndl) mImageBOHndl = imgHndl;
        // Allocate buffer
        alloc_buffer();

        // Start the kernel
        start(portNames);
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    void aie2host_nb(xrt::bo* imgHndl,
                     const cv::Size& img_size,
                     std::array<uint16_t, 2> tiles,
                     std::array<std::string, CORES> portNames) {
        aie2host_nb((*imgHndl), img_size, tiles, portNames);
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    void aie2host_nb(DATA_TYPE* img_data,
                     const cv::Size& img_size,
                     std::array<uint16_t, 2> tiles,
                     std::array<std::string, CORES> portNames) {
        int old_img_buffer_size = imgSize();

        mpImgData = (DATA_TYPE*)img_data;
        mImageSize = {(uint16_t)img_size.height, (uint16_t)img_size.width, sizeof(DATA_TYPE) * mchannels};
        mTileRows = tiles[0];
        mTileCols = tiles[1];

        int new_img_buffer_size = imgSize();

        if ((new_img_buffer_size > old_img_buffer_size)) {
            free_buffer();
        }

        // Allocate buffer
        alloc_buffer();

        // Start the kernel
        start(portNames);
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    void aie2host_nb(cv::Mat& img, std::array<uint16_t, 2> tiles, std::array<std::string, CORES> portNames) {
        mpImage = &img;

        CtypeToCVMatType<DATA_TYPE> cType;

        if (cType.type == img.type()) {
            return aie2host_nb((DATA_TYPE*)img.data, img.size(), tiles, portNames);
        }

        DATA_TYPE* imgData = (DATA_TYPE*)malloc(img.size().height * img.size().width * sizeof(DATA_TYPE) * mchannels);

        aie2host_nb(imgData, img.size(), tiles, portNames);
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == TILER)>::type* = nullptr>
    std::array<uint16_t, 2> host2aie(cv::Mat& img,
                                     std::array<std::string, CORES> portNames,
                                     const xfcvDataMoverParams& params = xfcvDataMoverParams()) {
        std::array<uint16_t, 2> ret = host2aie_nb(img, portNames, params);

        wait(portNames);

        return ret;
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == TILER)>::type* = nullptr>
    std::array<uint16_t, 2> host2aie(DATA_TYPE* img_data,
                                     const cv::Size& img_size,
                                     std::array<std::string, CORES> portNames,
                                     const xfcvDataMoverParams& params = xfcvDataMoverParams()) {
        std::array<uint16_t, 2> ret = host2aie_nb(img_data, img_size, portNames, params);

        wait(portNames);

        return ret;
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    void aie2host(cv::Mat& img, std::array<uint16_t, 2> tiles, std::array<std::string, CORES> portNames) {
        aie2host_nb(img, tiles, portNames);

        wait(portNames);
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    void aie2host(DATA_TYPE* img_data,
                  const cv::Size& img_size,
                  std::array<uint16_t, 2> tiles,
                  std::array<std::string, CORES> portNames) {
        aie2host_nb(img_data, img_size, tiles, portNames);

        wait(portNames);
    }

    //}
};

template <DataMoverKind KIND,
          typename DATA_TYPE,
          int TILE_HEIGHT_MAX,
          int TILE_WIDTH_MAX,
          int AIE_VECTORIZATION_FACTOR,
          int CORES>
void xfcvDataMovers<KIND, DATA_TYPE, TILE_HEIGHT_MAX, TILE_WIDTH_MAX, AIE_VECTORIZATION_FACTOR, CORES, 0, true>::
    compute_metadata(const cv::Size& inImgSize,
                     const cv::Size& outImgSize,
                     bool YorUV,
                     const int OUT_TILE_WIDTH_MAX,
                     const int OUT_TILE_HEIGHT_MAX,
                     bool resize_bicubic,
                     bool sbm) {
    mMetaDataList.clear();

    cv::Size inputImgSize = inImgSize;
    cv::Size outputImgSize = outImgSize;

    // Set default output image size to input image size
    if (outputImgSize == cv::Size(0, 0)) {
        outputImgSize = inputImgSize;
    }

    mImageSize[0] = (uint16_t)inputImgSize.height;
    mImageSize[1] = (uint16_t)inputImgSize.width;

    if (inputImgSize.height == outputImgSize.height && inputImgSize.width == outputImgSize.width && sbm == false) {
        smartTileTilerGenerateMetaDataWithSpecifiedTileSize({inputImgSize.height, inputImgSize.width}, mMetaDataList,
                                                            mTileRows, mTileCols, {TILE_HEIGHT_MAX, TILE_WIDTH_MAX},
                                                            {mOverlapH, mOverlapH}, {mOverlapV, mOverlapV},
                                                            AIE_VECTORIZATION_FACTOR, true, false);
        mIsOutputResize = false;
        mOutSize = cv::Size(inputImgSize.height, inputImgSize.width);
    } else if (resize_bicubic == true) {
        smartTileTilerGenerateMetaDataWithSpecifiedTileSize(
            {inputImgSize.height, inputImgSize.width}, {outputImgSize.height, outputImgSize.width}, mMetaDataList,
            {TILE_HEIGHT_MAX, TILE_WIDTH_MAX}, {OUT_TILE_HEIGHT_MAX, OUT_TILE_WIDTH_MAX}, mTileRows, mTileCols,
            AIE_VECTORIZATION_FACTOR, YorUV, true, true);
        isDynamicOutputResize = true;
        mIsOutputResize = true;
        mOutSize = cv::Size(OUT_TILE_WIDTH_MAX, OUT_TILE_HEIGHT_MAX);
        std::cout << mOutSize.height << " " << mOutSize.width << std::endl;
    } else if (sbm == true) {
        smartTileTilerGenerateMetaDataWithSpecifiedTileSize(
            {inputImgSize.height, inputImgSize.width}, mMetaDataList, mTileRows, mTileCols,
            {TILE_HEIGHT_MAX, TILE_WIDTH_MAX}, {mOverlapH, 0}, {mOverlapV, 0}, AIE_VECTORIZATION_FACTOR, true, true);
    } else {
        smartTileTilerGenerateMetaDataWithSpecifiedTileSize({inputImgSize.height, inputImgSize.width},
                                                            {outputImgSize.height, outputImgSize.width}, mMetaDataList,
                                                            mTileRows, mTileCols, AIE_VECTORIZATION_FACTOR, true);
        mIsOutputResize = true;
        mOutSize = cv::Size(outputImgSize.height, outputImgSize.width);
        std::cout << outputImgSize.height << " " << outputImgSize.width << std::endl;
    }
    char sMesg[2048];
    sMesg[0] = '\0';
    sprintf(sMesg, "Requested tile size (%d,%d). Computed tile size (%d,%d). Number of tiles (%d,%d)\n",
            TILE_HEIGHT_MAX, TILE_WIDTH_MAX, mMetaDataList[0].tileHeight(), mMetaDataList[0].tileWidth(), mTileRows,
            mTileCols);
    std::cout << sMesg << std::endl;
}

} // xF

#endif
