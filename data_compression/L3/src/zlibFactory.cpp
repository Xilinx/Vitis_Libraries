#include "zlibFactory.hpp"

// Private constructor
zlibFactory::zlibFactory(void) {}

// Provide the zlibFactory instance
// Called in deflateInit2_
zlibFactory* zlibFactory::getInstance(void) {
    static zlibFactory instance;
    return &instance;
}

void zlibFactory::xilinxPreChecks(void) {
    lock();
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
            this->m_xMode = false;
        } else {
#endif
            // By defalut try to pick XCLBIN from /opt area
            this->m_u50_xclbin = std::string(c_installRootDir) + c_hardXclbinPath;

#ifdef XILINX_DEBUG
            // Check for ENT variable for XCLBIN if it doesnt
            // exist then use hardcoded path to pick
            char* xclbin = getenv("XILINX_LIBZ_XCLBIN");
            if (xclbin) this->m_u50_xclbin = xclbin;
#endif
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
                this->m_xMode = false;
            } else {
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
                    this->m_xMode = false;
                } else {
                    // Getting ACCELERATOR Devices and platform info
                    OCL_CHECK(err, err = platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &m_ndevices));
                    cl_context_properties temp = (cl_context_properties)(platforms[idx])();
                    this->m_platform = temp;

                    // Create OpenCL context
                    cl_context_properties props[3] = {CL_CONTEXT_PLATFORM, this->m_platform, 0};

                    int deviceCount = this->m_ndevices.size();
                    this->m_deviceCount.resize(deviceCount);
                    this->m_deviceContext.resize(deviceCount);
                    this->m_deviceProgram.resize(deviceCount);
                    for (int i = 0; i < deviceCount; i++) {
                        this->m_deviceCount[i] = this->m_ndevices[i];
                        OCL_CHECK(err, this->m_deviceContext[i] =
                                           new cl::Context(this->m_deviceCount[i], props, NULL, NULL, &err));
                        if (err) this->m_xMode = false;

                        std::ifstream bin_file(this->m_u50_xclbin.c_str(), std::ifstream::binary);
                        if (bin_file.fail()) {
                            std::cerr << "XCLBIN1: Unable to open binary file" << std::endl;
                            this->m_xMode = false;
                            bin_file.close();
                        } else {
                            bin_file.seekg(0, bin_file.end);
                            auto nb = bin_file.tellg();
                            bin_file.seekg(0, bin_file.beg);

                            std::vector<uint8_t> buf;
                            buf.resize(nb);
                            bin_file.read(reinterpret_cast<char*>(buf.data()), nb);

                            cl::Program::Binaries bins{{buf.data(), buf.size()}};
                            bin_file.close();
                            // Try to load light weight XCLBIN
                            // If it fails then load full XCLBIN
                            this->m_deviceProgram[i] =
                                new cl::Program(*this->m_deviceContext[i], {this->m_deviceCount[i]}, bins, NULL, &err);
                            if (err != CL_SUCCESS) {
                                if (this->m_deviceProgram[i]) {
                                    delete this->m_deviceProgram[i];
                                    this->m_deviceProgram[i] = nullptr;
                                }
                                // Open Full XCLBIN
                                std::ifstream bin_file((std::string(c_installRootDir) + c_hardFullXclbinPath),
                                                       std::ifstream::binary);
                                if (bin_file.fail()) {
                                    std::cerr << "XCLBIN2: Unable to open binary file" << std::endl;
                                    this->m_xMode = false;
                                    bin_file.close();
                                } else {
                                    bin_file.seekg(0, bin_file.end);
                                    auto nb = bin_file.tellg();
                                    bin_file.seekg(0, bin_file.beg);
                                    buf.clear();
                                    buf.resize(nb);
                                    bin_file.read(reinterpret_cast<char*>(buf.data()), nb);
                                    cl::Program::Binaries bins{{buf.data(), buf.size()}};
                                    // Load full XCLBIN
                                    this->m_deviceProgram[i] = new cl::Program(
                                        *this->m_deviceContext[i], {this->m_deviceCount[i]}, bins, NULL, &err);
                                    if (err != CL_SUCCESS) {
                                        std::cerr << "Failed to program the device " << std::endl;
                                        this->m_xMode = false;
                                        bin_file.close();
                                    }
                                    bin_file.close();
                                }
                            }
                        }
                    }
                }
            }
#ifdef XILINX_DEBUG
        }
#endif
    }
    unlock();
}

// Get method for xMode
bool zlibFactory::getXmode(void) {
    return this->m_xMode;
}

// Provde Driver object by zlibFactory object
zlibDriver* zlibFactory::getDriverInstance(z_streamp strm, int flow, bool init) {
    lock();
    std::string kernel;
    zlibDriver* ret = nullptr;

    // Find current z_stream object if it exists in map
    auto iterator = this->m_driverMapObj.find(strm);
    if (iterator != this->m_driverMapObj.end()) {
        // If it exists the return
        ret = iterator->second;
    } else {
        zlibDriver* driver = new zlibDriver(strm, this->m_platform, this->m_u50_xclbin, flow, this->m_ndevices,
                                            this->m_deviceContext, this->m_deviceProgram, init);
        if (driver->getErrStatus()) {
            ret = nullptr;
        } else {
            // Insert the strm,xlz key/value pairs in map
            this->m_driverMapObj.insert(std::pair<z_streamp, zlibDriver*>(strm, driver));
            ret = driver;
        }
    }
    unlock();
    // return the object
    return ret;
}

void zlibFactory::releaseZlibObj(z_streamp strm) {
    lock();
    // Find if current z_stream exists in map
    auto driverIter = this->m_driverMapObj.find(strm);
    if (driverIter != this->m_driverMapObj.end()) {
        zlibDriver* driver = driverIter->second;
        driver->releaseZlibCU(strm);
    }
    unlock();
}

// Release the zlibDriver by zlibFactory object
void zlibFactory::releaseDriverObj(z_streamp strm) {
    lock();
    // Find if current z_stream exists in map
    auto driverIter = this->m_driverMapObj.find(strm);
    if (driverIter != this->m_driverMapObj.end()) {
        delete driverIter->second;
        // Remove <key,value> pair from map
        this->m_driverMapObj.erase(driverIter);
    }
    unlock();
}
