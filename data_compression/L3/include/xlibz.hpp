#include "zlib.hpp"
#include "deflate.h"
using namespace xf::compression;
enum flow { XILINX_DEFLATE = 1, XILINX_INFLATE };
#ifndef DEFLATE_BUFFER
#define DEFLATE_BUFFER 1024 * 1024
#endif
namespace xlibz {
namespace driver {
class xzlib {
   public:
    void xilinxPreChecks(void);
    bool xilinxHwDeflate(z_streamp s, int flush);
    uint64_t xilinxHwCompress(z_streamp s, int level);
    uint64_t xilinxHwDecompress(z_streamp s);
    xzlib(z_streamp s, uint8_t flow);
    ~xzlib();
    bool getXmode();
    bool getStatus();
    void struct_update(z_streamp s, int flow);
#ifdef ENABLE_XRM
    xrmCuProperty cuProp;
    xrmCuResource cuRes;
#endif
    std::string instance_name;
    uint8_t deviceId;

   private:
    xlibData m_info;
    bool m_xMode = false;
    bool m_status = false;
    uint8_t m_flow = 0;
    uint32_t m_minInSize = 32 * 1024;
    unsigned char* m_deflateOutput;
    unsigned char* m_deflateInput;
    uint32_t m_outputSize = 0;
    uint32_t m_inputSize = 0;
    const uint32_t m_bufSize = DEFLATE_BUFFER;
    bool m_instCreated = false;
    std::string cu_id;
};

class singleton {
   public:
    // Use getInstance of singleton class
    // various requests through deflateInit
    // access singleton instance
    static singleton* getInstance();

    // Zlib instance
    xfZlib* getZlibInstance(xzlib* driver, z_streamp strm, std::string xclbin, int flow = XILINX_DEFLATE);

#ifdef ENABLE_XRM
    // XRM instance
    xrmContext* getXrmContext(void) {
        lock();
        auto var = getXrmContext_p();
        unlock();
        return var;
    }

    // XRM CU Instance
    void getXrmCuInstance(xzlib* driver, const std::string& kernel_name);

    // XRM Device ID
    uint8_t getXrmDeviceId();
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

    // Xilinx Zlib Object
    xfZlib* xlz;

    // Xilinx Driver Object
    xzlib* driver;

#ifdef ENABLE_XRM
    static xrmContext* ctx;
#endif
    // std::map container to hold
    // zstream structure pointers and xfZlib Class Object
    // Mapping
    std::map<z_streamp, xfZlib*> zlibMapObj;

    // std::map container to hold
    // zstream structure pointers and xzlib Class Object
    // Mapping (Driver Class)
    std::map<z_streamp, xzlib*> driverMapObj;
    std::mutex m_mutex;
    void lock() { m_mutex.lock(); }
    void unlock() { m_mutex.unlock(); }
#ifdef ENABLE_XRM
    xrmContext* getXrmContext_p(void);
#endif
};
}
}
