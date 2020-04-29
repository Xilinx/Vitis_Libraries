#include <iostream>
#include <stdlib.h>
#include <hls_stream.h>
#include <ap_int.h>

#define DDRSIZEIN512 (5000)
#define BUSADDRWIDTH (32)
#define BUSDATAWIDTH (64)
#define EACHLINE (512 / BUSDATAWIDTH)
#define RAMROW (4096)
#define GRPRAM (4)

#define TOTALADDRWIDTH (15) // should be smaller for CoSim 15 for example

void syn_top(hls::stream<ap_uint<BUSADDRWIDTH> >& raddrStrm,
             hls::stream<bool>& e_raddrStrm,
             hls::stream<ap_uint<BUSDATAWIDTH> >& rdataStrm,
             hls::stream<bool>& e_rdataStrm,
             ap_uint<512>* ddrMem);
