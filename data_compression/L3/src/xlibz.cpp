#include "xlibz.hpp"
using namespace xlibz::driver;

#ifdef ENABLE_XRM

// Initialize the static XRM context
xrmContext* singleton::ctx = nullptr;

#endif

// Private constructor
singleton::singleton(void) {}

// Provide the singleton instance
// Called in deflateInit2_
singleton* singleton::getInstance(void) {
    static singleton instance;
    return &instance;
}

// Provde Driver object by singleton object
xzlib* singleton::getDriverInstance(z_streamp strm, uint8_t flow) {
    lock();
    // Find current z_stream object if it exists in map
    auto iterator = driverMapObj.find(strm);
    if (iterator != driverMapObj.end()) {
        unlock();
        // If it exists the return
        return iterator->second;
    } else {
        // If not create it
        driver = new xzlib(strm, flow);
        // Insert the strm,xlz key/value pairs in map
        driverMapObj.insert(std::pair<z_streamp, xzlib*>(strm, driver));

        // Do prechecks
        driver->xilinxPreChecks();
        unlock();
        // return the object
        return driver;
    }
}

// Proivde xfZlib object by singleton object
xfZlib* singleton::getZlibInstance(xzlib* driver, z_streamp strm, std::string u50_xclbin, int flow) {
    lock();
    // Find current z_stream object if it exists in map
    auto iterator = zlibMapObj.find(strm);
    if (iterator != zlibMapObj.end()) {
        // If it exists the return
        unlock();
        return iterator->second;
    } else {
        // In no-xrm flow by default device ID is 0
        uint8_t deviceId = 0;
#ifdef ENABLE_XRM
        // Pick Device ID
        deviceId = driver->deviceId;
#endif

        if (flow == XILINX_DEFLATE) {
            // If not create it
            xlz = new xfZlib(u50_xclbin.c_str(), 0, COMP_ONLY, deviceId, 0, FULL, XILINX_ZLIB);
            if (lz77KernelMap.empty()) {
                for (int i = 0; i < C_COMPUTE_UNIT; i++) {
                    auto cu_id = std::to_string(i + 1);
                    std::string lz77_compress_kname = xlz->getCompressKernel(1) + "_" + cu_id;
                    lz77KernelMap.insert(std::pair<std::string, int>(lz77_compress_kname, i));
                }
            }
        } else if (flow == XILINX_INFLATE) {
            // Construct xfZlib object
            xlz = new xfZlib(u50_xclbin.c_str(), 0, DECOMP_ONLY, deviceId, 0, FULL, XILINX_ZLIB);
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
        unlock();
        // return the object
        return xlz;
    }
}

#ifdef ENABLE_XRM
// Proivde xfZlib object by singleton object
xrmContext* singleton::getXrmContext_p(void) {
    if (ctx == NULL) {
        ctx = (xrmContext*)xrmCreateContext(XRM_API_VERSION_1);
        if (ctx == NULL) {
            std::cout << "Failed to create XRM context " << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    return ctx;
}

// Provide XRM CU Instance
void singleton::getXrmCuInstance(xzlib* driver, const std::string& kernel_name) {
    lock();
    if (driver == NULL) {
        unlock();
        return;
    }
    if (driver->instance_name.empty()) {
        std::string cu_id;
        //// Query XRM
        memset(&(driver->cuProp), 0, sizeof(xrmCuProperty));
        memset(&(driver->cuRes), 0, sizeof(xrmCuResource));

        // Assign kernel name to look for CU allocation
        strncpy(driver->cuProp.kernelName, kernel_name.c_str(), kernel_name.size() + 1);
        strcpy(driver->cuProp.kernelAlias, "");

        // Open device in exclusive mode
        driver->cuProp.devExcl = false;
        // % of load
        driver->cuProp.requestLoad = 100;
        driver->cuProp.poolId = 0;

        // Get XRM context
        xrmContext* lctx = getXrmContext_p();

        // Block until get exclusive access to the CU
        int ret = xrmCuBlockingAlloc(lctx, &(driver->cuProp), 2, &(driver->cuRes));
        if (ret != 0) {
            std::cout << "xrmCuAlloc: Failed to allocate CU \n" << std::endl;
            unlock();
            exit(EXIT_FAILURE);
        } else {
            // Return instance name
            cu_id = driver->cuRes.instanceName;
            driver->deviceId = driver->cuRes.deviceId;
        }
        driver->instance_name = cu_id;
    }
    unlock();
    return;
}

#endif

// Release the xzlib by singleton object
void singleton::releaseDriverObj(z_streamp strm) {
    lock();
    // Find if current z_stream exists in map
    auto iterator = driverMapObj.find(strm);
    if (iterator != driverMapObj.end()) {
// If it does then pick the value entry
// which is xfZlib object
// Delete it
#ifdef ENABLE_XRM
        xrmContext* lctx = getXrmContext_p();
        if (lctx != NULL) xrmCuRelease(lctx, &(iterator->second->cuRes));
#endif
        delete iterator->second;
        // Remove <key,value> pair from map
        driverMapObj.erase(iterator);
    }
    unlock();
}

// Release the xfZlib object by singleton object
void singleton::releaseZlibObj(z_streamp strm) {
    lock();
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
    unlock();
}

// Constructor
xzlib::xzlib(z_streamp strm, uint8_t flow) {
    struct_update(strm, flow);

    m_deflateOutput = new unsigned char[2 * m_bufSize];
    m_deflateInput = new unsigned char[2 * m_bufSize];
}

// destructor
xzlib::~xzlib() {
    if (m_deflateOutput) {
        delete m_deflateOutput;
    }

    if (m_deflateInput) {
        delete m_deflateInput;
    }

    instance_name.clear();
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
        std::string hpath = "/opt/xilinx/apps/zlib/scripts/xrt.ini";
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

    bool last_data = false;

    // Pick kernel name
    // xilDecompressFull
    if (m_instCreated == false) {
        std::string kernel_name = "xilZlibCompressFull";
        std::string cu_number = "1";

        cu_id = kernel_name + "_" + cu_number;

#ifdef ENABLE_XRM
        // Get XRM CU instance ID
        singleton::getInstance()->getXrmCuInstance(this, kernel_name);
        cu_id = this->instance_name;
#endif
        m_instCreated = true;
    }

    xlz = singleton::getInstance()->getZlibInstance(this, strm, m_info.u50_xclbin);

    while ((m_info.input_size != 0 || flush) && (strm->avail_out != 0)) {
        bool last_buffer = false;
        if (m_info.input_size + m_inputSize >= m_bufSize) {
            std::memcpy(m_deflateInput + m_inputSize, m_info.in_buf, m_bufSize - m_inputSize);
            m_info.in_buf += (m_bufSize - m_inputSize);
            m_info.input_size -= (m_bufSize - m_inputSize);
            m_inputSize += (m_bufSize - m_inputSize);
        } else if (m_info.input_size != 0) {
            std::memcpy(m_deflateInput + m_inputSize, m_info.in_buf, m_info.input_size);
            m_inputSize += m_info.input_size;
            m_info.input_size = 0;
        }
        if (flush != Z_NO_FLUSH && m_info.input_size == 0) last_buffer = true;
        // Set adler value
        xlz->set_checksum(strm->adler);
        size_t inputSize = m_inputSize;
        // Call compression_buffer API
        if (last_buffer && (m_outputSize < m_bufSize)) {
            enbytes = xlz->deflate_buffer(m_deflateInput, m_deflateOutput + m_outputSize, inputSize, last_data,
                                          last_buffer, cu_id);
            m_outputSize += enbytes;
            m_inputSize = inputSize;
        } else if ((inputSize >= m_bufSize)) {
            while (inputSize != 0) {
                enbytes = xlz->deflate_buffer(m_deflateInput, m_deflateOutput + m_outputSize, inputSize, last_data,
                                              last_buffer, cu_id);
                m_outputSize += enbytes;
            }
            m_inputSize = 0;
        }
        adler32 = xlz->get_checksum();

        if (m_outputSize > 0) {
            if (flush == Z_FINISH && last_data && enbytes > 0) {
                long int last_block = 0xffff000001;
                // zlib special block based on Z_SYNC_FLUSH
                std::memcpy(&(m_deflateOutput[m_outputSize]), &last_block, 5);
                m_outputSize += 5;

                m_deflateOutput[m_outputSize] = adler32 >> 24;
                m_deflateOutput[m_outputSize + 1] = adler32 >> 16;
                m_deflateOutput[m_outputSize + 2] = adler32 >> 8;
                m_deflateOutput[m_outputSize + 3] = adler32;
                m_outputSize += 4;
            }
            if (m_outputSize > strm->avail_out) {
                std::memcpy(strm->next_out, m_deflateOutput, strm->avail_out);
                m_outputSize -= strm->avail_out;
                std::memmove(m_deflateOutput, m_deflateOutput + strm->avail_out, m_outputSize);
                strm->total_out += strm->avail_out;
                strm->next_out += strm->avail_out;
                strm->avail_out = 0;
            } else {
                std::memcpy(strm->next_out, m_deflateOutput, m_outputSize);
                strm->avail_out = strm->avail_out - m_outputSize;
                strm->total_out += m_outputSize;
                strm->next_out += m_outputSize;
                m_outputSize = 0;
                if (last_data) break;
            }
        }
        if (m_outputSize == 0) {
            if (last_data) break;
        }
    }

    strm->avail_in = m_info.input_size;

    strm->adler = adler32;

    if (last_data && strm->avail_out != 0) {
        return true;
    } else
        return false;
}

// Call to HW Compress
uint64_t xzlib::xilinxHwCompress(z_streamp strm, int val) {
    return this->xilinxHwDeflate(strm, val);
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
        xlz = singleton::getInstance()->getZlibInstance(this, strm, m_info.u50_xclbin, XILINX_INFLATE);
        int err_code = xlz->error_code();
        if (err_code && (err_code != c_clOutOfHostMemory)) {
#if (VERBOSE_LEVEL >= 2)
            std::cout << "Failed to create object " << std::endl;
#endif
            delete xlz;
        }

        // Call to decompress API
        m_info.output_size =
            xlz->decompress(m_info.in_buf, m_info.out_buf, m_info.input_size, m_info.output_size, cu_id);
        if (m_info.output_size == 0) {
#if (VERBOSE_LEVEL >= 2)
            std::cout << "Failed to create device buffer, retrying ... " << std::endl;
#endif
            cu_id++;
            cu_id = cu_id % D_COMPUTE_UNIT;
            retry_flag = true;
            singleton::getInstance()->releaseZlibObj(strm);
        } else {
            retry_flag = false;
        }
    } while (retry_flag);
#else
    // Pick kernel name
    // xilDecompressFull
    std::string kernel_name = "xilDecompressFull";

    // Get XRM CU instance ID
    singleton::getInstance()->getXrmCuInstance(this, kernel_name);

    xlz = singleton::getInstance()->getZlibInstance(this, strm, m_info.u50_xclbin, XILINX_INFLATE);

    int cu = 0;
    auto iterator = singleton::getInstance()->decKernelMap.find(this->instance_name);
    if (iterator != singleton::getInstance()->decKernelMap.end()) cu = iterator->second;

    // Call to decompress API
    m_info.output_size = xlz->decompress(m_info.in_buf, m_info.out_buf, m_info.input_size, m_info.output_size, cu);
    if (m_info.output_size == 0) {
#if (VERBOSE_LEVEL >= 1)
        std::cout << "Decompression failed " << std::endl;
#endif
    }

#endif

#if (VERBOSE_LEVEL >= 1)
    openlog("XILINX_UNCOMPRESS", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "strm_ptr %d Input Size %d, UnCompress Size %d \n", strm, m_info.input_size, m_info.output_size);
    closelog();
#endif
    return m_info.output_size;
}
