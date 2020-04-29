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

void syn_top(int size,
             hls::stream<ap_uint<BUSADDRWIDTH> >& raddrStrm,
             hls::stream<ap_uint<BUSDATAWIDTH> >& rdataStrm0,
             hls::stream<ap_uint<BUSDATAWIDTH> >& rdataStrm1,
             ap_uint<512>* ddrMem0,
             ap_uint<512>* ddrMem1);
