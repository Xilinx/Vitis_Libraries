#include "zlib.hpp"
#include "deflate.h"
using namespace xf::compression;
#ifndef DEFLATE_BUFFER
#define DEFLATE_BUFFER (1024 * 1024)
#endif
enum m_enumMode { XILINX_DEFLATE = 1, XILINX_INFLATE };
class zlibDriver {
   public:
    bool xilinxHwDeflate(z_streamp s, int flush);
    uint64_t xilinxHwCompress(z_streamp s, int level);
    uint64_t xilinxHwUncompress(z_streamp s, int cu);
    zlibDriver(z_streamp s,
               const std::string& instance_name,
               const cl::Device& device_id,
               const cl_context_properties& platform_id,
               const std::string& m_u50_xclbin,
               int flow,
               const int bank_id);
    ~zlibDriver();
    bool getStatus();
    void struct_update(z_streamp s, int flow);
    bool getZlibInstance(const std::string& kinstance,
                         const cl::Device& device_id,
                         const cl_context_properties& platform_id,
                         const int bank_id);
    void releaseZlibObj(z_streamp strm);
    bool getErrStatus(void) { return m_status; }

   private:
    int m_flow = XILINX_DEFLATE;
    xfZlib* m_xlz = nullptr;
    int m_cuid = 0;
    std::string m_u50_xclbin;
    std::string m_kernel_instance = compress_kernel_names[1];
    xlibData m_info;
    bool m_status = false;
    uint32_t m_minInSize = 32 * 1024;
    unsigned char* m_deflateOutput;
    unsigned char* m_deflateInput;
    uint32_t m_outputSize = 0;
    uint32_t m_inputSize = 0;
    const uint32_t m_bufSize = DEFLATE_BUFFER;
    bool m_instCreated = false;
};
