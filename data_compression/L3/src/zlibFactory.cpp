#include "zlibFactory.hpp"

#ifdef ENABLE_XRM
// Initialize the static XRM context
xrmContext* zlibFactory::m_ctx = nullptr;
#endif

// Private constructor
zlibFactory::zlibFactory(void) {}

// Provide the zlibFactory instance
// Called in deflateInit2_
zlibFactory* zlibFactory::getInstance(void) {
    static zlibFactory instance;
    return &instance;
}

#ifdef ENABLE_XRM
// Provide XRM CU Instance
xrmStruct* zlibFactory::getXrmCuInstance(z_streamp strm, const std::string& kernel_name) {
    auto iterator = m_xrmMapObj.find(strm);
    if (iterator != m_xrmMapObj.end()) {
        // Return if it exists
        return iterator->second;
    } else {
        // Create new xrmData structure for
        // current stream pointer
        xrmStruct* xrmData = new xrmStruct;
        if (xrmData == nullptr) return nullptr;

        // Insert the stream,xrmdataobj key/value pairs in map
        m_xrmMapObj.insert(std::pair<z_streamp, xrmStruct*>(strm, xrmData));

        //// Query XRM
        memset(&(xrmData->cuProp), 0, sizeof(xrmCuProperty));
        memset(&(xrmData->cuRes), 0, sizeof(xrmCuResource));

        // Assign kernel name to look for CU allocation
        strncpy(xrmData->cuProp.kernelName, kernel_name.c_str(), kernel_name.size() + 1);
        strcpy(xrmData->cuProp.kernelAlias, "");

        // Open device in exclusive mode
        xrmData->cuProp.devExcl = false;
        // % of load
        xrmData->cuProp.requestLoad = 100;
        xrmData->cuProp.poolId = 0;

        // Block until get exclusive access to the CU
        int ret = xrmCuBlockingAlloc(m_ctx, &(xrmData->cuProp), 2, &(xrmData->cuRes));
        if (ret != 0) {
#if (VERBOSE_LEVEL >= 1)
            std::cout << "xrmCuAlloc: Failed to allocate CU \n" << std::endl;
#endif
            return nullptr;
        }
        return xrmData;
    }
}
#endif

void zlibFactory::xilinxPreChecks(void) {
    if (this->m_PreCheckStatus) {
        this->m_PreCheckStatus = false;

#ifdef XILINX_DEBUG
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
#endif

#ifdef ENABLE_XRM
        // XRM Context Creation
        m_ctx = (xrmContext*)xrmCreateContext(XRM_API_VERSION_1);
        if (m_ctx == nullptr) {
#if (VERBOSE_LEVEL >= 1)
            std::cout << "Failed to create XRM Context " << std::endl;
#endif
            m_xMode = false;
            return;
        }

        bool ret = xrmIsDaemonRunning(m_ctx);
        if (ret == true) {
#if (VERBOSE_LEVEL >= 1)
            std::cout << "XRM Daemon is Running " << std::endl;
#endif
        } else {
#if (VERBOSE_LEVEL >= 1)
            std::cout << "XRM Daemon is Not Runnig " << std::endl;
#endif
            xrmDestroyContext(m_ctx);
            m_xMode = false;
            return;
        }
#endif
        // XCLBIN -- CHECK
        // By defalut try to pick XCLBIN from /opt area
        this->m_u50_xclbin = std::string(c_installRootDir) + c_hardXclbinPath;

        // Check for ENT variable for XCLBIN if it doesnt
        // exist then use hardcoded path to pick
        char* xclbin = getenv("XILINX_LIBZ_XCLBIN");
        if (xclbin) this->m_u50_xclbin = xclbin;

        // Set Xilinx XRT path
        setenv("XILINX_XRT", "/opt/xilinx/xrt", 1);

        // Set Xilinx XRT_INI_PATH
        std::string hpath = std::string(c_installRootDir) + "zlib/scripts/xrt.ini";
        setenv("XRT_INI_PATH", hpath.c_str(), 1);

        // Create Platform & Device information
        cl_int err;

        // Look for platform
        std::vector<cl::Platform> platforms;
        OCL_CHECK(err, err = cl::Platform::get(&platforms));
        auto num_platforms = platforms.size();
        if (num_platforms == 0) {
#if (VERBOSE_LEVEL >= 2)
            std::cerr << "No Platforms were found this could be cased because of the OpenCL \
                          ICD not installed at /etc/OpenCL/vendors directory"
                      << std::endl;
#endif
            m_xMode = false;
            return;
        }

        std::string platformName;
        cl::Platform platform;
        bool foundFlag = false;
        int idx = 0;
        for (size_t i = 0; i < platforms.size(); i++) {
            platform = platforms[i];
            OCL_CHECK(err, platformName = platform.getInfo<CL_PLATFORM_NAME>(&err));
            if (platformName == "Xilinx") {
                idx = i;
                foundFlag = true;
#if (VERBOSE_LEVEL >= 2)
                std::cout << "Found Platform" << std::endl;
                std::cout << "Platform Name: " << platformName.c_str() << std::endl;
#endif
                break;
            }
        }
        if (foundFlag == false) {
            std::cerr << "Error: Failed to find Xilinx platform" << std::endl;
            m_xMode = false;
            return;
        }
        // Getting ACCELERATOR Devices and platform info
        OCL_CHECK(err, err = platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &m_ndevices));
        cl_context_properties temp = (cl_context_properties)(platforms[idx])();
        this->m_platform = temp;
    }
}

