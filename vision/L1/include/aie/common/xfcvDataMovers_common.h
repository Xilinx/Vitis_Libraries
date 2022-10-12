int xrtSyncBOAIENB(xrtDeviceHandle handle,
                   xrtBufferHandle bohdl,
                   const char* gmioName,
                   enum xclBOSyncDirection dir,
                   size_t size,
                   size_t offset);
int xrtGMIOWait(xrtDeviceHandle handle, const char* gmioName);

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

static xrtDeviceHandle gpDhdl = nullptr;
static std::vector<char> gHeader;
static const axlf* gpTop = nullptr;
static uint16_t gnTilerInstCount = 0;
static uint16_t gnStitcherInstCount = 0;

void deviceInit(const char* xclBin) {
    if (xclBin != nullptr) {
        if (gpDhdl == nullptr) {
            assert(gpTop == nullptr);

            gpDhdl = xrtDeviceOpen(0);
            if (gpDhdl == nullptr) {
                throw std::runtime_error("No valid device handle found. Make sure using right xclOpen index.");
            }

            std::ifstream stream(xclBin);
            stream.seekg(0, stream.end);
            size_t size = stream.tellg();
            stream.seekg(0, stream.beg);

            gHeader.resize(size);
            stream.read(gHeader.data(), size);

            gpTop = reinterpret_cast<const axlf*>(gHeader.data());
            if (xrtDeviceLoadXclbin(gpDhdl, gpTop)) {
                throw std::runtime_error("Xclbin loading failed");
            }

            adf::registerXRT(gpDhdl, gpTop->m_header.uuid);
        }
    }

    if (gpDhdl == nullptr) {
        throw std::runtime_error("No valid device handle found. Make sure using right xclOpen index.");
    }

    if (gpTop == nullptr) {
        throw std::runtime_error("Xclbin loading failed");
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
