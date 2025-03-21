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

#ifndef _XFCVDATAMOVERS_PLIO
#define _XFCVDATAMOVERS_PLIO

namespace xF {

template <DataMoverKind KIND,
          typename DATA_TYPE,
          int TILE_HEIGHT_MAX,
          int TILE_WIDTH_MAX,
          int AIE_VECTORIZATION_FACTOR,
          int CORES = 1,
          int PL_AXI_BITWIDTH = 16,
          bool USE_GMIO = false,
          bool RESIZE_1D = false>
class xfcvDataMovers {
   private:
    uint16_t mOverlapH;
    uint16_t mOverlapV;
    uint16_t mTileRowsPerCore;
    uint16_t mTileColsPerCore;
    uint16_t mTileRows;
    uint16_t mTileCols;
    bool mTilerRGBtoRGBA;
    bool mStitcherRGBAtoRGB;
    bool mbUserHndl;
    uint16_t mBurstLength;

    cv::Mat* mpImage;
    std::array<uint16_t, 3> mImageSize;

    std::vector<smartTileMetaData> mMetaDataList;
    std::vector<EmulAxiData<PL_AXI_BITWIDTH> > mMetaDataVec;

    std::array<int, CORES> mMetadataSize;
    // std::array<xrtBufferHandle, CORES> mMetadataBOHndl;
    // xrtBufferHandle mImageBOHndl;

    std::array<xrt::bo, CORES> mMetadataBOHndl;
    xrt::bo mImageBOHndl;

    std::array<xrt::kernel, CORES> mPLKHandleArr;
    std::array<xrt::run, CORES> mPLRHandleArr;

    xrt::kernel krnl;