// Get method for xMode
bool zlibFactory::getXmode(void) {
    return m_xMode;
}

// Provde Driver object by zlibFactory object
zlibDriver* zlibFactory::getDriverInstance(z_streamp strm, int flow) {
    lock();
    std::string kernel;

    if (flow == XILINX_DEFLATE)
        kernel = compress_kernel_names[1];
    else
        kernel = stream_decompress_kernel_name[2];

    // Find current z_stream object if it exists in map
    auto iterator = m_driverMapObj.find(strm);
    if (iterator != m_driverMapObj.end()) {
        unlock();
        // If it exists the return
        return iterator->second;
    } else {
        std::string instance_name;
        cl::Device device_id;
        int bank_id = 0;
#ifdef ENABLE_XRM
        // Created relavant XRM object
        xrmStruct* xrmData = getXrmCuInstance(strm, kernel);
        if (xrmData == nullptr) {
            releaseDriverObj(strm);
            unlock();
            return nullptr;
        }
        instance_name = xrmData->cuRes.instanceName;
        device_id = m_ndevices[xrmData->cuRes.deviceId];
        bank_id = xrmData->cuRes.cuId % C_COMPUTE_UNIT;
#else
        instance_name = kernel;
        std::random_device randDevice;
        std::uniform_int_distribution<> range(0, (m_ndevices.size() - 1));
        uint8_t devNo = range(randDevice);
        device_id = m_ndevices[devNo];
#endif

        // If not create it
        zlibDriver* driver = new zlibDriver(strm, instance_name, device_id, m_platform, m_u50_xclbin, flow, bank_id);
        if (driver->getErrStatus()) {
            releaseDriverObj(strm);
            unlock();
            return nullptr;
        }
        // Insert the strm,xlz key/value pairs in map
        m_driverMapObj.insert(std::pair<z_streamp, zlibDriver*>(strm, driver));

        // Do prechecks
        unlock();
        // return the object
        return driver;
    }
}

// Release the zlibDriver by zlibFactory object
void zlibFactory::releaseDriverObj(z_streamp strm) {
    // Find if current z_stream exists in map
    auto driverIter = m_driverMapObj.find(strm);
    if (driverIter != m_driverMapObj.end()) {
        delete driverIter->second;
        // Remove <key,value> pair from map
        m_driverMapObj.erase(driverIter);
    }
#ifdef ENABLE_XRM
    // XRM iterator
    auto xrmIter = m_xrmMapObj.find(strm);
    if (xrmIter != m_xrmMapObj.end()) {
        if (m_ctx != NULL) {
            xrmCuRelease(m_ctx, &(xrmIter->second->cuRes));
        }
        delete xrmIter->second;
        // Remove <key,value> pair from map
        m_xrmMapObj.erase(xrmIter);
    }
#endif
}
