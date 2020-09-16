#include "xlibz.hpp"
using namespace xlibz::driver;
extern singleton* sObj;
// Initialize the static member of the Singleton
singleton* singleton::instance = nullptr;

#ifdef ENABLE_XRM

// Initialize the static XRM context
xrmContext* singleton::ctx = nullptr;

#endif

// Private constructor
singleton::singleton(void) {}

// Provide the singleton instance
// Called in deflateInit2_
singleton* singleton::getInstance(void) {
    // If instance is not created
    // then create it
    if (instance == NULL) {
        instance = new singleton();
    }
    // Otherwise return the
    // existing instance
    return instance;
}

// Provde Driver object by singleton object
xzlib* singleton::getDriverInstance(z_streamp strm, uint8_t flow) {
    // Find current z_stream object if it exists in map
    auto iterator = driverMapObj.find(strm);
    if (iterator != driverMapObj.end()) {
        // If it exists the return
        return iterator->second;
    } else {
        // If not create it
        driver = new xzlib(strm, flow);
        // Insert the strm,xlz key/value pairs in map
        driverMapObj.insert(std::pair<z_streamp, xzlib*>(strm, driver));

        // Do prechecks
        driver->xilinxPreChecks();

        // return the object
        return driver;
    }
}

// Proivde xfZlib object by singleton object
xfZlib* singleton::getZlibInstance(z_streamp strm, std::string u50_xclbin, int flow) {
    // Find current z_stream object if it exists in map
    auto iterator = zlibMapObj.find(strm);
    if (iterator != zlibMapObj.end()) {
        // If it exists the return
        return iterator->second;
    } else {
        // In no-xrm flow by default device ID is 0
        uint8_t deviceId = 0;
#ifdef ENABLE_XRM
        // Pick Device ID
        deviceId = getXrmDeviceId();
#endif

        if (flow == XILINX_DEFLATE) {
            // If not create it
            xlz = new xfZlib(u50_xclbin.c_str(), 20, COMP_ONLY, deviceId, 0, FULL, XILINX_ZLIB);
            if (lz77KernelMap.empty()) {
                for (int i = 0; i < C_COMPUTE_UNIT; i++) {
                    auto cu_id = std::to_string(i + 1);
                    std::string lz77_compress_kname = xlz->getCompressKernel(0) + "_" + cu_id;
                    lz77KernelMap.insert(std::pair<std::string, int>(lz77_compress_kname, i));
                }
            }
        } else if (flow == XILINX_INFLATE) {
            // Construct xfZlib object
            xlz = new xfZlib(u50_xclbin.c_str(), 20, DECOMP_ONLY, deviceId, 0, FULL, XILINX_ZLIB);
            if (decKernelMap.empty()) {
                // Use map data structure to track the
                // Cu instance and CU number combination for
                // xrm generated CU allocation process
                for (int i = 0; i < D_COMPUTE_UNIT; i++) {
                    std::string cu_id = std::to_string(i + 1);
                    std::string decompress_kname = xlz->getDeCompressKernel(FULL) + "_" + cu_id;
                    decKernelMap.insert(std::pair<std::string, int>(decompress_kname, i));
                }
            }
        }
        // Insert the strm,xlz key/value pairs in map
        zlibMapObj.insert(std::pair<z_streamp, xfZlib*>(strm, xlz));
        // return the object
        return xlz;
    }
}

