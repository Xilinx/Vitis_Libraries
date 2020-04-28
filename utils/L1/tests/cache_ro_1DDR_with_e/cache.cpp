#include "xf_utils_hw/cache.hpp"

// XXX: macro with same name as template parameter is defined in this header, for easy matching up.
// so this header must be included AFTER the API header under test.
#include "cache_tb.hpp"

void syn_top(hls::stream<ap_uint<BUSADDRWIDTH> >& raddrStrm,
             hls::stream<bool>& e_raddrStrm,
             hls::stream<ap_uint<BUSDATAWIDTH> >& rdataStrm,
             hls::stream<bool>& e_rdataStrm,
             ap_uint<512>* ddrMem) {
    const int ddrsize = DDRSIZEIN512;
#pragma HLS INTERFACE m_axi offset = slave latency = 64 depth = ddrsize num_write_outstanding = \
    1 num_read_outstanding = 256 max_write_burst_length = 2 max_read_burst_length = 2 bundle = gmem0_0 port = ddrMem

#pragma HLS INTERFACE s_axilite port = ddrMem bundle = control

    xf::common::utils_hw::cache<ap_uint<BUSDATAWIDTH>, RAMROW, GRPRAM, EACHLINE, BUSADDRWIDTH, 1, 1, 1> dut;

    dut.initSingleOffChip();

    dut.readOnly(ddrMem, raddrStrm, e_raddrStrm, rdataStrm, e_rdataStrm);
}