    long int imgSize() { return (mImageSize[0] * mImageSize[1] * mImageSize[2]); }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == TILER)>::type* = nullptr>
    int metaDataSizePerTile() {
        return (mMetaDataVec.size() * sizeof(EmulAxiData<PL_AXI_BITWIDTH>)) / mMetaDataList.size();
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    int metaDataSizePerTile() {
        return 0;
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == TILER)>::type* = nullptr>
    int metadataSize(int core) {
        return mMetadataSize[core];
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    int metadataSize(int core) {
        return 0;
    }

    auto metadataSize() { return mMetadataSize; }

    // Tiler copy {
    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == TILER)>::type* = nullptr>
    void copy() {
        // Pack meta-data and image buffer in device buffer handle
        if (mTileColsPerCore == mTileCols) {
            // No column wise partition
            char* MetaDataVecP = (char*)mMetaDataVec.data();
            for (int i = 0; i < CORES; i++) {
                assert(mMetadataBOHndl[i]);
                // char* metadata_buffer = (char*)xrtBOMap(mMetadataBOHndl[i]); // C API
                char* metadata_buffer = (char*)(mMetadataBOHndl[i].map());
                memcpy(metadata_buffer, MetaDataVecP, metadataSize(i));
                MetaDataVecP += metadataSize(i);
                mMetadataBOHndl[i].sync(XCL_BO_SYNC_BO_TO_DEVICE);
                // std::cout << "Tiler copy done " << std::endl;
            }
        } else {
            // Column wise partitioning
            int r = 0;
            int c = 0;
            for (int i = 0; i < CORES; i++) {
                assert(mMetadataBOHndl[i]);
                // char* metadata_buffer = (char*)xrtBOMap(mMetadataBOHndl[i]);  //C API
                char* metadata_buffer = (char*)(mMetadataBOHndl[i].map());
                char* MetaDataVecP = ((char*)mMetaDataVec.data()) + (r * mTileCols + c) * metaDataSizePerTile();

                int x_r = tileRowsPerCore(i);
                int x_c = tileColsPerCore(i);

                int sz = x_c * metaDataSizePerTile();
                int stride = mTileCols * metaDataSizePerTile();
                for (int j = 0; j < x_r; j++) {
                    memcpy(metadata_buffer, MetaDataVecP, sz);
                    metadata_buffer += sz;
                    MetaDataVecP += stride;
                }

                c = c + mTileColsPerCore;
                if (c >= mTileCols) {
                    c = c - mTileCols;
                    r = (r + mTileRowsPerCore);
                }
            }
        }

        if (mbUserHndl == false) {
            assert(mpImage);
            assert(mImageBOHndl);
            // void* buffer = xrtBOMap(mImageBOHndl);  //C API
            void* buffer = mImageBOHndl.map();
            memcpy(buffer, mpImage->data, imgSize());
        } else {
            mImageBOHndl.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        }
    }
    //}

    // Stitcher copy {
    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    void copy() {
        // No meta-data
        assert(mImageBOHndl);

        // void* buffer = xrtBOMap(mImageBOHndl);  //C API
        void* buffer = mImageBOHndl.map();
        if (mbUserHndl == false) {
            assert(mpImage);
            memcpy(mpImage->data, buffer, imgSize());
        } else {
            // xrtBOSync(mImageBOHndl, XCL_BO_SYNC_BO_TO_DEVICE, imgSize(), 0); // C API
            mImageBOHndl.sync(XCL_BO_SYNC_BO_FROM_DEVICE, imgSize(), 0);
        }
    }
    //}

    void free_metadata_buffer(int core) {
        mMetadataBOHndl[core] = {};
        mMetadataSize[core] = 0;
    }

    void free_metadata_buffer() {
        for (int i = 0; i < CORES; i++) {
            free_metadata_buffer(i);
        }
    }

    void alloc_metadata_buffer() {
        for (int i = 0; i < CORES; i++) {
            if (!bool(mMetadataBOHndl[i])) {
                // Update size
                mMetadataSize[i] = metaDataSizePerTile() * tilesPerCore(i);

                // Allocate buffer
                // assert(metadataSize(i) > 0);
                std::cout << "Allocating metadata device buffer (Tiler), "
                          << " Size : " << metadataSize(i) << " bytes" << std::endl;
                // mMetadataBOHndl[i] = xrt::bo(gpDhdl, metadataSize(i), mPLKHandleArr[i].group_id(3));
                mMetadataBOHndl[i] = xrt::bo(gpDhdl, metadataSize(i), 0, 0);
            }
        }
    }

    void free_buffer() {
        if (mbUserHndl == false) {
            if (bool(mImageBOHndl)) { // != nullptr) {
                // xrtBOFree(mImageBOHndl);
                mImageBOHndl = {};
            }
            mImageBOHndl = {};
        }
    }

    void alloc_buffer() {
        if (!bool(mImageBOHndl)) { // == nullptr) {
            assert(imgSize() > 0);
            std::cout << "Allocating image device buffer (Tiler), "
                      << " Size : " << imgSize() << " bytes" << std::endl;
            mImageBOHndl = xrt::bo(gpDhdl, imgSize(), mPLKHandleArr[0].group_id(2));
        }
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == TILER)>::type* = nullptr>
    std::string krnl_inst_name(int n) {
        std::ostringstream ss;
        ss << "Tiler_top:{Tiler_top_" << n << "}";
        return ss.str();
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    std::string krnl_inst_name(int n) {
        std::ostringstream ss;
        ss << "stitcher_top:{stitcher_top_" << n << "}";
        return ss.str();
    }

    void load_krnl() {
        for (int i = 0; i < CORES; i++) {
            std::string name =
                (KIND == TILER) ? krnl_inst_name(++gnTilerInstCount) : krnl_inst_name(++gnStitcherInstCount);
            std::cout << ">>>>:> Loading kernel " << name.c_str() << std::endl;
            mPLKHandleArr[i] = xrt::kernel(gpDhdl, xclbin_uuid, name.c_str());
        }
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == TILER)>::type* = nullptr>
    void setArgs() {
        std::cout << "Setting kernel args (Tiler) ..." << std::endl;
        for (int i = 0; i < CORES; i++) {
            uint16_t r = tileRowsPerCore(i);
            uint16_t c = tileColsPerCore(i);
            uint32_t NumTiles = r * c;

            mPLRHandleArr[i] = mPLKHandleArr[i](mImageSize[1], NumTiles, mImageBOHndl, mMetadataBOHndl[i], mBurstLength,
                                                mTilerRGBtoRGBA); // Add mTilerRGBtoRGBA inplace of 0
        }
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    void setArgs() {
        std::cout << "Setting kernel args (Stitcher) ..." << std::endl;
        for (int i = 0; i < CORES; i++) {
            uint16_t r = tileRowsPerCore(i);
            uint16_t c = tileColsPerCore(i);
            uint32_t NumTiles = r * c;

            mPLRHandleArr[i] = mPLKHandleArr[i](mImageSize[1], NumTiles, mImageBOHndl, r, c, mStitcherRGBAtoRGB);
        }
    }

   public:
    void start() {
        for (auto& r : mPLRHandleArr) {
            r.start();
            // xrtRunStart(r);
        }
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == TILER)>::type* = nullptr>
    xfcvDataMovers(uint16_t overlapH, uint16_t overlapV, bool tilerRGBtoRGBA = false, int burst = 1) {
        // if (gpDhdl == nullptr) {
        //    throw std::runtime_error("No valid device handle found. Make sure using xF::deviceInit(...) is called.");
        // }

        mpImage = nullptr;
        mImageSize = {0, 0, 0};

        // Initialize overlaps
        mOverlapH = overlapH;
        mOverlapV = overlapV;
        mTilerRGBtoRGBA = tilerRGBtoRGBA;
        mBurstLength = burst;

        mTileRowsPerCore = 0;
        mTileColsPerCore = 0;
        mTileRows = 0;
        mTileCols = 0;
        mbUserHndl = false;
        for (int i = 0; i < CORES; i++) {
            mMetadataBOHndl[i] = {};
            mMetadataSize[i] = 0;
        }
        mImageBOHndl = {};
        // Load the PL kernel
        load_krnl();
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    xfcvDataMovers(bool stitcherRGBAtoRGB = false, int burst = 1) {
        // if (gpDhdl == nullptr) {
        //    throw std::runtime_error("No valid device handle found. Make sure using xF::deviceInit(...) is called.");
        // }
        mpImage = nullptr;
        mImageSize = {0, 0, 0};

        // Initialize overlaps
        mOverlapH = 0;
        mOverlapV = 0;
        mStitcherRGBAtoRGB = stitcherRGBAtoRGB;
        mBurstLength = burst;

        mTileRowsPerCore = 0;
        mTileColsPerCore = 0;
        mTileRows = 0;
        mTileCols = 0;
        mbUserHndl = false;

        for (int i = 0; i < CORES; i++) {
            mMetadataBOHndl[i] = {};
            mMetadataSize[i] = 0;
        }
        mImageBOHndl = {};

        // Load the PL kernel
        load_krnl();
    }

    // Non copyable {
    xfcvDataMovers(const xfcvDataMovers&) = delete;
    xfcvDataMovers& operator=(const xfcvDataMovers&) = delete;
    //}

    // Close / free operations tp be done here {
    ~xfcvDataMovers() {
        free_buffer();
        free_metadata_buffer();

        for (auto& r : mPLRHandleArr) {
            // xrtRunClose(r);
        }
        for (auto& r : mPLKHandleArr) {
            // xrtKernelClose(r);
        }
        if (bool(gpDhdl)) {
            // xrtDeviceClose(gpDhdl);
            // gpDhdl = {};
        }
    }
    //}

    // template <bool _t = RESIZE_1D, typename std::enable_if<(_t == true)>::type* = nullptr>
    template <bool T = RESIZE_1D, typename std::enable_if<T, int>::type = 0>
    void compute_metadata(const cv::Size& img_size,
                          const cv::Size& outImgSize = cv::Size(0, 0),
                          bool YorUV = false,
                          const int OUT_TILE_WIDTH_MAX = TILE_WIDTH_MAX,
                          const int OUT_TILE_HEIGHT_MAX = TILE_HEIGHT_MAX,
                          bool resize_bicubic = false);

    // template <bool _t = RESIZE_1D, typename std::enable_if<(_t == false)>::type* = nullptr>
    template <bool T = RESIZE_1D, typename std::enable_if<!T, int>::type = 0>
    void compute_metadata(const cv::Size& img_size,
                          const cv::Size& outImgSize = cv::Size(0, 0),
                          const cv::Size& orgImgSize = cv::Size(0, 0),
                          bool YorUV = false,
                          int crop_x = 0,
                          int crop_y = 0);

    // These functions will start the data transfer protocol {
    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == TILER)>::type* = nullptr>
    auto host2aie_nb(cv::Mat& img, xrt::bo imgHndl = {}, const xfcvDataMoverParams& params = xfcvDataMoverParams()) {
        assert(sizeof(DATA_TYPE) >= img.elemSize());

        auto old_metadata_buffer_size = metadataSize();
        int old_img_buffer_size = imgSize();

        if (params.mOutputImgSize == cv::Size(0, 0)) {
            bool bRecompute = false;
            if ((mImageSize[0] != img.rows) || (mImageSize[1] != img.cols)) {
                bRecompute = true;
            }

            mpImage = &img;
            mImageSize = {(uint16_t)img.rows, (uint16_t)img.cols, (uint16_t)img.elemSize()};
            if (bRecompute == true) {
                // Pack metadata
                compute_metadata(img.size(), img.size());
            }
        }

        auto new_metadata_buffer_size = metadataSize();
        int new_img_buffer_size = imgSize();

        for (int i = 0; i < CORES; i++) {
            if (new_metadata_buffer_size[i] > old_metadata_buffer_size[i]) {
                free_metadata_buffer(i);
            }
        }
        if ((new_img_buffer_size > old_img_buffer_size) || bool(imgHndl)) {
            free_buffer();
        }
        mbUserHndl = bool(imgHndl);

        if (mbUserHndl) mImageBOHndl = imgHndl;

        // Allocate buffer
        alloc_metadata_buffer();
        alloc_buffer();
        // Copy input data to device buffer
        copy();
        // Set args
        setArgs();
        // Start the kernel
        // start();

        std::array<uint16_t, 4> ret = {mTileRowsPerCore, mTileColsPerCore, mTileRows, mTileCols};
        return ret;
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == TILER)>::type* = nullptr>
    auto host2aie_nb(xrt::bo* imgHndl,
                     const cv::Size& size,
                     const xfcvDataMoverParams& params = xfcvDataMoverParams()) {
        cv::Mat img(size, CV_8UC1); // This image is redundant in case a handle is passed
        return host2aie_nb(img, (*imgHndl), params);
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    void aie2host_nb(cv::Mat& img, std::array<uint16_t, 4> tiles, xrt::bo imgHndl = {}) {
        assert(sizeof(DATA_TYPE) >= img.elemSize());

        int old_img_buffer_size = imgSize();

        mpImage = &img;
        mImageSize = {(uint16_t)img.rows, (uint16_t)img.cols, (uint16_t)img.elemSize()};
        mTileRowsPerCore = tiles[0];
        mTileColsPerCore = tiles[1];
        mTileRows = tiles[2];
        mTileCols = tiles[3];

        int new_img_buffer_size = imgSize();
        if ((new_img_buffer_size > old_img_buffer_size) || bool(imgHndl)) {
            free_buffer();
        }

        mbUserHndl = bool(imgHndl);
        if (mbUserHndl) mImageBOHndl = imgHndl;

        // Allocate buffer
        alloc_buffer();

        // Set args
        setArgs();
        // Start the kernel
        // start();
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    void aie2host_nb(xrt::bo* imgHndl, const cv::Size& size, std::array<uint16_t, 4> tiles) {
        cv::Mat img(size, CV_8UC1); // This image is redundant in case a handle is passed
        aie2host_nb(img, tiles, (*imgHndl));
    }
    //}

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == TILER)>::type* = nullptr>
    void wait() {
        for (auto& r : mPLRHandleArr) {
            (void)r.wait();
        }
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    void wait() {
        for (auto& r : mPLRHandleArr) {
            (void)r.wait();
        }

        // Copy data from device buffer to host
        copy();
    }

    uint16_t tilesPerCore() {
        if ((mTileRowsPerCore * mTileColsPerCore * CORES) != (mTileRows * mTileCols)) {
            std::cerr << "ERR: Tile rows distribution is not even across cores. Total number of generated tiles for "
                         "given resolution image and requested tile size is "
                      << "Rows(" << mTileRows << ") x Cols(" << mTileCols << ") and number of cores is " << CORES
                      << ". Please use core specific tile count function by passing core index of the corresponding "
                         "core as arguement value (eg. tilesPerCore(<core index>))."
                      << std::endl;
            exit(-1);
        }

        return (mTileRowsPerCore * mTileColsPerCore);
    }

    uint16_t tileColsPerCore(int core) {
        int hcore_distribution_factor = (mTileCols + mTileColsPerCore - 1) / mTileColsPerCore;
        int hidx = core % hcore_distribution_factor;

        uint16_t c_s = hidx * mTileColsPerCore;
        uint16_t c_e = std::min(mTileCols, uint16_t(c_s + mTileColsPerCore));

        return (c_e - c_s);
    }

    uint16_t tileRowsPerCore(int core) {
        int hcore_distribution_factor = (mTileCols + mTileColsPerCore - 1) / mTileColsPerCore;
        int vidx = core / hcore_distribution_factor;

        uint16_t r_s = vidx * mTileRowsPerCore;
        uint16_t r_e = std::min(mTileRows, uint16_t(r_s + mTileRowsPerCore));

        return (r_e - r_s);
    }

    uint16_t tilesPerCore(int core) {
        if (core >= CORES) {
            std::cerr << "ERR: Out of bound access. Trying to access " << core << " core in a " << CORES << " design."
                      << std::endl;
            exit(-1);
        }

        return tileRowsPerCore(core) * tileColsPerCore(core);
    }
};

template <DataMoverKind KIND,
          typename DATA_TYPE,
          int TILE_HEIGHT_MAX,
          int TILE_WIDTH_MAX,
          int AIE_VECTORIZATION_FACTOR,
          int CORES,
          int PL_AXI_BITWIDTH,
          bool USE_GMIO,
          bool RESIZE_1D>
template <bool T, typename std::enable_if<T, int>::type>
void xfcvDataMovers<KIND,
                    DATA_TYPE,
                    TILE_HEIGHT_MAX,
                    TILE_WIDTH_MAX,
                    AIE_VECTORIZATION_FACTOR,
                    CORES,
                    PL_AXI_BITWIDTH,
                    USE_GMIO,
                    RESIZE_1D>::compute_metadata(const cv::Size& inImgSize,
                                                 const cv::Size& outImgSize,
                                                 bool YorUV,
                                                 const int OUT_TILE_WIDTH_MAX,
                                                 const int OUT_TILE_HEIGHT_MAX,
                                                 bool resize_bicubic) {
    // std::cout << "OUT_TILE_WIDTH_MAX= " << OUT_TILE_WIDTH_MAX << "OUT_TILE_HEIGHT_MAX=" << OUT_TILE_HEIGHT_MAX <<
    // std::endl;
    mMetaDataList.clear();
    mMetaDataVec.clear();

    cv::Size inputImgSize = inImgSize;
    cv::Size outputImgSize = outImgSize;

    // Set default output image size to input image size
    if (outputImgSize == cv::Size(0, 0)) {
        outputImgSize = inputImgSize;
    }

    mImageSize[0] = (uint16_t)inputImgSize.height;
    mImageSize[1] = (uint16_t)inputImgSize.width;

    bool isOutputResize = false;
    bool isDynamicOutputResize = false;

    if (inputImgSize.height == outputImgSize.height && inputImgSize.width == outputImgSize.width) {
        printf("%d  %d   %d  %d\n", inputImgSize.width, inputImgSize.height, outputImgSize.width, outputImgSize.height);
        smartTileTilerGenerateMetaDataWithSpecifiedTileSize({inputImgSize.height, inputImgSize.width}, mMetaDataList,
                                                            mTileRows, mTileCols, {TILE_HEIGHT_MAX, TILE_WIDTH_MAX},
                                                            {mOverlapH, mOverlapH}, {mOverlapV, mOverlapV},
                                                            AIE_VECTORIZATION_FACTOR, true);
    } else {
        if (inputImgSize.width == TILE_WIDTH_MAX) {
            isOutputResize = true;
            smartTileTilerGenerateMetaDataWithSpecifiedTileSize(
                {inputImgSize.height, inputImgSize.width}, {outputImgSize.height, outputImgSize.width}, mMetaDataList,
                mTileRows, mTileCols, AIE_VECTORIZATION_FACTOR, false, true);
        } else if (resize_bicubic == true) {
            isDynamicOutputResize = true;
            smartTileTilerGenerateMetaDataWithSpecifiedTileSize(
                {inputImgSize.height, inputImgSize.width}, {outputImgSize.height, outputImgSize.width}, mMetaDataList,
                {TILE_HEIGHT_MAX, TILE_WIDTH_MAX}, {OUT_TILE_HEIGHT_MAX, OUT_TILE_WIDTH_MAX}, mTileRows, mTileCols,
                AIE_VECTORIZATION_FACTOR, YorUV, true, true);
        }
    }

    char sMesg[2048];
    sMesg[0] = '\0';
    sprintf(sMesg, "Requested tile size (%d,%d). Computed tile size (%d,%d). Number of tiles (%d,%d)\n",
            TILE_HEIGHT_MAX, TILE_WIDTH_MAX, mMetaDataList[0].tileHeight(), mMetaDataList[0].tileWidth(), mTileRows,
            mTileCols);
    std::cout << sMesg << std::endl;

    mTileRowsPerCore = (mTileRows + CORES - 1) / CORES;
    mTileColsPerCore = mTileCols;

    int OutOffset, InOffset, OutpositionV, OutpositionH, OutWidth, OutHeight, ImageStride;

    int InputImageStride = (int)inputImgSize.width;
    int OutputImageStride = (int)outputImgSize.width;

    int i = 0;
    int j = 0;
    std::ofstream metadata("metadata.txt");
    for (auto& metaData : mMetaDataList) {
        if (isOutputResize == true) {
            OutpositionV = i++;
            OutpositionH = 0;
            OutWidth = OutputImageStride;
            OutHeight = 1;
        } else if (isDynamicOutputResize == true) {
            OutpositionV = metaData.outPositionV();
            OutpositionH = metaData.outPositionH();
            OutWidth = metaData.finalWidth();
            OutHeight = metaData.finalHeight();
        } else {
            OutpositionV = metaData.positionV() + metaData.overlapSizeV_top();
            OutpositionH = metaData.positionH() + metaData.overlapSizeH_left();
            OutWidth = (metaData.tileWidth() - (metaData.overlapSizeH_left() + metaData.overlapSizeH_right()));
            OutHeight = (metaData.tileHeight() - (metaData.overlapSizeV_top() + metaData.overlapSizeV_bottom()));
        }
        OutOffset = ((OutpositionV * OutputImageStride) + OutpositionH);
        InOffset = ((metaData.positionV() * InputImageStride) + metaData.positionH());

        mMetaDataVec.emplace_back((int16_t)(InOffset & 0x0000ffff));
        mMetaDataVec.emplace_back((int16_t)(InOffset >> 16));
        mMetaDataVec.emplace_back((int16_t)metaData.tileWidth());
        mMetaDataVec.emplace_back((int16_t)metaData.tileHeight());
        mMetaDataVec.emplace_back((int16_t)metaData.overlapSizeH_left());
        mMetaDataVec.emplace_back((int16_t)metaData.overlapSizeH_right());
        mMetaDataVec.emplace_back((int16_t)metaData.overlapSizeV_top());
        mMetaDataVec.emplace_back((int16_t)metaData.overlapSizeV_bottom());
        mMetaDataVec.emplace_back((int16_t)(OutOffset & 0x0000ffff));
        mMetaDataVec.emplace_back((int16_t)(OutOffset >> 16));
        mMetaDataVec.emplace_back((int16_t)OutWidth);
        mMetaDataVec.emplace_back((int16_t)OutHeight);
        mMetaDataVec.emplace_back((int16_t)0); // Enable saturation, 1: 8U, 2: 8S
        mMetaDataVec.emplace_back((int16_t)OutpositionH);
        mMetaDataVec.emplace_back((int16_t)OutpositionV);
        mMetaDataVec.emplace_back((int16_t)metaData.positionH()); // In PosH
        mMetaDataVec.emplace_back((int16_t)metaData.positionV()); // In PosV
        mMetaDataVec.emplace_back((int16_t)16);                   // BIT_WIDTH
        mMetaDataVec.emplace_back((int16_t)OutputImageStride);
        mMetaDataVec.emplace_back((int16_t)0);
        mMetaDataVec.emplace_back((int16_t)0);
        mMetaDataVec.emplace_back((int16_t)0);
        mMetaDataVec.emplace_back((int16_t)0);
        mMetaDataVec.emplace_back((int16_t)0);
        mMetaDataVec.emplace_back((int16_t)0);
        mMetaDataVec.emplace_back((int16_t)0);
        mMetaDataVec.emplace_back((int16_t)0);
        mMetaDataVec.emplace_back((int16_t)0);
        mMetaDataVec.emplace_back((int16_t)0);
        mMetaDataVec.emplace_back((int16_t)0);
        mMetaDataVec.emplace_back((int16_t)0);
        mMetaDataVec.emplace_back((int16_t)0);
    }
}

template <DataMoverKind KIND,
          typename DATA_TYPE,
          int TILE_HEIGHT_MAX,
          int TILE_WIDTH_MAX,
          int AIE_VECTORIZATION_FACTOR,
          int CORES,
          int PL_AXI_BITWIDTH,
          bool USE_GMIO,
          bool RESIZE_1D>
template <bool T, typename std::enable_if<!T, int>::type>
void xfcvDataMovers<KIND,
                    DATA_TYPE,
                    TILE_HEIGHT_MAX,
                    TILE_WIDTH_MAX,
                    AIE_VECTORIZATION_FACTOR,
                    CORES,
                    PL_AXI_BITWIDTH,
                    USE_GMIO,
                    RESIZE_1D>::compute_metadata(const cv::Size& inImgSize,
                                                 const cv::Size& outImgSize,
                                                 const cv::Size& orgImgSize,
                                                 bool YorUV,
                                                 int crop_x,
                                                 int crop_y) {
    // std::cout << "OUT_TILE_WIDTH_MAX= " << OUT_TILE_WIDTH_MAX << "OUT_TILE_HEIGHT_MAX=" << OUT_TILE_HEIGHT_MAX <<
    // std::endl;
    mMetaDataList.clear();
    mMetaDataVec.clear();

    cv::Size inputImgSize = inImgSize;
    cv::Size outputImgSize = outImgSize;
    cv::Size originalImgSize = orgImgSize;

    // Set default output image size to input image size
    if (outputImgSize == cv::Size(0, 0)) {
        outputImgSize = inputImgSize;
    }

    if (originalImgSize == cv::Size(0, 0)) {
        originalImgSize = inputImgSize;
    }

    mImageSize[0] = (uint16_t)inputImgSize.height;
    mImageSize[1] = (uint16_t)inputImgSize.width;

    bool isOutputResize = false;
    bool isDynamicOutputResize = false;

    if (inputImgSize.height == outputImgSize.height && inputImgSize.width == outputImgSize.width) {
        smartTileTilerGenerateMetaDataWithSpecifiedTileSize({inputImgSize.height, inputImgSize.width}, mMetaDataList,
                                                            mTileRows, mTileCols, {TILE_HEIGHT_MAX, TILE_WIDTH_MAX},
                                                            {mOverlapH, mOverlapH}, {mOverlapV, mOverlapV},
                                                            AIE_VECTORIZATION_FACTOR, true);
    } else {
        if (inputImgSize.width == TILE_WIDTH_MAX) {
            isOutputResize = true;
            smartTileTilerGenerateMetaDataWithSpecifiedTileSize(
                {inputImgSize.height, inputImgSize.width}, {outputImgSize.height, outputImgSize.width}, mMetaDataList,
                mTileRows, mTileCols, AIE_VECTORIZATION_FACTOR, YorUV, true);
        }
    }

    char sMesg[2048];
    sMesg[0] = '\0';
    sprintf(sMesg, "Requested tile size (%d,%d). Computed tile size (%d,%d). Number of tiles (%d,%d)\n",
            TILE_HEIGHT_MAX, TILE_WIDTH_MAX, mMetaDataList[0].tileHeight(), mMetaDataList[0].tileWidth(), mTileRows,
            mTileCols);
    std::cout << sMesg << std::endl;

    mTileRowsPerCore = (mTileRows + CORES - 1) / CORES;
    mTileColsPerCore = mTileCols;

    int OutOffset, InOffset, OutpositionV, OutpositionH, OutWidth, OutHeight, ImageStride;

    int InputImageStride = (int)originalImgSize.width;
    int OutputImageStride = (int)outputImgSize.width;

    int i = 0;
    int j = 0;
    for (auto& metaData : mMetaDataList) {
        if (isOutputResize == true) {
            OutpositionV = i++;
            OutpositionH = 0;
            OutWidth = OutputImageStride;
            OutHeight = 1;
        } else if (isDynamicOutputResize == true) {
            OutpositionV = metaData.outPositionV();
            OutpositionH = metaData.outPositionH();
            OutWidth = metaData.finalWidth();
            OutHeight = metaData.finalHeight();
        } else {
            OutpositionV = metaData.positionV() + metaData.overlapSizeV_top();
            OutpositionH = metaData.positionH() + metaData.overlapSizeH_left();
            OutWidth = (metaData.tileWidth() - (metaData.overlapSizeH_left() + metaData.overlapSizeH_right()));
            OutHeight = (metaData.tileHeight() - (metaData.overlapSizeV_top() + metaData.overlapSizeV_bottom()));
        }
        OutOffset = ((OutpositionV * OutputImageStride) + OutpositionH);
        // InOffset = ((metaData.positionV() * InputImageStride) + metaData.positionH());
        InOffset = (((crop_y + metaData.positionV()) * InputImageStride) + (metaData.positionH() + crop_x));

        mMetaDataVec.emplace_back((int16_t)(InOffset & 0x0000ffff));
        mMetaDataVec.emplace_back((int16_t)(InOffset >> 16));
        mMetaDataVec.emplace_back((int16_t)metaData.tileWidth());
        mMetaDataVec.emplace_back((int16_t)metaData.tileHeight());
        mMetaDataVec.emplace_back((int16_t)metaData.overlapSizeH_left());
        mMetaDataVec.emplace_back((int16_t)metaData.overlapSizeH_right());
        mMetaDataVec.emplace_back((int16_t)metaData.overlapSizeV_top());
        mMetaDataVec.emplace_back((int16_t)metaData.overlapSizeV_bottom());
        mMetaDataVec.emplace_back((int16_t)(OutOffset & 0x0000ffff));
        mMetaDataVec.emplace_back((int16_t)(OutOffset >> 16));
        mMetaDataVec.emplace_back((int16_t)OutWidth);
        mMetaDataVec.emplace_back((int16_t)OutHeight);
        mMetaDataVec.emplace_back((int16_t)0); // Enable saturation, 1: 8U, 2: 8S
        mMetaDataVec.emplace_back((int16_t)OutpositionH);
        mMetaDataVec.emplace_back((int16_t)OutpositionV);
        mMetaDataVec.emplace_back((int16_t)metaData.positionH()); // In PosH
        mMetaDataVec.emplace_back((int16_t)metaData.positionV()); // In PosV
        mMetaDataVec.emplace_back((int16_t)16);                   // BIT_WIDTH
        mMetaDataVec.emplace_back((int16_t)OutputImageStride);
        mMetaDataVec.emplace_back((int16_t)0);
        mMetaDataVec.emplace_back((int16_t)0);
        mMetaDataVec.emplace_back((int16_t)0);
        mMetaDataVec.emplace_back((int16_t)0);
        mMetaDataVec.emplace_back((int16_t)0);
        mMetaDataVec.emplace_back((int16_t)0);
        mMetaDataVec.emplace_back((int16_t)0);
        mMetaDataVec.emplace_back((int16_t)0);
        mMetaDataVec.emplace_back((int16_t)0);
        mMetaDataVec.emplace_back((int16_t)0);
        mMetaDataVec.emplace_back((int16_t)0);
        mMetaDataVec.emplace_back((int16_t)0);
        mMetaDataVec.emplace_back((int16_t)0);
    }
}
} // xF

#endif