#ifdef ENABLE_XRM
// Proivde xfZlib object by singleton object
xrmContext* singleton::getXrmContext(void) {
    if (ctx == NULL) {
        ctx = (xrmContext*)xrmCreateContext(XRM_API_VERSION_1);
        if (ctx == NULL) {
            std::cout << "Failed to create XRM context " << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    return ctx;
}

// PRovide XRM Device ID
uint8_t singleton::getXrmDeviceId(void) {
    return deviceId;
}

// Provide XRM CU Instance
std::string singleton::getXrmCuInstance(const std::string& kernel_name) {
    if (instance_name.empty()) {
        std::string cu_id;
        //// Query XRM
        memset(&cuProp, 0, sizeof(xrmCuProperty));
        memset(&cuRes, 0, sizeof(xrmCuResource));

        // Assign kernel name to look for CU allocation
        strcpy(cuProp.kernelName, kernel_name.c_str());
        strcpy(cuProp.kernelAlias, "");

        // Open device in exclusive mode
        cuProp.devExcl = false;
        // % of load
        cuProp.requestLoad = 100;
        cuProp.poolId = 0;

        // Get XRM context
        xrmContext* lctx = getXrmContext();

        // Block until get exclusive access to the CU
        int ret = xrmCuBlockingAlloc(lctx, &cuProp, 2, &cuRes);
        if (ret != 0) {
            std::cout << "xrmCuAlloc: Failed to allocate CU \n" << std::endl;
            exit(EXIT_FAILURE);
        } else {
            // Return instance name
            cu_id = cuRes.instanceName;
            deviceId = cuRes.deviceId;
        }
        instance_name = cu_id;
    }
    return instance_name;
}

void singleton::releaseXrmCuInstance(void) {
    xrmContext* lctx = getXrmContext();
    instance_name.clear();
    xrmCuRelease(lctx, &cuRes);
}

#endif

// Release the xzlib by singleton object
void singleton::releaseDriverObj(z_streamp strm) {
    // Find if current z_stream exists in map
    auto iterator = driverMapObj.find(strm);
    if (iterator != driverMapObj.end()) {
        // If it does then pick the value entry
        // which is xfZlib object
        // Delete it
        delete iterator->second;
        // Remove <key,value> pair from map
        driverMapObj.erase(iterator);
    }
}

// Release the xfZlib object by singleton object
void singleton::releaseZlibObj(z_streamp strm) {
    // Find if current z_stream exists in map
    auto iterator = zlibMapObj.find(strm);
    if (iterator != zlibMapObj.end()) {
        // If it does then pick the value entry
        // which is xfZlib object
        // Delete it
        delete iterator->second;
        // Remove <key,value> pair from map
        zlibMapObj.erase(iterator);
    }
}

// Constructor
xzlib::xzlib(z_streamp strm, uint8_t flow) {
    struct_update(strm, flow);
}

// Structure update
void xzlib::struct_update(z_streamp strm, int flow) {
    m_info.in_buf = strm->next_in;
    m_info.out_buf = strm->next_out;
    m_info.input_size = strm->avail_in;
    m_info.output_size = strm->avail_out;
    m_info.adler32 = strm->adler;
    if (flow == XILINX_DEFLATE) {
        m_info.level = strm->state->level;
        m_info.strategy = strm->state->strategy;
        m_info.window_bits = strm->state->w_bits;
    }
    m_flow = flow;
}

// Get method for xMode
bool xzlib::getXmode(void) {
    return m_xMode;
}

// Get method for status
bool xzlib::getStatus(void) {
    return m_status;
}

// Class based approach
void xzlib::xilinxPreChecks(void) {
    // XILINX_NO_ACCEL -- CHECK
    // Check if user requesteed for No acceleration
    char* noaccel = getenv("XILINX_NO_ACCEL");
    if (noaccel && (std::stoi(noaccel) == 0)) {
// User requested no accel -- Return
#if (VERBOSE_LEVEL >= 1)
        std::cout << "[XZLIB]: User requested for No-Acceleration" << std::endl;
#endif
        m_xMode = false;
        return;
    }

#ifdef ENABLE_XRM
    xrmContext* lctx = (xrmContext*)xrmCreateContext(XRM_API_VERSION_1);
    if (lctx == NULL) {
#if (VERBOSE_LEVEL >= 1)
        std::cout << "Failed to create XRM Context " << std::endl;
#endif
        m_xMode = false;
        return;
    }

    bool ret = xrmIsDaemonRunning(lctx);
    if (ret == true) {
#if (VERBOSE_LEVEL >= 1)
        std::cout << "XRM Daemon is Running " << std::endl;
#endif
    } else {
#if (VERBOSE_LEVEL >= 1)
        std::cout << "XRM Daemon is Not Runnig " << std::endl;
#endif
        xrmDestroyContext(lctx);
        m_xMode = false;
        return;
    }

#endif

    // XCLBIN -- CHECK
    // By defalut try to pick XCLBIN from /opt area
    m_info.u50_xclbin = c_hardXclbinPath;

    // Check for ENT variable for XCLBIN if it doesnt
    // exist then use hardcoded path to pick
    char* xclbin = getenv("XILINX_LIBZ_XCLBIN");
    if (xclbin) m_info.u50_xclbin = xclbin;

    // Check if the XCLBIN file exist
    std::ifstream bin_file(m_info.u50_xclbin);
    m_xMode = bin_file.good() ? true : false;
    bin_file.close();

    // Fail to open XCLBIN -- Return
    if (!m_xMode) {
#if (VERBOSE_LEVEL >= 1)
        std::cout << "[XZLIB]: XCLBIN Not Found" << std::endl;
#endif
        return;
    }

    // MIN_INPUT_SIZE -- CHECK
    // First Check ENT variable

    // Deflate Min Input Size
    if (m_flow == XILINX_DEFLATE) {
        char* minBufSize = getenv("XILINX_LIBZ_DEFLATE_BUFFER_SIZE");
        if (minBufSize) m_minInSize = std::stoi(minBufSize);
    }

    // Inflate Min Input Size
    if (m_flow == XILINX_INFLATE) {
        char* minBufSize = getenv("XILINX_LIBZ_INFLATE_BUFFER_SIZE");
        if (minBufSize) m_minInSize = std::stoi(minBufSize);
    }

    // Compare with current input
    m_xMode = (m_info.input_size < m_minInSize) ? false : true;
    // Fail to meet minimum input size criteria -- Return
    if (!m_xMode) {
#if (VERBOSE_LEVEL >= 1)
        std::cout << "[XZLIB]: Minimum Input Size Failed " << std::endl;
#endif
        return;
    }

    // XILINX_XRT -- CHECK
    char* xilinx_xrt = getenv("XILINX_XRT");
    if (xilinx_xrt) {
        // Check if the XCLBIN file exist
        std::ifstream xrtpath(xilinx_xrt);
        // Check if XRT path provided by user is correct or not
        // If it is invalid proceed with CPU flow
        m_xMode = xrtpath.good() ? true : false;
        xrtpath.close();

        if (!m_xMode) {
#if (VERBOSE_LEVEL >= 1)
            std::cout << "[XZLIB]: User provided XRT path is Invalid " << std::endl;
#endif
            return;
        }
    } else {
        std::string hpath = "/opt/xilinx/xrt";
        // Ensure hardcoded path is correct first before setting
        std::ifstream xrtpath(hpath.c_str());
        if (xrtpath.good()) {
            setenv("XILINX_XRT", hpath.c_str(), 1);
        } else {
#if (VERBOSE_LEVEL >= 1)
            std::cout << "[XZLIB]: XRT is not installed at  " << hpath << std::endl;
#endif
            m_xMode = false;
            return;
        }
        xrtpath.close();
    }

    // XRT_INI Check
    char* xrtini = getenv("XRT_INI_PATH");
    if (xrtini) {
        // Check if the xrt.ini file exist
        std::ifstream xrtinipath(xrtini);
        m_xMode = xrtinipath.good() ? true : false;
        xrtinipath.close();
        if (!m_xMode) {
#if (VERBOSE_LEVEL >= 1)
            std::cout << "[XZLIB]: XRT INI Path is Invalid  " << std::endl;
#endif
            return;
        }
    } else {
        std::string hpath = "/opt/xilinx/zlib/xrt.ini";
        // Ensure the hardcoded path is correct
        std::ifstream xrtinifile(hpath.c_str());
        if (xrtinifile.good()) {
            setenv("XRT_INI_PATH", hpath.c_str(), 1);
        } else {
#if (VERBOSE_LEVEL >= 1)
            std::cout << "[XZLIB]: XRT INI is not found at " << hpath << std::endl;
#endif
            m_xMode = false;
            return;
        }
        xrtinifile.close();
    }
}

// Call to HW Deflate
bool xzlib::xilinxHwDeflate(z_streamp strm, int flush) {
    // Construct xfZlib object
    xfZlib* xlz = nullptr;

    // Retry flag in case unable to
    // access
    bool retry_flag = false;

    uint32_t adler32 = 0;
    size_t enbytes = 0;

    bool last_buffer = false;
    if (flush == Z_FINISH) last_buffer = true;

#ifndef ENABLE_XRM
    do {
        // Singleton provides zlib object if its already created
        // otherwise it will create and return the same
        xlz = sObj->getZlibInstance(strm, m_info.u50_xclbin);
        if (xlz->error_code()) {
            std::cout << "Failed to create object " << std::endl;
            retry_flag = true;
            sObj->releaseZlibObj(strm);
        } else {
            // Set adler value
            xlz->set_checksum(strm->adler);

            // Call compression_buffer API
            enbytes = xlz->compress_buffer(m_info.in_buf, m_info.out_buf, m_info.input_size, last_buffer);

            if (xlz->error_code()) {
#if (VERBOSE_LEVEL >= 2)
                std::cout << "Retrying .... " << std::endl;
#endif
                retry_flag = true;
                sObj->releaseZlibObj(strm);
            } else {
                retry_flag = false;
            }
            adler32 = xlz->get_checksum();
        }

    } while (retry_flag);
#else
    size_t host_buffer_size = HOST_BUFFER_SIZE;

    // Pick kernel name
    // xilDecompressFull
    std::string kernel_name = "xilLz77Compress";

    // Get XRM CU instance ID
    std::string cu_id = sObj->getXrmCuInstance(kernel_name);

    // Get ZLIB instance
    xlz = sObj->getZlibInstance(strm, m_info.u50_xclbin);

    auto lz77_iterator = sObj->lz77KernelMap.find(cu_id);
    int cu = 0;
    if (lz77_iterator != sObj->lz77KernelMap.end()) cu = lz77_iterator->second;

    // Call to decompress API
    enbytes = xlz->compress_buffer(m_info.in_buf, m_info.out_buf, m_info.input_size, last_buffer, host_buffer_size, cu);
    adler32 = xlz->get_checksum();
#endif

    if (last_buffer) {
        m_info.out_buf[enbytes] = adler32 >> 24;
        m_info.out_buf[enbytes + 1] = adler32 >> 16;
        m_info.out_buf[enbytes + 2] = adler32 >> 8;
        m_info.out_buf[enbytes + 3] = adler32;
        enbytes += 4;
    }

    strm->total_out += enbytes;
    strm->avail_out = strm->avail_out - enbytes;
    strm->avail_in = 0;
    strm->adler = adler32;
    strm->next_out += enbytes;

#if (VERBOSE_LEVEL >= 1)
    static int in = 0;
    static int out = 0;
    in += m_info.input_size;
    out += enbytes;
    float cr = in / out;
    openlog("XILINX_DEFLATE", LOG_PID, LOG_USER);
    syslog(LOG_INFO,
           "strm_ptr %d Input Chunk %d, Output Chunk %d Total Input %d Total Output %d Compression Ratio %f \n", strm,
           m_info.input_size, enbytes, in, out, cr);
    closelog();
#endif
    if (flush == Z_FINISH) {
        return true;
    } else
        return false;
}

// Call to HW Compress
uint64_t xzlib::xilinxHwCompress(z_streamp strm, int val) {
    // Construct xfZlib object
    xfZlib* xlz = nullptr;
    m_info.output_size = 0;

#ifndef ENABLE_XRM
    // Initialize checksum value
    bool retry_flag = false;
    int cu = 0;
    do {
        m_info.output_size = 0;
        // Singleton provides zlib object if its already created
        // otherwise it will create and return the same
        xlz = sObj->getZlibInstance(strm, m_info.u50_xclbin);
        int err_code = xlz->error_code();
        if (err_code) {
#if (VERBOSE_LEVEL >= 2)
            std::cout << "Failed to create object " << std::endl;
#endif
            retry_flag = true;
            sObj->releaseZlibObj(strm);
        } else {
            xlz->set_checksum(1);
            // Call to compress API
            m_info.output_size = xlz->compress(m_info.in_buf, m_info.out_buf, m_info.input_size, cu, m_info.level,
                                               m_info.strategy, m_info.window_bits);
            if (xlz->error_code()) {
#if (VERBOSE_LEVEL >= 2)
                std::cout << "Retrying .... " << std::endl;
#endif
                cu++;
                cu = cu % C_COMPUTE_UNIT;
                retry_flag = true;
                sObj->releaseZlibObj(strm);
            } else {
                retry_flag = false;
            }
        }
    } while (retry_flag);
#else
    // Pick kernel name
    // xilDecompressFull
    std::string kernel_name = "xilLz77Compress";

    // Get XRM CU instance ID
    std::string cu_id = sObj->getXrmCuInstance(kernel_name);

    // Get ZLIB instance
    xlz = sObj->getZlibInstance(strm, m_info.u50_xclbin);

    auto lz77_iterator = sObj->lz77KernelMap.find(cu_id);
    int cu = 0;
    if (lz77_iterator != sObj->lz77KernelMap.end()) {
        cu = lz77_iterator->second;
    }

    // Call to decompress API
    m_info.output_size = xlz->compress(m_info.in_buf, m_info.out_buf, m_info.input_size, cu, m_info.level,
                                       m_info.strategy, m_info.window_bits);
#endif

#if (VERBOSE_LEVEL >= 1)
    openlog("XILINX_COMPRESS2", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "strm_ptr %d Input Size %d, Compress Size %d \n", strm, m_info.input_size, m_info.output_size);
    closelog();
#endif

    return m_info.output_size;
}

// Call to HW DeCompress
uint64_t xzlib::xilinxHwDecompress(z_streamp strm) {
    xfZlib* xlz = nullptr;
    bool retry_flag = false;

#ifndef ENABLE_XRM
    int cu_id = 0;
    // Hack Random Generator / 256MB
    std::random_device rand_hw;
    std::uniform_int_distribution<> range(0, (D_COMPUTE_UNIT - 1));
    cu_id = range(rand_hw);
    do {
        xlz = sObj->getZlibInstance(strm, m_info.u50_xclbin, XILINX_INFLATE);
        int err_code = xlz->error_code();
        if (err_code && (err_code != c_clOutOfHostMemory)) {
#if (VERBOSE_LEVEL >= 2)
            std::cout << "Failed to create object " << std::endl;
#endif
            delete xlz;
        }

        // Call to decompress API
        m_info.output_size = xlz->decompress(m_info.in_buf, m_info.out_buf, m_info.input_size, cu_id);
        if (m_info.output_size == 0) {
#if (VERBOSE_LEVEL >= 2)
            std::cout << "Failed to create device buffer, retrying ... " << std::endl;
#endif
            cu_id++;
            cu_id = cu_id % D_COMPUTE_UNIT;
            retry_flag = true;
            sObj->releaseZlibObj(strm);
        } else {
            retry_flag = false;
        }
    } while (retry_flag);
#else
    // Pick kernel name
    // xilDecompressFull
    std::string kernel_name = "xilDecompressFull";

    // Get XRM CU instance ID
    std::string cu_id = sObj->getXrmCuInstance(kernel_name);

    xlz = sObj->getZlibInstance(strm, m_info.u50_xclbin, XILINX_INFLATE);

    int cu = 0;
    auto iterator = sObj->decKernelMap.find(cu_id);
    if (iterator != sObj->decKernelMap.end()) cu = iterator->second;

    // Call to decompress API
    m_info.output_size = xlz->decompress(m_info.in_buf, m_info.out_buf, m_info.input_size, cu);

#endif

#if (VERBOSE_LEVEL >= 1)
    openlog("XILINX_UNCOMPRESS", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "strm_ptr %d Input Size %d, UnCompress Size %d \n", strm, m_info.input_size, m_info.output_size);
    closelog();
#endif
    return m_info.output_size;
}
