#include "zlib.hpp"
#include "deflate.h"
using namespace xf::compression;
enum flow { XILINX_DEFLATE = 1, XILINX_INFLATE };

namespace xlibz {
namespace driver {
class xzlib {
   public:
    void xilinxPreChecks(void);
    bool xilinxHwDeflate(z_streamp s, int flush);
    uint64_t xilinxHwCompress(z_streamp s, int level);
    uint64_t xilinxHwDecompress(z_streamp s);
    xzlib(z_streamp s, uint8_t flow);
    bool getXmode();
    bool getStatus();
    void struct_update(z_streamp s, int flow);

   private:
    xlibData m_info;
    bool m_xMode = false;
    bool m_status = false;
    uint8_t m_flow = 0;
    uint32_t m_minInSize = 256 * 1024;
};

class singleton {
   public:
    // Use getInstance of singleton class
    // various requests through deflateInit
    // access singleton instance
    static singleton* getInstance();

    // Zlib instance
    xfZlib* getZlibInstance(z_streamp strm, std::string xclbin, int flow = XILINX_DEFLATE);

#ifdef ENABLE_XRM
    // XRM instance
    xrmContext* getXrmContext(void);

    // XRM CU Instance
    std::string getXrmCuInstance(const std::string& kernel_name);

    // XRM Device ID
    uint8_t getXrmDeviceId();

    // Release CU Instance
    void releaseXrmCuInstance(void);
#endif
    // Return current size of
    // Map data structure used to store ZLIB objects
    int getMapSize(void);

    // Release ZLIB object
    // Erase the map entry
    void releaseZlibObj(z_streamp strm);

    // Driver Instance
    xzlib* getDriverInstance(z_streamp strm, uint8_t flow);

    // Release Driver Instance
    void releaseDriverObj(z_streamp strm);

    // Map DS to track CU ID for compress
    std::map<std::string, int> lz77KernelMap;

    // Map DS to track CU ID for decompress
    std::map<std::string, int> decKernelMap;

   private:
    // Private constructor
    singleton();

    // Single Object instance of Singleton
    // Class, accessed by various clients within the
    // same process
    static singleton* instance;

    // Xilinx Zlib Object
    xfZlib* xlz;

    // Xilinx Driver Object
    xzlib* driver;

#ifdef ENABLE_XRM
    static xrmContext* ctx;
    xrmCuProperty cuProp;
    xrmCuResource cuRes;
    std::string instance_name;
    uint8_t deviceId;
#endif
    // std::map container to hold
    // zstream structure pointers and xfZlib Class Object
    // Mapping
    std::map<z_streamp, xfZlib*> zlibMapObj;

    // std::map container to hold
    // zstream structure pointers and xzlib Class Object
    // Mapping (Driver Class)
    std::map<z_streamp, xzlib*> driverMapObj;
};
}
}
