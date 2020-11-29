#include "zlibDriver.hpp"
// Constructor
zlibDriver::zlibDriver(z_streamp strm,
                       const std::string& instance_name,
                       const cl::Device& device_id,
                       const cl_context_properties& platform_id,
                       const std::string& u50_xclbin,
                       const int flow,
                       const int bank // useful only for xrm case, random flow picks bank randomly
                       ) {
    m_kernel_instance = instance_name;
    m_u50_xclbin = u50_xclbin;
    m_flow = flow;
    m_cuid = (instance_name[instance_name.size() - 1] - '0') - 1;
    // Update stream pointer infor
    struct_update(strm, flow);
    m_deflateOutput = new unsigned char[2 * m_bufSize];
    m_deflateInput = new unsigned char[2 * m_bufSize];

    bool err;
    err = this->getZlibInstance(m_kernel_instance, device_id, platform_id, bank);
    if (err) this->m_status = true;
}

// Destructor
zlibDriver::~zlibDriver() {
    if (m_deflateOutput) {
        delete m_deflateOutput;
    }

    if (m_deflateInput) {
        delete m_deflateInput;
    }

    if (m_xlz) {
        delete m_xlz;
    }
}

// Structure update
void zlibDriver::struct_update(z_streamp strm, int flow) {
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

// Get method for status
bool zlibDriver::getStatus(void) {
    return m_status;
}

// Proivde xfZlib object
bool zlibDriver::getZlibInstance(const std::string& instance_name,
                                 const cl::Device& device_id,
                                 const cl_context_properties& platform_id,
                                 const int bank) {
    if (m_flow == XILINX_DEFLATE) {
        // If not create it
        m_xlz = new xfZlib(m_u50_xclbin.c_str(), COMP_ONLY, device_id, platform_id, XILINX_ZLIB, bank);
        if (m_xlz->error_code()) {
            return 1;
        }
    } else if (m_flow == XILINX_INFLATE) {
        // If not create it
        m_xlz = new xfZlib(m_u50_xclbin.c_str(), DECOMP_ONLY, device_id, platform_id, XILINX_ZLIB);
        if (m_xlz->error_code()) {
            return 1;
        }
    }
}

// Call to HW Deflate
bool zlibDriver::xilinxHwDeflate(z_streamp strm, int flush) {
    uint32_t adler32 = 0;
    size_t enbytes = 0;

    bool last_data = false;

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
        m_xlz->set_checksum(strm->adler);
        size_t inputSize = m_inputSize;
        // Call compression_buffer API
        if (last_buffer && (m_outputSize < m_bufSize)) {
            enbytes = m_xlz->deflate_buffer(m_deflateInput, m_deflateOutput + m_outputSize, inputSize, last_data,
                                            last_buffer, m_kernel_instance);
            m_outputSize += enbytes;
            m_inputSize = inputSize;
        } else if ((inputSize >= m_bufSize)) {
            while (inputSize != 0) {
                enbytes = m_xlz->deflate_buffer(m_deflateInput, m_deflateOutput + m_outputSize, inputSize, last_data,
                                                last_buffer, m_kernel_instance);
                m_outputSize += enbytes;
            }
            m_inputSize = 0;
        }
        adler32 = m_xlz->get_checksum();

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
uint64_t zlibDriver::xilinxHwCompress(z_streamp strm, int val) {
    return this->xilinxHwDeflate(strm, val);
}

// Call to HW decompress
uint64_t zlibDriver::xilinxHwUncompress(z_streamp strm, int cu) {
#ifdef ENABLE_XRM
    // Call decompress API
    m_info.output_size =
        m_xlz->decompress(m_info.in_buf, m_info.out_buf, m_info.input_size, m_info.output_size, m_cuid);
    if (m_info.output_size == 0) {
#if (VERBOSE_LEVEL >= 1)
        std::cout << "Decompression Failed " << std::endl;
#endif
    }
#endif
    return m_info.output_size;
}
