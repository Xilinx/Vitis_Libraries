
template <DataMoverKind KIND,
          typename DATA_TYPE,
          int TILE_HEIGHT_MAX,
          int TILE_WIDTH_MAX,
          int AIE_VECTORIZATION_FACTOR,
          int CORES = 1,
          int PL_AXI_BITWIDTH = 16,
          bool USE_GMIO = false>
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
    std::array<xrtBufferHandle, CORES> mMetadataBOHndl;
    xrtBufferHandle mImageBOHndl;

    std::array<xrtKernelHandle, CORES> mPLKHandleArr;
    std::array<xrtRunHandle, CORES> mPLRHandleArr;

    int imgSize() { return (mImageSize[0] * mImageSize[1] * mImageSize[2]); }

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
                char* metadata_buffer = (char*)xrtBOMap(mMetadataBOHndl[i]);
                memcpy(metadata_buffer, MetaDataVecP, metadataSize(i));
                MetaDataVecP += metadataSize(i);
            }
        } else {
            // Column wise partitioning
            int r = 0;
            int c = 0;
            for (int i = 0; i < CORES; i++) {
                assert(mMetadataBOHndl[i]);
                char* metadata_buffer = (char*)xrtBOMap(mMetadataBOHndl[i]);
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
            void* buffer = xrtBOMap(mImageBOHndl);
            memcpy(buffer, mpImage->data, imgSize());
        }
    }
    //}

    // Stitcher copy {
    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    void copy() {
        // No meta-data
        assert(mImageBOHndl);

        void* buffer = xrtBOMap(mImageBOHndl);
        if (mbUserHndl == false) {
            assert(mpImage);
            memcpy(mpImage->data, buffer, imgSize());
        } else {
            xrtBOSync(mImageBOHndl, XCL_BO_SYNC_BO_TO_DEVICE, imgSize(), 0);
        }
    }
    //}

    void free_metadata_buffer(int core) {
        if (mMetadataBOHndl[core] != nullptr) {
            xrtBOFree(mMetadataBOHndl[core]);
        }
        mMetadataBOHndl[core] = nullptr;
        mMetadataSize[core] = 0;
    }

    void free_metadata_buffer() {
        for (int i = 0; i < CORES; i++) {
            free_metadata_buffer(i);
        }
    }

    void alloc_metadata_buffer() {
        for (int i = 0; i < CORES; i++) {
            if (mMetadataBOHndl[i] == nullptr) {
                // Update size
                mMetadataSize[i] = metaDataSizePerTile() * tilesPerCore(i);

                // Allocate buffer
                // assert(metadataSize(i) > 0);
                std::cout << "Allocating metadata device buffer (Tiler), "
                          << " Size : " << metadataSize(i) << " bytes" << std::endl;
                mMetadataBOHndl[i] = xrtBOAlloc(gpDhdl, metadataSize(i), 0, 0);
            }
        }
    }

    void free_buffer() {
        if (mbUserHndl == false) {
            if (mImageBOHndl != nullptr) {
                xrtBOFree(mImageBOHndl);
            }
            mImageBOHndl = nullptr;
        }
    }

    void alloc_buffer() {
        if (mImageBOHndl == nullptr) {
            assert(imgSize() > 0);
            std::cout << "Allocating image device buffer (Tiler), "
                      << " Size : " << imgSize() << " bytes" << std::endl;
            mImageBOHndl = xrtBOAlloc(gpDhdl, imgSize(), 0, 0);
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
            std::cout << "Loading kernel " << name.c_str() << std::endl;
            mPLKHandleArr[i] = xrtPLKernelOpen(gpDhdl, gpTop->m_header.uuid, name.c_str());
            mPLRHandleArr[i] = xrtRunOpen(mPLKHandleArr[i]);
        }
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == TILER)>::type* = nullptr>
    void setArgs() {
        std::cout << "Setting kernel args (Tiler) ..." << std::endl;
        for (int i = 0; i < CORES; i++) {
            uint16_t r = tileRowsPerCore(i);
            uint16_t c = tileColsPerCore(i);
            uint32_t NumTiles = r * c;
            (void)xrtRunSetArg(mPLRHandleArr[i], 0, mImageSize[1]); // Stride is same as Width
            (void)xrtRunSetArg(mPLRHandleArr[i], 1, NumTiles);      // Number of Tiles
            (void)xrtRunSetArg(mPLRHandleArr[i], 2, mImageBOHndl);
            (void)xrtRunSetArg(mPLRHandleArr[i], 3, mMetadataBOHndl[i]);
            (void)xrtRunSetArg(mPLRHandleArr[i], 4, mBurstLength);
            (void)xrtRunSetArg(mPLRHandleArr[i], 5, mTilerRGBtoRGBA);
        }
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    void setArgs() {
        std::cout << "Setting kernel args (Stitcher) ..." << std::endl;
        for (int i = 0; i < CORES; i++) {
            uint16_t r = tileRowsPerCore(i);
            uint16_t c = tileColsPerCore(i);
            uint32_t NumTiles = r * c;
            (void)xrtRunSetArg(mPLRHandleArr[i], 0, mImageSize[1]); // Width
            (void)xrtRunSetArg(mPLRHandleArr[i], 1, NumTiles);      // Number of Tiles
            (void)xrtRunSetArg(mPLRHandleArr[i], 2, mImageBOHndl);
            (void)xrtRunSetArg(mPLRHandleArr[i], 3, r); // Number of Tiles rows
            (void)xrtRunSetArg(mPLRHandleArr[i], 4, c); // Number of Tiles cols
            (void)xrtRunSetArg(mPLRHandleArr[i], 5, mStitcherRGBAtoRGB);
        }
    }

   public:
    void start() {
        for (auto& r : mPLRHandleArr) {
            xrtRunStart(r);
        }
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == TILER)>::type* = nullptr>
    xfcvDataMovers(uint16_t overlapH, uint16_t overlapV, bool tilerRGBtoRGBA = false, int burst = 2) {
        if (gpDhdl == nullptr) {
            throw std::runtime_error("No valid device handle found. Make sure using xF::deviceInit(...) is called.");
        }

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
            mMetadataBOHndl[i] = nullptr;
            mMetadataSize[i] = 0;
        }
        mImageBOHndl = nullptr;

        // Load the PL kernel
        load_krnl();
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    xfcvDataMovers(bool stitcherRGBAtoRGB = false, int burst = 2) {
        if (gpDhdl == nullptr) {
            throw std::runtime_error("No valid device handle found. Make sure using xF::deviceInit(...) is called.");
        }

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
            mMetadataBOHndl[i] = nullptr;
            mMetadataSize[i] = 0;
        }
        mImageBOHndl = nullptr;

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
            xrtRunClose(r);
        }
        for (auto& r : mPLKHandleArr) {
            xrtKernelClose(r);
        }
        if (gpDhdl != nullptr) {
            xrtDeviceClose(gpDhdl);
            gpDhdl = nullptr;
        }
    }
    //}

    void compute_metadata(const cv::Size& inImgSize, const cv::Size& outImgSize = cv::Size(0, 0));

    // These functions will start the data transfer protocol {
    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == TILER)>::type* = nullptr>
    auto host2aie_nb(cv::Mat& img, xrtBufferHandle imgHndl, const xfcvDataMoverParams& params) {
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
                compute_metadata(img.size());
            }
        } /* else {
             mpImage = &img;
             mImageSize = {(uint16_t)img.rows, (uint16_t)img.cols, (uint16_t)img.elemSize()};
             compute_metadata(img.size(), params.mOutputImgSize);
         }*/

        auto new_metadata_buffer_size = metadataSize();
        int new_img_buffer_size = imgSize();

        for (int i = 0; i < CORES; i++) {
            if (new_metadata_buffer_size[i] > old_metadata_buffer_size[i]) {
                free_metadata_buffer(i);
            }
        }

        if ((new_img_buffer_size > old_img_buffer_size) || (imgHndl != nullptr)) {
            free_buffer();
        }

        mbUserHndl = (imgHndl != nullptr);
        if (mbUserHndl) mImageBOHndl = imgHndl;

        // Allocate buffer
        alloc_metadata_buffer();
        alloc_buffer();

        // Copy input data to device buffer
        copy();

        // Set args
        setArgs();

        // Start the kernel
        start();

        std::array<uint16_t, 4> ret = {mTileRowsPerCore, mTileColsPerCore, mTileRows, mTileCols};
        return ret;
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == TILER)>::type* = nullptr>
    auto host2aie_nb(xrtBufferHandle imgHndl,
                     const cv::Size& size,
                     const xfcvDataMoverParams& params = xfcvDataMoverParams()) {
        cv::Mat img(size, CV_8UC1); // This image is redundant in case a handle is passed
        return host2aie_nb(img, imgHndl, params);
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    void aie2host_nb(cv::Mat& img, std::array<uint16_t, 4> tiles, xrtBufferHandle imgHndl = nullptr) {
        assert(sizeof(DATA_TYPE) >= img.elemSize());

        int old_img_buffer_size = imgSize();

        mpImage = &img;
        mImageSize = {(uint16_t)img.rows, (uint16_t)img.cols, (uint16_t)img.elemSize()};
        mTileRowsPerCore = tiles[0];
        mTileColsPerCore = tiles[1];
        mTileRows = tiles[2];
        mTileCols = tiles[3];

        int new_img_buffer_size = imgSize();
        if ((new_img_buffer_size > old_img_buffer_size) || (imgHndl != nullptr)) {
            free_buffer();
        }

        mbUserHndl = (imgHndl != nullptr);
        if (mbUserHndl) mImageBOHndl = imgHndl;

        // Allocate buffer
        alloc_buffer();

        // Set args
        setArgs();

        // Start the kernel
        start();
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    void aie2host_nb(xrtBufferHandle imgHndl, const cv::Size& size, std::array<uint16_t, 4> tiles) {
        cv::Mat img(size, CV_8UC1); // This image is redundant in case a handle is passed
        aie2host_nb(img, tiles, imgHndl);
    }
    //}

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == TILER)>::type* = nullptr>
    void wait() {
        for (auto& r : mPLRHandleArr) {
            (void)xrtRunWait(r);
        }
    }

    template <DataMoverKind _t = KIND, typename std::enable_if<(_t == STITCHER)>::type* = nullptr>
    void wait() {
        for (auto& r : mPLRHandleArr) {
            (void)xrtRunWait(r);
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
          bool USE_GMIO>
void xfcvDataMovers<KIND,
                    DATA_TYPE,
                    TILE_HEIGHT_MAX,
                    TILE_WIDTH_MAX,
                    AIE_VECTORIZATION_FACTOR,
                    CORES,
                    PL_AXI_BITWIDTH,
                    USE_GMIO>::compute_metadata(const cv::Size& inImgSize, const cv::Size& outImgSize) {
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
    if (inputImgSize.height == outputImgSize.height && inputImgSize.width == outputImgSize.width) {
        smartTileTilerGenerateMetaDataWithSpecifiedTileSize({inputImgSize.height, inputImgSize.width}, mMetaDataList,
                                                            mTileRows, mTileCols, {TILE_HEIGHT_MAX, TILE_WIDTH_MAX},
                                                            {mOverlapH, mOverlapH}, {mOverlapV, mOverlapV},
                                                            AIE_VECTORIZATION_FACTOR, true);
    } else {
        isOutputResize = true;
        smartTileTilerGenerateMetaDataWithSpecifiedTileSize({inputImgSize.height, inputImgSize.width},
                                                            {outputImgSize.height, outputImgSize.width}, mMetaDataList,
                                                            mTileRows, mTileCols, AIE_VECTORIZATION_FACTOR, true);
    }

    char sMesg[2048];
    sMesg[0] = '\0';
    sprintf(sMesg, "Requested tile size (%d,%d). Computed tile size (%d,%d). Number of tiles (%d,%d)\n",
            TILE_HEIGHT_MAX, TILE_WIDTH_MAX, mMetaDataList[0].tileHeight(), mMetaDataList[0].tileWidth(), mTileRows,
            mTileCols);
    std::cout << sMesg << std::endl;

    mTileRowsPerCore = (mTileRows + CORES - 1) / CORES;
    mTileColsPerCore = mTileCols;

    int OutOffset, InOffset, OutpositionV, OutpositionH, OutWidth, OutHeight;
    int InputImageStride = (int)inputImgSize.width;
    int OutputImageStride = (int)outputImgSize.width;

    int i = 0;
    for (auto& metaData : mMetaDataList) {
        if (isOutputResize == false) {
            OutpositionV = metaData.positionV() + metaData.overlapSizeV_top();
            OutpositionH = metaData.positionH() + metaData.overlapSizeH_left();
            OutWidth = (metaData.tileWidth() - (metaData.overlapSizeH_left() + metaData.overlapSizeH_right()));
            OutHeight = (metaData.tileHeight() - (metaData.overlapSizeV_top() + metaData.overlapSizeV_bottom()));
        } else {
            OutpositionV = i++;
            OutpositionH = 0;
            OutWidth = OutputImageStride;
            OutHeight = 1;
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
