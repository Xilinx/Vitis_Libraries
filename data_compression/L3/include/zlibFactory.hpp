#include "zlib.hpp"
#include "deflate.h"
#include "zlibDriver.hpp"
#ifdef ENABLE_XRM
#include <xrm.h>
#endif
using namespace xf::compression;

#ifdef ENABLE_XRM
typedef struct {
    xrmCuProperty cuProp;
    xrmCuResource cuRes;
} xrmStruct;

#endif
class zlibFactory {
   public:
    // Use getInstance of zlibFactory class
    // various requests through deflateInit
    // access zlibFactory instance
    static zlibFactory* getInstance();

    // Prechecks
    void xilinxPreChecks(void);

    // fpga/cpu mode
    bool getXmode();

    // Driver Instance
    zlibDriver* getDriverInstance(z_streamp strm, int flow);

    // Release Driver Instance
    void releaseDriverObj(z_streamp strm);

   private:
    // XCLBIN name
    std::string m_u50_xclbin;

    // Common platform / device
    // exists throughout the process
    cl_context_properties m_platform;
    cl::Device m_device;

    // Various opencl devices
    std::vector<cl::Device> m_ndevices;

    // Private constructor
    zlibFactory();

    // PreCheck flag --> FPGA/CPU
    bool m_xMode = true;

#ifdef ENABLE_XRM

    // XRM CU Instance
    xrmStruct* getXrmCuInstance(z_streamp strm, const std::string& kernel_name);

    // XRM Context
    static xrmContext* m_ctx;

    // Map data structure to hold
    // zstream struture pointer and xrmObject
    std::map<z_streamp, xrmStruct*> m_xrmMapObj;
#endif

    // std::map container to hold
    // zstream structure pointers and zlibDriver Class Object
    // Mapping (Driver Class)
    std::map<z_streamp, zlibDriver*> m_driverMapObj;

    std::mutex m_mutex;
    void lock() { m_mutex.lock(); }
    void unlock() { m_mutex.unlock(); }

    // Precheck status
    bool m_PreCheckStatus = true;
};
